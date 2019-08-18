#include "frax.reserve.hpp"

[[eosio::action]]
void fraxloans::addtoken(name contract, symbol ticker) {
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
        s.available = asset(0, ticker);
        s.loaned = asset(0, ticker);
        s.price = asset(0, USDT_SYMBOL);
    });
}

void fraxloans::deposit( name from, name to, asset quantity, string memo ) {
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
    accounts deptbl( _self, from.value );
    auto account_it = deptbl.find(symbol.raw());
    if (account_it == deptbl.end()) {
        deptbl.emplace( _self, [&](auto& a) {
            a.balance = quantity;
        });
    }
    else {
        deptbl.modify( account_it, _self, [&](auto& a) {
            a.balance += quantity;
        });
    }

}

void fraxloans::borrow(name borrower, asset quantity) {
    require_auth(borrower);

    eosio_assert(quantity.amount > 0, "Must borrow positive amount");

    // Check ticker is borrowable
    stats statstbl( _self, _self );
    auto stat_it = statstbl.get(quantity.symbol.raw(), "Token is not supported");
    eosio_assert(stat_it->available > quantity, "Not enough supply available for requested quantity");

    // Update supply
    statstbl.modify( stat_it, _self, [&](auto& s) {
        s.available -= quantity;
        s.loaned += quantity;
    });

    // Update borrower account
    accounts acctstbl( _self, borrower.value );
    auto account_it = acctstbl.find(quantity.symbol)
    if (account_it == accstbl.end()) {
        acctstbl.emplace( _self, [&](auto& a) {
            a.balance = quantity;
        });
    }
    else {
        acctstbl.modify( account_it, _self, [&](auto& a) {
            a.balance += quantity;
            a.borrowing += quantity;
        });
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    auto _self = receiver;
    if (code != _self && action == name("transfer").value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &fraxloans::deposit
        );
    }
    else if (code == _self)
    {
        switch (action) {
            EOSIO_DISPATCH_HELPER( fraxloans, (addtoken) )
        }
    }
}
