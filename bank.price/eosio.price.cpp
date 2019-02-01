#include "eosio.price.hpp"

void bankprice::create(asset initprice) {
    require_auth( _self );

    auto sym = initprice.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( initprice.is_valid(), "invalid initprice");
    eosio_assert( initprice.amount > 0, "initprice must be positive");

    prices_table pricetable( _self, _self.value );
    auto existing = pricetable.find( sym.code().raw() );
    eosio_assert( existing == pricetable.end(), "price with symbol already exists" );
    
    pricetable.emplace( _self, [&]( auto& p ) {
       p.price = initprice;
    });
}

void bankprice::update(name updater, asset price) {
    require_auth(updater);

    // validate inputs
    eosio_assert( price.is_valid(), "invalid price" );
    eosio_assert( price.amount > 0, "must update with positive price" );

    // get producer vote info
    eosiosystem::producers_table producers(name("eosio"), name("eosio").value);
    auto votes_idx = producers.get_index<name("prototalvote")>();
    auto vote_it = votes_idx.end();
    
    // make sure producer is in top 21
    uint8_t loop = 0;
    bool isTop21 = false;
    while(vote_it != votes_idx.begin() && loop < 21) {
        if (vote_it->owner == updater) {
            isTop21 = true;
            break;
        }
    }
    eosio_assert(isTop21, "Producer must be in top 21");

    // get price info
    prices_table pricetable( _self, _self.value );
    auto current_price = pricetable.get(price.symbol.code().raw(), "Symbol does not exist. Create it first");
    eosio_assert( current_price.price.symbol == price.symbol, "Symbol precision mismatch" );

    // calculate updated price info
    uint64_t new_price = current_price.price.amount * 9 / 10 + price.amount / 10;

    // update table
    pricetable.modify( current_price, _self, [&]( auto& p ) {
       p.price.amount = new_price;
    });
    
}
EOSIO_DISPATCH( bankprice, (create)(update))
