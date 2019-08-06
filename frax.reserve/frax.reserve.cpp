#include "frax.reserve.hpp"

[[eosio::action]]
void fraxreserve::addtoken(name contract, symbol ticker) {
    require_auth( _self );

    eosio_assert(ticker.is_valid(), "invalid symbol name");

    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(contract, ticker);
    auto existing = contract_sym_index.find(merged);
    eosio_assert(existing == contract_sym_index.end(), "Token is already registered");
    eosio_assert(statstable.find(ticker.raw()) == statstable.end(), "Another token with that ticker already exists");

    statstable.emplace(_self, [&](auto& s) {
        s.contract = contract;
        s.supply = asset(0, ticker);
    });
}

void fraxreserve::deposit( name from, name to, asset quantity, string memo ) {
    if (from == _self) return; // sending tokens, ignore

    auto symbol = quantity.symbol;
    eosio_assert(symbol.is_valid(), "invalid symbol name");
    eosio_assert(to == _self, "stop trying to hack the contract");

    // make sure contract and symbol are accepted by contract
    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(_code, symbol);
    auto existing = contract_sym_index.find(merged);
    eosio_assert(existing != contract_sym_index.end(), "Token is not yet supported");

    // validate arguments
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount < 1e10, "quantity is suspiciously high. send a smaller amount");
    eosio_assert(quantity.amount > 0, "must deposit positive quantity");
    eosio_assert(memo.size() < 256, "memo must be less than 256 bytes");

    // create/update entry in table
    accounts acctstbl( _self, from.value );
    auto account_it = acctstbl.find(symbol.raw());
    if (account_it == acctstbl.end()) {
        acctstbl.emplace( _self, [&](auto& a) {
            a.ticker = symbol;
            a.balance = quantity;
        });
    }
    else {
        acctstbl.modify( account_it, _self, [&](auto& a) {
            a.balance += quantity;
        });
    }
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
