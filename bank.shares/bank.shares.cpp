#include "bank.shares.hpp"


// Start a reverse Dutch auction exchange BANK (or SYS) for bank.token currency
[[eosio::action]]
void bankshares::startauction(asset quantity, asset maxbid, string memo, symbol exchcurrsymb, time_point end_time) {
    require_auth( _self );

    // Validate issue quantity
    auto shareSym = quantity.symbol;
    eosio_assert( shareSym.is_valid(), "Invalid symbol name for asset" );
    eosio_assert( quantity.is_valid(), "Invalid asset");
    eosio_assert( quantity.amount.is_valid(), "Invalid quantity" );
    eosio_assert( quantity.amount > 0, "Issue quantity must be positive");

    // Validate memo
    eosio_assert(memo.size() <= 163, "memo has more than 164 bytes");

    // Validate the exchange currency
    eosio_assert(exchcurrsymb.is_valid(), "Invalid symbol name for exchange currency");

    // Validate that the accepted exchange currency exists. Assuming it has a statstable too
    // Make sure the currency actually exists
    banktoken::stats stats_tbl("bank.token"_n, "bank.token"_n.value);
    auto stats_itr = stats_tbl.find(exchcurrsymb.raw());
    eosio_assert(stats_itr != statstable.end(), "Provided bank.token currency does not exist");

    // Validate the time
    eosio_assert( end_time > now(), "Ending time of the auction must be in the future" );

    // Issue more BANK by calling the system contract
    action counter = action(
        permission_level{_self,"active"_n},
        "eosio.token"_n,
        "issue"_n,
        {"bank.shares"_n, quantity, memo }
    );

    // Create the auction
    auctions auctions_tbl(_self, _self.value);
    auctions_tbl.emplace(_self, [&](auto& auct) {
        auct.id = auctions_tbl.available_primary_key();
        auct.issue = quantity;
        auct.iss_remain = quantity;
        auct.curr_bid = asset(int64_t(0), exchcurrsymb);
        auct.max_bid = maxbid
        auct.symb = exchcurrsymb;
        auct.start_time = now();
        auct.end_time = end_time;
    });
}

void bankshares::updatebid(uint64_t auctionid) {
    // Fetch the auction
    auctions auctions_tbl(_self, _self.value);
    auto auctions_it = auctions_tbl.find( auctionid );

    // Validation
    eosio_assert( auctions_it != auctions_tbl.end(), "Auction not found" );
    eosio_assert( auctions_it->end_time > now(), "Auction has ended" );
    eosio_assert( auctions_it->iss_remain.amount > 0, "Issue has sold out");

    // Calculate the new bid
    // Linear algorithm for now [fraction of time left x maximum bid]
    int64_t newbid_amt = <float>((now() - auctions_it->start_time) / (auctions_it->end_time - auctions_it->start_time)) * auctions_it->max_bid;
    asset newbid = asset(newbid_amt, exchcurrsymb);

    // Sanity check
    eosio_assert( newbid.amount <= auctions_it->max_bid.amount, "Calculated new bid is over maximum bid" );

    // Update the bid
    auctions_tbl.modify( auctions_it, _self, [&]( auto& auct ) {
        auct.curr_bid = newbid;
    });
}

EOSIO_DISPATCH(bankshares, (startauction)(getbid))