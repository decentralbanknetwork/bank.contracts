#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.h>
#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("frax.reserve")]] fraxreserve : public contract {

public:
    using contract::contract;
    
    [[eosio::action]]
    void addtoken(name contract, symbol ticker);

    // Public but not a directly callable action
    // Called indirectly by sending EOS to this contract
    void deposit( name from, name to, asset quantity, string memo );

    struct [[eosio::table]] account {
      symbol ticker;
      asset balance;

      uint64_t primary_key() const { return ticker.raw(); }
    };
    typedef eosio::multi_index<"accounts"_n, account> accounts;

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

  private:

    static uint128_t merge_contract_symbol( name contract, symbol sym ) {
        uint128_t merged;
        uint64_t raw_sym = sym.raw();
        memcpy((uint8_t *)&merged, (uint8_t *)&contract.value, 8);
        memcpy((uint8_t *)&merged + 8, (uint8_t *)&raw_sym, 8);
        return merged;
    }

};
