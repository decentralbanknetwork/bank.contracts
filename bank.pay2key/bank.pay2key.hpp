#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/types.h>
#include <eosiolib/crypto.h>
#include <eosiolib/asset.hpp>

using namespace eosio;
using namespace std;

const std::string WITHDRAW_ADDRESS = "1111111111111111111114oLvT2";
uint8_t WITHDRAW_KEY_BYTES[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 148, 160, 9, 17 };

typedef string bitcoin_address;
struct bitcoin_bin {
    char data[25];
};

class [[eosio::contract("bank.pay2key")]] pay2key : public contract {

public:
    using contract::contract;

    [[eosio::action]]
    void create(name token_contract, symbol ticker);

    [[eosio::action]]
    void transfer(
                uint64_t chain_id,
                name relayer_account,
                bitcoin_address relayer,
                bitcoin_address from,
                bitcoin_address to,
                asset amount,
                asset fee,
                uint64_t nonce,
                string memo,
                signature sig);

    // Public but not a directly callable action
    // Called indirectly by sending EOS to this contract
    void issue( name from, name to, asset quantity, string memo );

    struct [[eosio::table]] account {
      uint64_t key;
      bitcoin_address publickey;
      asset balance;
      uint64_t last_nonce;

      uint64_t primary_key() const { return key; }
      fixed_bytes<32> bypk() const {
        return bitcoin_address_to_fixed_bytes(publickey);
      };
    };

    struct [[eosio::table]] currstats {
        uint64_t chain_id;
        asset supply;
        name token_contract;
        eosio::symbol symbol;

        uint64_t primary_key() const { return chain_id; }
        uint64_t by_token_contract() const { return token_contract.value; }
        uint128_t by_contract_symbol() const { return merge_contract_symbol(token_contract, symbol); }
    };

    typedef eosio::multi_index<"btcaccounts"_n,
                               account,
                               indexed_by<"bypk"_n, const_mem_fun<account, fixed_bytes<32>, &account::bypk>>
                              > accounts;

    typedef eosio::multi_index<"stats"_n, currstats,
       indexed_by<"bycontract"_n, const_mem_fun<currstats, uint64_t, &currstats::by_token_contract>>,
       indexed_by<"byctrsym"_n, const_mem_fun<currstats, uint128_t, &currstats::by_contract_symbol>>
    > stats;

  private:

    static fixed_bytes<32> bitcoin_address_to_fixed_bytes(bitcoin_address address) {
        return sha256(address.c_str(), address.size());
    }

    void sub_balance(uint64_t chain_id, bitcoin_address sender, asset value);

    void add_balance(uint64_t chain_id, bitcoin_address recipient, asset value, name ram_payer);

    static uint128_t merge_contract_symbol( name contract, symbol sym ) {
        uint128_t merged;
        uint64_t raw_sym = sym.raw();
        memcpy((uint8_t *)&merged, (uint8_t *)&contract.value, 8);
        memcpy((uint8_t *)&merged + 8, (uint8_t *)&raw_sym, 8);
        return merged;
    }

    bitcoin_bin bitcoin_address_to_bin(bitcoin_address address);

    void validate_bitcoin_address(bitcoin_address address);

    bitcoin_address public_key_to_bitcoin_address (public_key key);

};
