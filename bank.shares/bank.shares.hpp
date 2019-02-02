#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("bank.shares")]] bankshares : public contract {
    public:
        using contract::contract;

        // -------------STRUCTS-------------
        // List of auctions
        struct [[eosio::table]] auction {
            uint64_t id;
            asset issue;
            asset iss_remain;
            asset max_bid;
            asset curr_bid;
            symbol symb;
            time_point start_time;
            time_point end_time;

            uint64_t primary_key() const { return id; }
            symbol by_symbol () const { return symb; }
        };

        // -------------ACTIONS-------------
        [[eosio::action]]
        startauction(asset quantity, asset maxbid, string memo, symbol exchcurrsymb, time_point end_time);

        [[eosio::action]]
        bankshares::updatebid(uint64_t auctionid);


        // --------TABLE DEFINITIONS--------
        typedef eosio::multi_index<"auctions"_n, auction,
            indexed_by< "bypk", const_mem_fun<auction, uint64_t, &auction::primary_key>>,
            indexed_by< "bysymbol", const_mem_fun<auction, symbol, &auction::by_symbol >>
        > auctions;
};

// Table info from bank.token contract. Needed for the auction and various other things.
namespace banktoken {

     struct [[eosio::table, eosio::contract("bank.token")]] account {
        asset    balance;

        uint64_t primary_key()const { return balance.symbol.code().raw(); }
     };

     struct [[eosio::table, eosio::contract("bank.token")]] currency_stats {
        asset   supply;
        asset   max_supply;
        name    issuer;

        uint64_t primary_key()const { return supply.symbol.code().raw(); }
     };
     typedef eosio::multi_index<"accounts"_n, account> accounts;
     typedef eosio::multi_index<"stats"_n, currency_stats> stats;
}