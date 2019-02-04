/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "bank.token.hpp"

// Create the currency token
[[eosio::action]]
void banktoken::create( name issuer, asset maximum_supply ){
	// Auth check
	require_auth( _self );

	// Validate the token you want to create
    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

	// Make sure the token doesn't already exist
    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

	// Create the entry for the token
    statstable.emplace( _self, [&]( auto& s ) {
		s.supply.symbol = maximum_supply.symbol;
		s.max_supply    = maximum_supply;
		s.issuer        = issuer;
    });
}

// Issue brand new tokens to a specified account
[[eosio::action]]
void banktoken::issue( name to, asset quantity, string memo ) {
	// Make sure the asset is valid
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	// Make sure the token exists
    auto sym_name = sym.code().raw();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

	// Auth check
    require_auth( st.issuer );

	// Make sure the quantity is valid
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

	// Update the total quantity of tokens
    statstable.modify( st, same_payer, [&]( auto& s ) {
        s.supply += quantity;
    });

	// Update the balance table 
    add_balance( st.issuer, quantity, st.issuer );

    // Use the safesend temporarily until RAM stealing exploit is fixed.
    auto n = name{to};
    std::string tempmemo = n.to_string();
    if( to != st.issuer ) {
        SEND_INLINE_ACTION( *this, transfer, {st.issuer, "active"_n}, {st.issuer, "bank.safesnd"_n, quantity, tempmemo} );
    }
}

// Transfer the token
[[eosio::action]]
void banktoken::transfer( name from, name to, asset quantity, string memo ) {
	// Validate the sender and recipient
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");

	// Fetch the token stats
    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

	// Notify the sender and recipient
    require_recipient( from );
    require_recipient( to );

	// Validate the quantity
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

	// Validate the memo
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	// Perform the transfer
    sub_balance( from, quantity );
    add_balance( to, quantity, from );
}

// Burn / delete some tokens
[[eosio::action]]
void banktoken::burn( name from, asset quantity, string memo ) {
	// Auth check
	require_auth( from );

	// Validate the symbol
	auto sym = quantity.symbol;
	eosio_assert( sym.is_valid(), "invalid symbol name" );

	// Validate the memo
	eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	// Make sure the token actually exists
	auto sym_name = sym.code().raw();
	stats statstable( _self, sym_name );
	auto existing = statstable.find( sym_name );
	eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
	const auto& st = *existing;

	// Validate the quantity
	eosio_assert( quantity.is_valid(), "invalid quantity" );
	eosio_assert( quantity.amount > 0, "must burn positive quantity" );
	eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

	// Adjust the balance to reflect the burn
	sub_balance( from, quantity );

	// Reduce the total supply
	statstable.modify( st, same_payer, [&]( auto& s ) {
		s.supply -= quantity;
	});
}

// Internal function to subtract some tokens
void banktoken::sub_balance( name owner, asset value ) {
	// Fetch the account
    accounts from_acnts( _self, owner.value );

	// Make sure the owner has a balance
    const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );

	// Make sure that the owner will not overdraw
    eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

	// Modify the balance, or delete the entry if it is now zero
    if( from.balance.amount == value.amount ) {
       from_acnts.erase( from );
    } else {
       from_acnts.modify( from, owner, [&]( auto& a ) {
           a.balance -= value;
       });
    }
}

// Internal function to add some tokens
void banktoken::add_balance( name owner, asset value, name ram_payer ) {
	// Fetch the account and the balance entry
    accounts to_acnts( _self, owner.value );
    auto to = to_acnts.find( value.symbol.code().raw() );

	// Modify the balance, or create an entry if it doesn't exist
    if( to == to_acnts.end() ) {
       to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
       });
    } else {
       to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
       });
    }
}

EOSIO_DISPATCH( bank.token, (burn)(create)(issue)(transfer) )