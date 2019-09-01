#include "bank.pay2key.hpp"
#include "base58.c"

[[eosio::action]]
void pay2key::create(name token_contract, symbol ticker) {
    require_auth( _self );

    check(ticker.is_valid(), "invalid symbol name");

    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(token_contract, ticker);
    auto existing = contract_sym_index.find(merged);
    check(existing == contract_sym_index.end(), "Token is already registered");

    asset supply = asset(0, ticker);

    statstable.emplace(_self, [&](auto& s) {
        s.chain_id = statstable.available_primary_key();
        s.symbol = ticker;
        s.token_contract = token_contract;
        s.supply = supply;
    });
}

void pay2key::issue( name from, name to, asset quantity, string memo ) {
    if (from == _self) return; // sending tokens, ignore

    auto symbol = quantity.symbol;
    check(symbol.is_valid(), "invalid symbol name");
    check(memo.size() == 53, "memo must be a 53-char EOS public key");
    check(memo.substr(0,3) == "EOS", "public key must start with EOS");
    check(to == _self, "stop trying to hack the contract");

    // make sure contract and symbol are accepted by contract
    stats statstable(_self, _self.value);
    auto contract_sym_index = statstable.get_index<name("byctrsym")>();
    uint128_t merged = merge_contract_symbol(get_first_receiver(), symbol);
    auto existing = contract_sym_index.find(merged);
    check(existing != contract_sym_index.end(), "Token is not yet supported. You must assign it a chain_id with `create` first");
    const auto& st = *existing;

    // validate arguments
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must issue positive quantity");

    // TODO: ensure token balance is enough to cover UTXO issuance
    // This will always be true unless the contract has been hacked
    // so it is mostly a sanity check

    statstable.modify(st, same_payer, [&](auto& s) {
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
        check(checksum_bytes[i] == (uint8_t)issue_to.data[33+i], "invalid public key in memo: checksum does not validate");

    // Cannot charge RAM to other accounts during notify
    // This is a major issue. How can we get the sender to pay for their own RAM?
    add_balance(st.chain_id, issue_to, quantity, _self);
}

[[eosio::action]]
void pay2key::transfer(
            uint64_t chain_id,
            name relayer_account,
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
    check(st.symbol == quantity.symbol, "quantity and chain_id symbols don't match. check decimal places");
    check(st.symbol == fee.symbol, "fee and chain_id symbols don't match. check decimal places");

    // get last nonce
    accounts accounts_table(_self, chain_id);
    auto pk_index = accounts_table.get_index<name("bypk")>();
    auto account_it = pk_index.find(public_key_to_fixed_bytes(from));
    uint64_t last_nonce = 0;
    if (account_it != pk_index.end())
        last_nonce = account_it->last_nonce;

    // validate inputs
    check(from != to, "cannot transfer to self");
    check(quantity.is_valid(), "invalid quantity" );
    check(fee.is_valid(), "invalid quantity" );
    check(quantity.amount > 0, "must transfer positive quantity");
    check(fee.amount >= 0, "fee must be non-negative");
    check(quantity.amount <= account_it->balance.amount, "user has insufficient balance");
    check(quantity.amount + fee.amount <= account_it->balance.amount, "user has insufficient balance to cover fee");
    check(memo.size() <= 159, "memo has more than 159 bytes");
    check(nonce > last_nonce, "Nonce must be greater than last nonce. This transaction may already have been relayed.");
    check(nonce < last_nonce + 100, "Nonce cannot jump by more than 100");

    // transaction format
    // bytes    field
    // 1        version
    // 1        length
    // 4        chain_id
    // 33       from_pubkey
    // 33       to_pubkey
    // 8        quantity
    // 8        fee
    // 8        nonce
    // 0-159    memo
    // tx meta fields
    uint8_t version = 0x02;
    uint8_t length = 96 + memo.size();
    uint32_t chain_id_32 = chain_id & 0xFFFFFFFF;

    // construct raw transaction
    uint8_t rawtx[length];
    rawtx[0] = version;
    rawtx[1] = length;
    memcpy(rawtx + 2, (uint8_t *)&chain_id, 4);
    memcpy(rawtx + 6, from.data.data(), 33);
    memcpy(rawtx + 39, to.data.data(), 33);
    memcpy(rawtx + 72, (uint8_t *)&quantity.amount, 8);
    memcpy(rawtx + 80, (uint8_t *)&fee.amount, 8);
    memcpy(rawtx + 88, (uint8_t *)&nonce, 8);
    memcpy(rawtx + 96, memo.c_str(), memo.size());

    // hash transaction
    checksum256 digest = sha256((const char *)rawtx, length);

    // verify signature
    assert_recover_key(digest, sig, from);

    // update last nonce
    pk_index.modify(account_it, same_payer, [&]( auto& n ){
        n.last_nonce = nonce;
    });

    // always subtract the quantity from the sender
    sub_balance(chain_id, from, quantity);

    // Create the public_key object for the WITHDRAW_ADDRESS
    public_key withdraw_key = to;
    memcpy(withdraw_key.data.data(), WITHDRAW_KEY_BYTES, 33);

    // if the to address is the withdraw address, send an IQ transfer out
    // and update the circulating supply
    if (to == withdraw_key) {
        name withdraw_account;
        std::string withdraw_memo;
        uint64_t colon_index = memo.find(":");
        if (colon_index == std::string::npos) {
            withdraw_account = name(memo);
            withdraw_memo = std::string("");
        }
        else {
            withdraw_account = name(memo.substr(0,colon_index));
            if (colon_index == memo.size() - 1) // edge case where there's nothing after the colon
                withdraw_memo = "";
            else
                withdraw_memo = memo.substr(colon_index + 1);
        }

        action(
            permission_level{ _self , name("active") },
            st.token_contract , name("transfer"),
            std::make_tuple( _self, withdraw_account, quantity, withdraw_memo)
        ).send();

        auto st = statstable.find(chain_id);
        statstable.modify(st, same_payer, [&](auto& s) {
           s.supply -= quantity;
        });
    }
    // add the balance if it's not a withdrawal
    else {
        add_balance(chain_id, to, quantity, relayer_account);
    }

    // update balances with fees
    if (fee.amount > 0) {
        sub_balance(chain_id, from, fee);
        add_balance(chain_id, relayer, fee, relayer_account);
    }

}

void pay2key::sub_balance(uint64_t chain_id, public_key sender, asset value) {
    accounts from_acts(_self, chain_id);

    auto accounts_index = from_acts.get_index<name("bypk")>();
    const auto& from = accounts_index.get(public_key_to_fixed_bytes(sender), "no public key object found");
    check(from.balance.amount >= value.amount, "overdrawn balance");

    if (from.balance.amount == value.amount) {
        from_acts.erase(from);
    } else {
        from_acts.modify(from, same_payer, [&]( auto& a ) {
            a.balance -= value;
        });
    }
}

void pay2key::add_balance(uint64_t chain_id, public_key recipient, asset value, name ram_payer) {
    accounts to_acts(_self, chain_id);

    auto accounts_index = to_acts.get_index<name("bypk")>();
    auto to = accounts_index.find(public_key_to_fixed_bytes(recipient));

    if (to == accounts_index.end()) {
        to_acts.emplace(ram_payer, [&]( auto& a ){
            a.key = to_acts.available_primary_key();
            a.balance = value;
            a.publickey = recipient;
            a.last_nonce = 0;
        });
    } else {
        accounts_index.modify(to, same_payer, [&]( auto& a ) {
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
