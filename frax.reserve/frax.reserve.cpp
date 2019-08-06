#include "bank.pay2key.hpp"
#include "base58.c"

[[eosio::action]]
void fraxreserve::addtoken(name token_contract, symbol ticker) {
    require_auth( _self );

    eosio_assert(ticker.is_valid(), "invalid symbol name");

    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(token_contract, ticker);
    auto existing = contract_sym_index.find(merged);
    eosio_assert(existing == contract_sym_index.end(), "Token is already registered");

    asset supply = asset(0, ticker);

    statstable.emplace(_self, [&](auto& s) {
        s.chain_id = statstable.available_primary_key();
        s.symbol = ticker;
        s.token_contract = token_contract;
        s.supply = supply;
    });
}

void fraxreserve::deposit( name from, name to, asset quantity, string memo ) {
    if (from == _self) return; // sending tokens, ignore

    auto symbol = quantity.symbol;
    eosio_assert(symbol.is_valid(), "invalid symbol name");
    eosio_assert(memo.size() == 53, "memo must be a 53-char EOS public key");
    eosio_assert(memo.substr(0,3) == "EOS", "public key must start with EOS");
    eosio_assert(to == _self, "stop trying to hack the contract");

    // make sure contract and symbol are accepted by contract
    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(_code, symbol);
    auto existing = contract_sym_index.find(merged);
    eosio_assert(existing != contract_sym_index.end(), "Token is not yet supported. You must assign it a chain_id with `addtoken` first");
    const auto& st = *existing;

    // validate arguments
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    auto _self = receiver;
    if (code != _self && action == name("transfer").value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &fraxreserve::deposit
        );
    }
    else if (code == _self && action == name("addtoken").value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &fraxreserve::addtoken
        );
    }
}
