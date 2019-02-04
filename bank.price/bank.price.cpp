#include "bank.price.hpp"

// Create a new price to track
[[eosio::action]]
void bankprice::create(asset initprice) {
    // Auth check
    require_auth( _self );

    // Validate the initprice asset
    auto sym = initprice.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( initprice.is_valid(), "invalid initprice");
    eosio_assert( initprice.amount > 0, "initprice must be positive");

    // Make sure the symbol is not already being tracked
    prices_table pricetable( _self, _self.value );
    auto existing = pricetable.find( sym.code().raw() );
    eosio_assert( existing == pricetable.end(), "price with symbol already exists" );
    
    // Create an entry for the symbol
    pricetable.emplace( _self, [&]( auto& p ) {
       p.price = initprice;
    });
}

// Update the price for a symbol
[[eosio::action]]
void bankprice::update(name updater, asset price) {
    // Auth check
    require_auth(updater);

    // Validate inputs
    eosio_assert( price.is_valid(), "invalid price" );
    eosio_assert( price.amount > 0, "must update with positive price" );

    // Get producer vote info
    eosiosystem::producers_table producers(name("eosio"), name("eosio").value);
    auto votes_idx = producers.get_index<name("prototalvote")>();
    auto vote_it = votes_idx.end();
    
    // Make sure producer is in top 21
    uint8_t loop = 0;
    bool isTop21 = false;
    while(vote_it != votes_idx.begin() && loop < 21) {
        if (vote_it->owner == updater) {
            isTop21 = true;
            break;
        }
    }
    eosio_assert(isTop21, "Producer must be in top 21");

    // Get price info
    prices_table pricetable( _self, _self.value );
    auto current_price = pricetable.get(price.symbol.code().raw(), "Symbol does not exist. Create it first");
    eosio_assert( current_price.price.symbol == price.symbol, "Symbol precision mismatch" );

    // Calculate updated price info
    uint64_t new_price = current_price.price.amount * 9 / 10 + price.amount / 10;

    // Update table
    pricetable.modify( current_price, _self, [&]( auto& p ) {
       p.price.amount = new_price;
    });
    
}
EOSIO_DISPATCH( bankprice, (create)(update))
