#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.h>
#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

const symbol FRAX_SYMBOL = symbol(symbol_code("FRAX"), 4);
const symbol FXS_SYMBOL = symbol(symbol_code("FXS"), 4);
const symbol USDT_SYMBOL = symbol(symbol_code("USDT"), 4);

class [[eosio::contract("frax.reserve")]] fraxloans : public contract {

public:
    using contract::contract;
    
    [[eosio::action]]
    void addtoken(name contract, symbol ticker);

    [[eosio::action]]
    void borrow(name borrower, asset quantity);

    [[eosio::action]]
    void setprice(asset price);

    // Public but not a directly callable action
    // Called indirectly by sending EOS to this contract
    void deposit( name from, name to, asset quantity, string memo );

    // Deposits table
    // Scoped by user
    struct [[eosio::table]] account {
      asset balance;
      asset borrowing;
      uint64_t last_updated;

      uint64_t primary_key() const { return balance.symbol.raw(); }
    };
    typedef eosio::multi_index<"accounts"_n, account> accounts;

    // Token stats
    // Contract scope
    struct [[eosio::table]] stats_t {
        asset available;
        asset loaned;
        name contract;
        asset price; // 4 decimal places, prices in USDT
        uint64_t interest_counter;


        uint64_t primary_key() const { return supply.symbol.raw(); }
        uint64_t by_contract() const { return contract.value; }
        uint128_t by_contract_symbol() const { return merge_contract_symbol(contract, supply.symbol); }
    };
    typedef eosio::multi_index<"stats"_n, stats_t, 
       indexed_by<"bycontract"_n, const_mem_fun<stats_t, uint64_t, &stats_t::by_contract>>,
       indexed_by<"byctrsym"_n, const_mem_fun<stats_t, uint128_t, &stats_t::by_contract_symbol>>
    > stats;

    // System parameters
    // Singleton - Contract scope
    struct [[eosio::table]] params_t {

        uint64_t primary_key() const { return 1; } // constant primary key forces singleton
    };
    typedef eosio::multi_index<"sysparams"_n, params_t > sysparams;

  private:

    static uint128_t merge_contract_symbol( name contract, symbol sym ) {
        uint128_t merged;
        uint64_t raw_sym = sym.raw();
        memcpy((uint8_t *)&merged, (uint8_t *)&contract.value, 8);
        memcpy((uint8_t *)&merged + 8, (uint8_t *)&raw_sym, 8);
        return merged;
    }

};
