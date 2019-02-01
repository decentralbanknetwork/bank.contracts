#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("bank.shares")]] bankshares : public contract {
    private:

    public:
        using contract::contract;
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
     typedef eosio::multi_index<"accounts"_n, account> accounts_tbl;
     typedef eosio::multi_index<"stats"_n, currency_stats> stats_tbl;
}