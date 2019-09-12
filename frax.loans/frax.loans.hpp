#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("frax.loans")]] fraxloans : public contract {

public:
    using contract::contract;

    // Constants
    const symbol FRAX_SYMBOL = symbol(symbol_code("FRAX"), 4);
    const symbol FXS_SYMBOL = symbol(symbol_code("FXS"), 4);
    const symbol USDT_SYMBOL = symbol(symbol_code("USDT"), 4);
    const double COLLATERAL_RATIO = 0.75; // Can only borrow upto 75% of collateral
    
    [[eosio::action]]
    void addtoken(name contract, symbol ticker);

    [[eosio::action]]
    void borrow(name borrower, asset quantity);

    [[eosio::action]]
    void repay(name borrower, asset quantity);

    [[eosio::action]]
    void setprice(asset price);

    //[[eosio::action]]
    //void liquidate(name user, name executor);
    
	[[eosio::on_notify("tmp::tmp")]] void tmp() { } // temp hack. required to make on_notify work until bug is patched

	[[eosio::on_notify("*::transfer")]] 
    void deposit( name from, name to, asset quantity, string memo );

    // Deposits table
    // Scoped by user
    struct [[eosio::table]] account {
      asset balance;
      asset borrowing;
      uint64_t last_updated;

      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };
    typedef eosio::multi_index<"accounts"_n, account> accounts;

    // Token stats
    // Contract scope
    struct [[eosio::table]] stats_t {
        asset available;
        asset loaned;
        name contract;
        asset price; // 4 decimal places, prices in USDT
        bool allowed_as_collateral;
        bool can_deposit;
        uint64_t interest_counter;

        uint64_t primary_key() const { return available.symbol.code().raw(); }
        uint64_t by_contract() const { return contract.value; }
        uint128_t by_contract_symbol() const { return merge_contract_symbol(contract, available.symbol); }
    };
    typedef eosio::multi_index<"tokenstats"_n, stats_t, 
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
