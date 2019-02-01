#include "bank.shares.hpp"


// Start a reverse Dutch auction exchange BANK (or SYS) for bank.token currency
[[eosio::action]]
void bankshares::startauction(asset quantity, string memo, symbol exchcurrsymb, time timeperiod) {
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
    banktoken::stats_tbl bnkTokenStats("bank.token"_n, "bank.token"_n.value);
    auto stats_itr = bnkTokenStats.find(exchcurrsymb.raw());
    eosio_assert(stats_itr != statstable.end(), "Provided bank.token currency does not exist");

    // Validate the time
    eosio_assert( timeperiod > 0, "Time period of the auction must be positive" );

    // Issue more BANK by calling the system contract
    action counter = action(
        permission_level{get_self(),"active"_n},
        "eosio.token"_n,
        "issue"_n,
        {"bank.shares"_n, quantity, memo }
    );

}