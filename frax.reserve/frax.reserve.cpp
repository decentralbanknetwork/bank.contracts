#include "frax.reserve.hpp"

[[eosio::action]]
void fraxreserve::addtoken(name contract, symbol ticker) {
    require_auth( _self );

    check(ticker.is_valid(), "invalid symbol name");

    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(contract, ticker);
    auto existing = contract_sym_index.find(merged);
    check(existing == contract_sym_index.end(), "Token is already registered");
    check(statstable.find(ticker.raw()) == statstable.end(), "Another token with that ticker already exists");

    statstable.emplace(_self, [&](auto& s) {
        s.contract = contract;
        s.supply = asset(0, ticker);
    });
}

void fraxreserve::deposit( name from, name to, asset quantity, string memo ) {
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

[[eosio::action]]
void fraxreserve::buyfrax(name buyer, asset frax) {
    require_auth(buyer);

    check(frax.amount > 0, "Must buy a positive amount");
    check(frax.symbol == FRAX_SYMBOL, "Can only buy FRAX");

    sysparams paramstbl( _self, _self.value);
    check(paramstbl.begin() != paramstbl.end(), "Reserve params must be initialized first");
    auto param_it = paramstbl.begin();

    stats statstable(_self, _self.value);
    auto fxs_stats = statstable.find(FXS_SYMBOL.raw());
    auto usdt_stats = statstable.find(USDT_SYMBOL.raw());
    check(fxs_stats != statstable.end(), "Must addtoken FXS first");
    check(usdt_stats != statstable.end(), "Must addtoken USDT first");

    asset needed_usdt = param_it->target_usdt - usdt_stats->supply;
    asset needed_fxs = param_it->target_fxs - fxs_stats->supply;
    uint64_t needed_fxs_value = needed_fxs.amount * param_it->fxs_price / 1e4;
    uint64_t total_needed_value = needed_usdt.amount + needed_fxs_value;

    asset sell_usdt = asset(frax.amount * needed_usdt.amount / total_needed_value, USDT_SYMBOL);
    asset sell_fxs = asset(frax.amount * needed_fxs_value / total_needed_value, FXS_SYMBOL);
    check(sell_usdt <= needed_usdt, "Attempting to buy too much FRAX");
    check(sell_fxs <= needed_fxs, "Attempting to buy too much FRAX");

    // Update accounts and stats
    accounts deptbl( _self, buyer.value );
    if (sell_usdt.amount > 0) {
        auto usdt_account = deptbl.find(USDT_SYMBOL.raw());
        check(usdt_account != deptbl.end(), "No USDT balance for this user");
        check(usdt_account->balance >= sell_usdt, "Not enough USDT balance to buy FRAX");
        deptbl.modify( usdt_account, _self, [&](auto& a) {
            a.balance -= sell_usdt;
        });
        statstable.modify( usdt_stats, same_payer, [&](auto& usdt) {
            usdt.supply += sell_usdt;
        });
    }
    if (sell_fxs.amount > 0) {
        auto fxs_account = deptbl.find(FXS_SYMBOL.raw());
        check(fxs_account != deptbl.end(), "No FXS balance for this user");
        check(fxs_account->balance >= sell_fxs, "Not enough FXS balance to buy FRAX");
        deptbl.modify( fxs_account, _self, [&](auto& a) {
            a.balance -= sell_fxs;
        });
        statstable.modify( fxs_stats, same_payer, [&](auto& fxs) {
            fxs.supply += sell_fxs;
        });
    }

    // Issue FRAX
    action(
        permission_level { _self, name("active") },
        name("fraxtokenfxs"), name("issue"),
        std::make_tuple( buyer, frax, std::string("issue FRAX") )
    ).send();

}

[[eosio::action]]
void fraxreserve::settarget(asset target_usdt, asset target_fxs, uint64_t fxs_price) {
    require_auth( _self );

    check(target_usdt.symbol == USDT_SYMBOL, "Symbol mismatch for target_usdt");
    check(target_fxs.symbol == FXS_SYMBOL, "Symbol mismatch for target_usdt");
    check(fxs_price > 0, "fxs_price must be postive");

    sysparams paramstbl( _self, _self.value);
    if (paramstbl.begin() == paramstbl.end()) {
        paramstbl.emplace( _self, [&](auto& p) {
            p.target_usdt = target_usdt;
            p.target_fxs = target_fxs;
            p.fxs_price = fxs_price;
        });
    }
    else {
        paramstbl.modify( paramstbl.begin(), _self, [&](auto& p) {
            p.target_usdt = target_usdt;
            p.target_fxs = target_fxs;
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
    else if (code == _self)
    {
        switch (action) {
            EOSIO_DISPATCH_HELPER( fraxreserve, (addtoken)(buyfrax)(settarget) )
        }
    }
}
