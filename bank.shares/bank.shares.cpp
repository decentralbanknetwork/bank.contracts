#include "bank.shares.hpp"


// Start a reverse Dutch auction exchange BANK (or SYS) for bank.token currency
[[eosio::action]]
void bankshares::startauction(asset quantity, asset askstart, asset askfloor, string memo, time_point end_time) {
    // Check authorization
    require_auth( _self );

    // Validate issue quantity
    eosio_assert( quantity.symbol.is_valid(), "Invalid symbol name for quantity" );
    eosio_assert( quantity.is_valid(), "Invalid quantity");
    eosio_assert( quantity.symbol.name == SYS_TOKEN_NAME, "Invalid system token name in quantity");
    eosio_assert( quantity.amount.is_valid(), "Invalid quantity quantity" );
    eosio_assert( quantity.amount > 0, "Issue quantity must be positive");

    // Validate askstart quantity
    eosio_assert( askstart.symbol.is_valid(), "Invalid symbol name for askstart" );
    eosio_assert( askstart.is_valid(), "Invalid askstart");
    eosio_assert( askstart.amount.is_valid(), "Invalid askstart quantity" );
    eosio_assert( askstart.amount > 0, "askstart quantity must be positive");

    // Validate askfloor quantity
    eosio_assert( askfloor.symbol.is_valid(), "Invalid symbol name for askfloor" );
    eosio_assert( askfloor.is_valid(), "Invalid askfloor");
    eosio_assert( askfloor.amount.is_valid(), "Invalid askfloor quantity" );
    eosio_assert( askfloor.amount > 0, "askfloor quantity must be positive");

    // Make sure the ask floor and the ask start symbols are the same
    eosio_assert( askstart.symbol.name == askfloor.symbol.name, "askfloor and askstart symbol name mismatch");

    // Validate memo
    eosio_assert(memo.size() <= 163, "memo has more than 164 bytes");

    // Validate the exchange currency
    eosio_assert(exchcurrsymb.is_valid(), "Invalid symbol name for exchange currency");

    // Validate that the accepted exchange currency exists. Assuming it has a statstable too
    // Make sure the currency actually exists
    banktoken::stats stats_tbl("bank.token"_n, "bank.token"_n.value);
    auto stats_it = stats_tbl.find(askfloor.symbol.raw());
    eosio_assert(stats_it != statstable.end(), "Provided bank.token currency does not exist");

    // Validate the time
    eosio_assert( end_time > now(), "Ending time of the auction must be in the future" );

    // Issue more BANK by calling the system contract
    action issuance = action(
        permission_level{_self,"active"_n},
        "eosio.token"_n,
        "issue"_n,
        {"bank.shares"_n, quantity, memo }
    );
    issuance.send();

    // Create the auction
    auctions auctions_tbl(_self, _self.value);
    auctions_tbl.emplace(_self, [&](auto& auct) {
        auct.id = auctions_tbl.available_primary_key();
        auct.issue = quantity;
        auct.iss_remain = quantity;
        auct.current_ask = askstart;
        auct.ask_floor = askfloor;
        auct.start_time = now();
        auct.end_time = end_time;
    });
}

// Sell currency for SYS_TOKEN_NAME shares
[[eosio::action]]
void bankshares::buyshares(uint64_t auctionid, name buyer, asset buyamount, asset maxbid) {
    // Update the ask and also check to make sure the auction exists and is valid.
    // Returns the iterator to the 
    auto auctionsPack = update_ask_local(auctionid);

    // Make sure the buy amount is in BANK / SYS tokens
    eosio_assert( buyamount.symbol.name == SYS_TOKEN_NAME, "Invalid system token name in buyamount");

    // Make sure the bid is in the correct currency that the auction is asking for
    eosio_assert( auctionsPack._it->current_ask.symbol.name == maxbid.symbol.name, "Invalid currency in the maxbid");

    // Price checks
    eosio_assert( maxbid.amount >= auctionsPack._it->current_ask.amount , "Your maximum bid is too low" );

    // Make sure there is suffient SYS_TOKEN_NAME left to sell. 
    eosio_assert( auctionsPack._it->iss_remain >= buyamount.amount, "Not enough BANK left to buy");

    // Make sure the seller has enough currency 
    banktoken::accounts accounts_tbl(buyer, buyer.value);
    auto accounts_it = accounts_tbl.find(sellamount.symbol.raw());
    eosio_assert(accounts_it != accounts_tbl.end(), "No balance found for the provided currency in bank.token");
    eosio_assert(accounts_it->balance.amount >= (buyamount.amount * auctionsPack._it->current_ask.amount), "You don't have enough currency to make a complete purchase");

}

// Update the ask (blockchain accessible)
[[eosio::action]]
void bankshares::updateask(uint64_t auctionid) {
    update_ask_local(auctionid);
}

// Update the ask (internal function). Returns the auction iterator and table as a struct
bankshares::AuctionPack bankshares::update_ask_local(uint64_t auctionid){
    // Fetch the auction and store it in the struct
    auctions auctions_tbl(_self, _self.value);
    auto auctions_it = auctions_tbl.find( auctionid );
    bankshares::AuctionPack aPack = {auctions_it, auctions_tbl};

    // Validation
    eosio_assert( auctions_it != auctions_tbl.end(), "Auction not found" );
    eosio_assert( auctions_it->end_time > now(), "Auction has ended" );
    eosio_assert( auctions_it->iss_remain.amount > 0, "Issue has sold out");

    // Calculate the new asking price for 1 SYS_TOKEN_NAME
    // Linear algorithm for now [fraction of time left x (current_ask - ask_floor) + ask_floor]
    float fraction_time_left = <float>((now() - auctions_it->start_time) / (auctions_it->end_time - auctions_it->start_time))
    int64_t newask_amt =  <int64_t>(fraction_time_left * (auctions_it->current_ask - auctions_it->ask_floor) + auctions_it->ask_floor)
    asset newask = asset(newask_amt, auctions_it->get_symbol());

    // Make sure the new ask is bounded by the current ask and the floor
    eosio_assert( newask.amount >= auctions_it->ask_floor.amount, "Calculated new ask is below floor ask" );
    eosio_assert( newask.amount < auctions_it->current_ask.amount, "Calculated new ask is above current ask" );

    // Update the ask
    auctions_tbl.modify( auctions_it, _self, [&]( auto& auct ) {
        auct.current_ask = newask;
    });

    return aPack;
}

EOSIO_DISPATCH(bankshares, (startauction)(updateask))