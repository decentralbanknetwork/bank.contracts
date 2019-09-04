#include "frax.loans.hpp"

[[eosio::action]]
void fraxloans::addtoken(name contract, symbol ticker) {
    require_auth( _self );

    check(ticker.is_valid(), "invalid symbol name");
    check(ticker.precision() == 4, "to simplify price calculations, only tokens with 4 decimal places are currently supported");

    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(contract, ticker);
    auto existing = contract_sym_index.find(merged);
    check(existing == contract_sym_index.end(), "Token is already registered");
    check(statstable.find(ticker.raw()) == statstable.end(), "Another token with that ticker already exists");

    statstable.emplace(_self, [&](auto& s) {
        s.contract = contract;
        s.available = asset(0, ticker);
        s.loaned = asset(0, ticker);
        s.price = asset(0, USDT_SYMBOL);
        s.allowed_as_collateral = true;
        s.can_deposit = true;
    });
}

void fraxloans::deposit( name from, name to, asset quantity, string memo ) {
    if (from == _self) return; // sending tokens, ignore

    auto symbol = quantity.symbol;
    check(symbol.is_valid(), "invalid symbol name");
    check(to == _self, "stop trying to hack the contract");

    // make sure contract and symbol are accepted by contract
    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(get_first_receiver(), symbol);
    auto existing = contract_sym_index.find(merged);
    check(existing != contract_sym_index.end(), "Token is not yet supported");

    // validate arguments
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount < 1e10, "quantity is suspiciously high. send a smaller amount");
    check(quantity.amount > 0, "must deposit positive quantity");
    check(memo.size() < 256, "memo must be less than 256 bytes");

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

    check(quantity.amount > 0, "Must borrow positive amount");

    // Check ticker is borrowable
    stats statstbl( _self, _self.value );
    auto stat_it = statstbl.get(quantity.symbol.raw(), "Token is not supported");
    check(stat_it.available > quantity, "Not enough supply available for requested quantity");

    // Check if sufficient collateral is present to back loan
    // NOTE: Both sum_collateral and sum_borrow are in units 1e-8 USDT
    accounts acctstbl( _self, borrower.value );
    auto account_it = acctstbl.begin();
    uint64_t sum_collateral = 0;
    while (account_it != acctstbl.end()) {
        auto token_price_it = statstbl.get(account_it->balance.symbol.raw(), "Token is not supported");
        if (!token_price_it.allowed_as_collateral) continue;
        sum_collateral += account_it->balance.amount * token_price_it.price.amount;
        account_it++;
    }
    uint64_t sum_borrow = quantity.amount * stat_it.price.amount;
    while (account_it != acctstbl.end()) {
        auto token_price_it = statstbl.get(account_it->balance.symbol.raw(), "Token is not supported");
        sum_borrow += account_it->borrowing.amount * token_price_it.price.amount;
        account_it++;
    }
    check((static_cast<int64_t>(sum_collateral * COLLATERAL_RATIO) > sum_borrow), "Insufficient collateral to back loan");
    
    // Update supply
    statstbl.modify( stat_it, _self, [&](auto& s) {
        s.available -= quantity;
        s.loaned += quantity;
    });


    // Update borrower account
    account_it = acctstbl.find(quantity.symbol.raw());
    if (account_it == acctstbl.end()) {
        acctstbl.emplace( _self, [&](auto& a) {
            a.balance = quantity;
            a.borrowing = quantity;
        });
    }
    else {
        acctstbl.modify( account_it, _self, [&](auto& a) {
            a.balance += quantity;
            a.borrowing += quantity;
        });
    }
}

[[eosio::action]]
void liquidate(name user, name executor) {
    // Check if sufficient collateral is present to back loan
    // NOTE: Both sum_collateral and sum_borrow are in units 1e-8 USDT
    //accounts acctstbl( _self, user.value );
    //auto account_it = acctstbl.begin();
    //uint64_t sum_collateral = 0;
    //while (account_it != acctstbl.end()) {
    //    auto token_price_it = statstbl.get(account_it->balance.symbol.raw(), "Token is not supported");
    //    if (!token_price_it->allowed_as_collateral) continue;
    //    sum_collateral += account_it->balance.amount * token_price_it->price.amount;
    //    account_it++;
    //}
    //uint64_t sum_borrow = quantity.amount * stat_it->price.amount;
    //while (account_it != acctstbl.end()) {
    //    auto token_price_it = statstbl.get(account_it->balance.symbol.raw(), "Token is not supported");
    //    sum_borrow += account_it->borrowing.amount * token_price_it->price.amount;
    //    account_it++;
    //}
    //check((static_cast<int64_t>(sum_collateral * COLLATERAL_RATIO) < sum_borrow), "Loans are sufficiently backed");

    //// TODO: Send to auction contract
    
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
