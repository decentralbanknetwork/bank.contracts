#include "bank.pay2key.hpp"
#include "base58.c"

std::string bytetohex(unsigned char *data, int len) {
    constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    std::string s(len * 2, ' ');
    for (int i = 0; i < len; ++i) {
        s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
        s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }
    return s;
}

[[eosio::action]]
void pay2key::create(name token_contract, symbol ticker) {
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

void pay2key::issue( name from, name to, asset quantity, string memo ) {
    if (from == _self) return; // sending tokens, ignore

    auto symbol = quantity.symbol;
    std::string issue_to = memo;
    validate_bitcoin_address(issue_to);
    eosio_assert(symbol.is_valid(), "invalid symbol name");
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

    // Cannot charge RAM to other accounts during notify
    // This is a major issue. How can we get the sender to pay for their own RAM?
    add_balance(st.chain_id, issue_to, quantity, _self);
}

[[eosio::action]]
void pay2key::transfer(
            uint64_t chain_id,
            name relayer_account,
            bitcoin_address relayer,
            bitcoin_address from,
            bitcoin_address to,
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
    eosio_assert(st.symbol == fee.symbol, "fee and chain_id symbols don't match. check decimal places");

    // validate addresses
    validate_bitcoin_address(to);
    validate_bitcoin_address(from);
    validate_bitcoin_address(relayer);
    eosio_assert(from.size() >= 26, "from address must be 26 chars min");
    eosio_assert(from.size() <= 34, "from address must be 34 chars max");
    eosio_assert(to.size() >= 26, "to address must be 26 chars min");
    eosio_assert(to.size() <= 34, "to address must be 34 chars max");
    eosio_assert(relayer.size() >= 26, "relayer address must be 26 chars min");
    eosio_assert(relayer.size() <= 34, "relayer address must be 34 chars max");

    // get last nonce
    accounts accounts_table(_self, chain_id);
    auto pk_index = accounts_table.get_index<name("bypk")>();
    auto account_it = pk_index.find(bitcoin_address_to_fixed_bytes(from));
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
    eosio_assert(memo.size() <= 159, "memo has more than 159 bytes");
    eosio_assert(nonce > last_nonce, "Nonce must be greater than last nonce. This transaction may already have been relayed.");
    eosio_assert(nonce < last_nonce + 100, "Nonce cannot jump by more than 100");

    // convert bitcoin addreses to binary
    bitcoin_bin to_bin = bitcoin_address_to_bin(to);
    bitcoin_bin from_bin = bitcoin_address_to_bin(from);

    // transaction format
    // bytes    field
    // 1        version
    // 1        length
    // 4        chain_id
    // 25       from_pubkey
    // 25       to_pubkey
    // 8        quantity
    // 8        fee
    // 8        nonce
    // 0-175    memo
    // tx meta fields
    uint8_t version = 0x03;
    uint8_t length = 80 + memo.size();
    uint32_t chain_id_32 = chain_id & 0xFFFFFFFF;

    // construct raw transaction
    uint8_t rawtx[length];
    rawtx[0] = version;
    rawtx[1] = length;
    memcpy(rawtx + 2, (uint8_t *)&chain_id, 4);
    memcpy(rawtx + 6, from_bin.data, 25);
    memcpy(rawtx + 31, to_bin.data, 25);
    memcpy(rawtx + 56, (uint8_t *)&quantity.amount, 8);
    memcpy(rawtx + 64, (uint8_t *)&fee.amount, 8);
    memcpy(rawtx + 72, (uint8_t *)&nonce, 8);
    memcpy(rawtx + 80, memo.c_str(), memo.size());

    // hash transaction
    checksum256 digest = sha256((const char *)rawtx, length);

    // verify signature
    // try both the compressed and uncompressed keys
    // if neither match, throw an error
    public_key compressed_key = recover_key(digest, sig);
    bitcoin_address compressed_address = public_key_to_bitcoin_address(compressed_key);
    bitcoin_address uncompressed_address;
    if (compressed_address != from) {
        uint8_t uncompressed_key[65];
        uncompressed_key[0] = 0x04; // identifier for uncompressed key
        uECC_decompress((uint8_t *)compressed_key.data.begin(), uncompressed_key + 1, uECC_secp256k1());
        uncompressed_address = public_key_to_bitcoin_address((const char *)uncompressed_key);
    }
    eosio_assert(compressed_address == from || uncompressed_address == from, "Recovered signature address was different than provided address. Either the wrong key or mismatched message information was used");

    // update last nonce
    pk_index.modify(account_it, _self, [&]( auto& n ){
        n.last_nonce = nonce;
    });

    // always subtract the quantity from the sender
    sub_balance(chain_id, from, quantity);

    // if the to address is the withdraw address, send an IQ transfer out
    // and update the circulating supply
    if (to == WITHDRAW_ADDRESS) {
        name withdraw_account = name(memo);
        action(
            permission_level{ _self , name("active") },
            st.token_contract , name("transfer"),
            std::make_tuple( _self, withdraw_account, quantity, std::string("withdraw EOS from UTXO"))
        ).send();

        auto st = statstable.find(chain_id);
        statstable.modify(st, _self, [&](auto& s) {
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

void pay2key::sub_balance(uint64_t chain_id, bitcoin_address sender, asset value) {
    accounts from_acts(_self, chain_id);

    auto accounts_index = from_acts.get_index<name("bypk")>();
    const auto& from = accounts_index.get(bitcoin_address_to_fixed_bytes(sender), "no public key object found");
    eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

    if (from.balance.amount == value.amount) {
        from_acts.erase(from);
    } else {
        from_acts.modify(from, _self, [&]( auto& a ) {
            a.balance -= value;
        });
    }
}

void pay2key::add_balance(uint64_t chain_id, bitcoin_address recipient, asset value, name ram_payer) {
    accounts to_acts(_self, chain_id);

    auto accounts_index = to_acts.get_index<name("bypk")>();
    auto to = accounts_index.find(bitcoin_address_to_fixed_bytes(recipient));

    if (to == accounts_index.end()) {
        to_acts.emplace(ram_payer, [&]( auto& a ){
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

void pay2key::validate_bitcoin_address(bitcoin_address address) {
    eosio_assert(address.size() >= 26, "address must be 26 chars min");
    eosio_assert(address.size() <= 34, "address must be 34 chars max");
    bitcoin_bin address_bin = bitcoin_address_to_bin(address);
    char address_base[21];
    memcpy(address_base, address_bin.data, 21);
    checksum256 check_mid = sha256((const char *)&address_base, 21);
    checksum256 fullcheck = sha256((const char *)check_mid.extract_as_byte_array().begin(), 32);
    char checksum[4];
    memcpy(checksum, fullcheck.extract_as_byte_array().begin(), 4);
    for (int i=0; i < 4; i++) {
        eosio_assert(checksum[i] == address_bin.data[21 + i], "bitcoin address checksum does not validate");
    }
}

bitcoin_bin pay2key::bitcoin_address_to_bin(bitcoin_address address) {
    bitcoin_bin bin;
    size_t binlen = 25;
    b58tobin((void *)bin.data, &binlen, address.c_str());
    return bin;
}

// convert compressed public key to bitcoin address
bitcoin_address pay2key::public_key_to_bitcoin_address (public_key key) {
    checksum256 hashkey1 = sha256(key.data.begin(), 33);
    checksum160 hashkey2 = ripemd160((const char *)hashkey1.extract_as_byte_array().begin(), 32);
    return hashed_key_to_bitcoin_address(hashkey2);
}

// convert uncompressed public key to bitcoin address
bitcoin_address pay2key::public_key_to_bitcoin_address (const char * key) {
    checksum256 hashkey1 = sha256(key, 65);
    checksum160 hashkey2 = ripemd160((const char *)hashkey1.extract_as_byte_array().begin(), 32);
    return hashed_key_to_bitcoin_address(hashkey2);
}

bitcoin_address pay2key::hashed_key_to_bitcoin_address (checksum160 hashed) {
    // first byte is 0x00
    // next 20 bytes are ripemd160 hash
    bitcoin_bin address_bin;
    address_bin.data[0] = 0;
    memcpy(address_bin.data + 1, hashed.extract_as_byte_array().begin(), 20);

    // last 4 bytes are checksum
    checksum256 check_mid = sha256((const char *)&address_bin.data, 21);
    checksum256 fullcheck = sha256((const char *)check_mid.extract_as_byte_array().begin(), 32);
    memcpy(address_bin.data + 21, fullcheck.extract_as_byte_array().begin(), 4);

    // base58 encode to get the address
    size_t num_chars = 35;
    char address_chars[num_chars];
    b58enc(address_chars, &num_chars, (const uint8_t *)address_bin.data, 25);
    address_chars[num_chars] = '\0';
    bitcoin_address address = std::string(address_chars);

    return address;
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
