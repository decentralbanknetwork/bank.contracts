#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>

using namespace eosio;
using namespace std;

const name SYS_TOKEN_NAME = "BANK"_n;

class [[eosio::contract("bank.shares")]] bankshares : public contract {
    public:
        using contract::contract;

        // Return type for the updater function
        struct AuctionPack {
            auto _it;
            auctions _tbl;
        }

        // -------------STRUCTS-------------
        // List of auctions
        struct [[eosio::table]] auction {
            uint64_t id;
            asset issue; // Amount of SYS_TOKEN_NAME being issued
            asset iss_remain; // SYS_TOKEN_NAME left to be sold
            asset current_ask; // Current asking price for 1 SYS_TOKEN_NAME, in CURR
            asset ask_floor; // Minimum asking price for 1 SYS_TOKEN_NAME that you are willing to accept, in CURR
            time_point start_time; // Starting time of the auction
            time_point end_time; // Ending time of the auction

            uint64_t primary_key() const { return id; }
            symbol get_symbol () const { return ask_floor.symbol; }
        };

        // -------------ACTIONS-------------
        [[eosio::action]]
        startauction(
            asset quantity, // Amount of SYS_TOKEN_NAME being issued
            asset askstart, // Starting asking price for 1 SYS_TOKEN_NAME, in CURR
            asset askfloor, // Minimum asking price for 1 SYS_TOKEN_NAME that you are willing to accept, in CURR
            string memo, // A memo
            time_point end_time // The ending time, in epoch milliseconds
        );

        [[eosio::action]]
        updateask(
            uint64_t auctionid // The id of the auction
        );

        [[eosio::action]]
        buyshares(
            uint64_t auctionid, // ID of the auction you want to participate in
            name buyer, // The name of you, the buyer
            asset buyamount, // The amount of SYS_TOKEN_NAME that you want to buy
            asset maxbid // The maximum price per 1 SYS_TOKEN_NAME that you are willing to pay, in CURR
        );

        // --------TABLE DEFINITIONS--------
        typedef eosio::multi_index<"auctions"_n, auction,
            indexed_by< "bypk", const_mem_fun<auction, uint64_t, &auction::primary_key>>,
            indexed_by< "bysymbol", const_mem_fun<auction, symbol, &auction::get_symbol >>
        > auctions;

    private:
        // Update the bid internally 
        AuctionPack update_ask_local(uint64_t auctionid);
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