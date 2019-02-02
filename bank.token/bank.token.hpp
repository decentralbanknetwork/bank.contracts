/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

using namespace eosio;
using std::string;

class [[eosio::contract("bank.token")]] banktoken : public contract {
    using contract::contract;

    public:

        [[eosio::action]]
        void burn( name from,
                    asset        quantity,
                    string       memo );

        [[eosio::action]]
        void create( name issuer,
                    asset        maximum_supply);

        [[eosio::action]]
        void issue( name to, asset quantity, string memo );

        [[eosio::action]]
        void transfer( name from,
                        name to,
                        asset        quantity,
                        string       memo );

        inline asset get_supply( symbol_code sym )const;

        inline asset get_balance( name owner, symbol_code sym )const;

  private:
     struct [[eosio::table]] account {
        asset    balance;

        uint64_t primary_key()const { return balance.symbol.code().raw(); }
     };

     struct [[eosio::table]] currency_stats {
        asset          supply;
        asset          max_supply;
        name   issuer;

        uint64_t primary_key()const { return supply.symbol.code().raw(); }
     };

     typedef eosio::multi_index<"accounts"_n, account> accounts;
     typedef eosio::multi_index<"stat"_n, currency_stats> stats;

     void sub_balance( name owner, asset value );
     void add_balance( name owner, asset value, name ram_payer );

};

asset banktoken::get_supply( symbol_code sym )const
{
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );
    return st.supply;
}

asset banktoken::get_balance( name owner, symbol_code sym )const
{
    accounts accountstable( _self, owner.value );
    const auto& ac = accountstable.get( sym.raw() );
    return ac.balance;
}
