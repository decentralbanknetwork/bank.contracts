#include "bank.pay2key.hpp"
#include "base58.c"

[[eosio::action]]
void pay2key::create(name token_contract, symbol ticker, uint64_t chain_id) {
    require_auth( _self );

    eosio_assert(ticker.is_valid(), "invalid symbol name");

    stats statstable(_self, chain_id);
    eosio_assert(statstable.begin() == statstable.end(), "chain_id already in use");

    asset supply = asset(0, ticker);

    statstable.emplace(_self, [&](auto& s) {
        s.symbol = ticker;
        s.token_contract = token_contract;
        s.supply = supply;
    });
}

void pay2key::issue( name from, name to, asset quantity, string memo ) {
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
    eosio_assert(existing != contract_sym_index.end(), "Token is not yet supported. You must assign it a chain_id with `create` first");
    const auto& st = *existing;

    // validate arguments
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must issue positive quantity");

    // TODO: ensure EOS balance is enough to cover UTXO issuance
    // This will always be true unless the contract has been hacked
    // so it is mostly a sanity check

    statstable.modify(st, _self, [&](auto& s) {
       s.supply += quantity;
    });

    // convert public key memo to public_key object
    public_key issue_to;
    size_t issue_to_len = 37;
    b58tobin((void *)issue_to.data.data(), &issue_to_len, memo.substr(3).c_str());

    // validate the checksum
    public_key issue_to_copy = issue_to;
    checksum160 checksum = ripemd160((const char *)issue_to_copy.data.begin(), 33);
    std::array<uint8_t,20> checksum_bytes = checksum.extract_as_byte_array();
    for (int i=0; i<4; i++)
        eosio_assert(checksum_bytes[i] == (uint8_t)issue_to.data[33+i], "invalid public key in memo: checksum does not validate");

    add_balance(issue_to, quantity);
}

[[eosio::action]]
void pay2key::transfer(
            uint64_t chain_id,
            public_key relayer,
            public_key from,
            public_key to,
            asset quantity,
            asset fee,
            uint64_t nonce,
            string memo,
            signature sig) {

    // get currency information
    stats statstable(_self, _self.value);
    const auto st = statstable.get(chain_id, "no token found for chain_id. chain_id must be created first");

    // verify symbol matches currency information
    eosio_assert(st.symbol == quantity.symbol, "quantity and chain_id symbols don't match. check decimal places");
    eosio_assert(st.symbol == fee.symbol, "quantity and chain_id symbols don't match. check decimal places");

    // get last nonce
    accounts accounts_table(_self, _self.value);
    auto pk_index = accounts_table.get_index<name("bypk")>();
    auto account_it = pk_index.find(public_key_to_fixed_bytes(from));
    uint64_t last_nonce = 0;
    if (account_it != pk_index.end())
        last_nonce = account_it->last_nonce;

    // validate inputs
    eosio_assert(from != to, "cannot transfer to self");
    eosio_assert(quantity.is_valid(), "invalid quantity" );
    eosio_assert(fee.is_valid(), "invalid quantity" );
    eosio_assert(quantity.amount > 0, "must transfer positive quantity");
    eosio_assert(fee.amount >= 0, "fee must be non-negative");
    eosio_assert(quantity.amount <= account_it->balance.amount, "user has insufficient balance");
    eosio_assert(quantity.amount + fee.amount <= account_it->balance.amount, "user has insufficient balance to cover fee");
    eosio_assert(memo.size() <= 163, "memo has more than 164 bytes");
    eosio_assert(nonce > last_nonce, "Nonce must be greater than last nonce. This transaction may already have been relayed.");
    eosio_assert(nonce < last_nonce + 100, "Nonce cannot jump by more than 100");

    // tx meta fields
    uint8_t version = 0x01;
    uint8_t length = 92 + memo.size();

    // construct raw transaction
    uint8_t rawtx[length];
    rawtx[0] = version;
    rawtx[1] = length;
    memcpy(rawtx + 2, from.data.data(), 33);
    memcpy(rawtx + 35, to.data.data(), 33);
    memcpy(rawtx + 68, (uint8_t *)&quantity.amount, 8);
    memcpy(rawtx + 76, (uint8_t *)&fee.amount, 8);
    memcpy(rawtx + 84, (uint8_t *)&nonce, 8);
    memcpy(rawtx + 92, memo.c_str(), memo.size());

    // hash transaction
    checksum256 digest = sha256((const char *)rawtx, length);

    // verify signature
    assert_recover_key(digest, sig, from);

    // update last nonce
    pk_index.modify(account_it, _self, [&]( auto& n ){
        n.last_nonce = nonce;
    });

    // always subtract the quantity from the sender
    sub_balance(from, quantity);

    // Create the public_key object for the WITHDRAW_ADDRESS
    public_key withdraw_key = to;
    memcpy(withdraw_key.data.data(), WITHDRAW_KEY_BYTES, 33);

    // if the to address is the withdraw address, send an IQ transfer out
    // and update the circulating supply
    if (to == withdraw_key) {
        asset eos_quantity = quantity;
        eos_quantity.symbol = EOS_SYMBOL;
        name withdraw_account = name(memo);
        action(
            permission_level{ _self , name("active") },
            name("everipediaiq") , name("transfer"),
            std::make_tuple( _self, withdraw_account, eos_quantity, std::string("withdraw EOS from UTXO"))
        ).send();

        stats statstable(_self, quantity.symbol.raw());
        auto& supply_it = statstable.get(quantity.symbol.raw(), "UTXO symbol is missing. Create it first");
        statstable.modify(supply_it, _self, [&](auto& s) {
           s.supply -= quantity;
        });
    }
    // add the balance if it's not a withdrawal
    else {
        add_balance(to, quantity);
    }

    // update balances with fees
    if (fee.amount > 0) {
        sub_balance(from, fee);
        add_balance(relayer, fee);
    }

}

void pay2key::sub_balance(public_key sender, asset value) {
    accounts from_acts(_self, _self.value);

    auto accounts_index = from_acts.get_index<name("bypk")>();
    const auto& from = accounts_index.get(public_key_to_fixed_bytes(sender), "no public key object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    if (from.balance.amount == value.amount) {
        from_acts.erase(from);
    } else {
        from_acts.modify(from, _self, [&]( auto& a ) {
            a.balance -= value;
        });
    }
}

void pay2key::add_balance(public_key recipient, asset value) {
    accounts to_acts(_self, _self.value);

    auto accounts_index = to_acts.get_index<name("bypk")>();
    auto to = accounts_index.find(public_key_to_fixed_bytes(recipient));

    if (to == accounts_index.end()) {
        to_acts.emplace(_self, [&]( auto& a ){
            a.key = to_acts.available_primary_key();
            a.balance = value;
            a.publickey = recipient;
            a.last_nonce = 0;
        });
    } else {
        accounts_index.modify(to, _self, [&]( auto& a ) {
            a.balance += value;
        });
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    auto _self = receiver;
    if (code != _self && action == name("transfer").value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &pay2key::issue
        );
    }
    else if (code == _self && action == name("create").value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &pay2key::create
        );
    }
    else if (code == _self && action == name("transfer").value)
    {
        eosio::execute_action(
            eosio::name(receiver), eosio::name(code), &pay2key::transfer
        );
    }
}
