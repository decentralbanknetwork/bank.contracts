#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/crypto.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

const symbol FRAX_SYMBOL = symbol(symbol_code("FRAX"), 4);
const symbol FXS_SYMBOL = symbol(symbol_code("FXS"), 4);
const symbol USDT_SYMBOL = symbol(symbol_code("USDT"), 4);

class [[eosio::contract("frax.reserve")]] fraxreserve : public contract {

public:
    using contract::contract;
    
    [[eosio::action]]
    void addtoken(name contract, symbol ticker);

    [[eosio::action]]
    void buyfrax(name buyer, asset frax);

    [[eosio::action]]
    void settarget(asset reserve_usdt, asset reserve_fxs, uint64_t fxs_price);
    
    // Public but not a directly callable action
    // Called indirectly by sending EOS to this contract
    void deposit( name from, name to, asset quantity, string memo );

    // Deposits table
    // Scoped by user
    struct [[eosio::table]] account {
      asset balance;

      uint64_t primary_key() const { return balance.symbol.raw(); }
    };
    typedef eosio::multi_index<"accounts"_n, account> accounts;

    // Token stats
    // Contract scope
    struct [[eosio::table]] stats_t {
        asset supply;
        name contract;

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
        asset target_usdt;
        asset target_fxs;
        uint64_t fxs_price; // (FXS / USDT) * 1e4. Ex: $20 FXS. Set to 20e4

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
