/*
 * @ author
 *   richard tiutiun 
 * @ copyright 
 *   defined in ../README.md 
 * @ file
 *   contract implementation for CDP engine
 *   design/features explained in ./README.md
 */

#include "bank.cdp.hpp"

using namespace eosio;      

// ACTION bank.cdp::give( name giver, name taker, symbol_code symbl )
// {  require_auth( giver ); 
//    is_account( taker );
//    eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   
//    stats stable( _self, _self.value );
//    const auto& st = stable.get( symbl.raw(), 
//                                     "CDP type does not exist" 
//                                   );
//    cdps cdpstable( _self, symbl.raw() );
//    const auto& git = cdpstable.get( giver.value, 
//                                    "(CDP type, owner) mismatch" 
//                                   );  
//    eosio_assert( git.live, "CDP is in liquidation" );

//    auto tit = cdpstable.find( taker.value );
//    eosio_assert( tit == cdpstable.end(), 
//                  "taker already has a cdp of this type" 
//                );
//    cdpstable.emplace( giver, [&]( auto& p ) { 
//       p.owner = taker;
//       p.created = git.created;
//       p.collateral = git.collateral;
//       p.stablecoin = git.stablecoin;
//    });
//    cdpstable.erase( git );
// }

// Start a new CDP
ACTION bank.cdp::open( name owner, symbol_code symbl, name ram_payer )
{  
   // Make sure the RAM payer is authenticated
   require_auth( ram_payer );

   // Check for a valid symbol
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );

   // Get the stats for the stablecoin
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );

   // Make sure the CDP is live and not in global settlement
   eosio_assert( st.live, "CDP type not yet live, or in global settlement" );

   // Make sure the price feed exists and has fresh data
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( st.total_collateral.quantity.symbol.code().raw(), "collateral Feed does not exist" );
   eosio_assert( fc.stamp >= now() - FEED_FRESH, "Collateral price feed data too stale" );

   // If the owner already has a CDP, they cannot create another one
   cdps cdpstable( _self, symbl.raw() );
   eosio_assert( cdpstable.find( owner.value ) == cdpstable.end(), "CDP of this type already exists for owner" );

   // Create the CDP
   cdpstable.emplace( ram_payer, [&]( auto& p ) {
      p.owner = owner;
      p.created = now();
      p.collateral = asset( 0, st.total_collateral.quantity.symbol );
      p.stablecoin = asset( 0, st.total_stablecoin.symbol );
   }); 
}

// Reclaim some collateral from an overcollateralized CDP
ACTION bank.cdp::bail( name owner, symbol_code symbl, asset quantity )
{  
   // Make sure the owner is authenticated
   require_auth(owner);

   // Make sure the symbol and its value are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );
   
   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( quantity.symbol == st.total_collateral.quantity.symbol, "(CDP type, collateral symbol) mismatch" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdp_it = cdpstable.get( owner.value, "(CDP type, owner) mismatch" );
   eosio_assert( cdp_it.live, "CDP is in liquidation" );

   // Verify the person is not trying to free too much collateral
   eosio_assert( cdp_it.collateral > quantity, "Can't free this much collateral, try shut?" );

   // Check that the price feed exists and is not stale
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), "Feed does not exist" );
   eosio_assert( fc.stamp >= now() - FEED_FRESH || !st.live, "Collateral price feed data too stale" ); 

   // Affirm that the amount being bailed does not cause the CDP to fall below the liquidation ratio
   uint64_t amt = st.liquid8_ratio;
   if ( cdp_it.stablecoin.amount ) // just safety against divide by 0
      amt = ( fc.price.amount * 100 / cdp_it.stablecoin.amount ) *
            ( cdp_it.collateral.amount - quantity.amount ) / 10000;
   eosio_assert( amt >= st.liquid8_ratio, "Can't go below liquidation ratio" );
   add_balance( owner, quantity, st.total_collateral.contract );
   
   // Update the collateral amount for this specific CDP
   cdpstable.modify( cdp_it, owner, [&]( auto& p ) 
   {  p.collateral -= quantity; });

   // Update the total amount of collateral locked up by CDPs of this type
   stable.modify( st, same_payer,  [&]( auto& t ) 
   {  t.total_collateral.quantity -= quantity; });

   // Update the total amount of collateral in circulation globally 
   feedstable.modify( fc, same_payer, [&]( auto& f ) 
   {  f.total += quantity; });
}

// Issue fresh stablecoin from this CDP
ACTION bank.cdp::draw( name owner, symbol_code symbl, asset quantity )
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );

   // Get the stats for the stablecoin
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );

   // Make sure the CDP is live and not in global settlement
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, "(CDP type, stablecoin symbol) mismatch" );
   eosio_assert( st.live, "CDP type not yet live, or in global settlement" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdp_it = cdpstable.get( owner.value, "(CDP type, owner) mismatch");
   eosio_assert( cdp_it.live, "CDP is in liquidation" );
   
   // Increase the stablecoin amount in the CDP
   uint64_t amt = cdp_it.stablecoin.amount + quantity.amount;

   // Increase the stablecoin amount in the global stats
   uint64_t gmt = st.total_stablecoin.amount + quantity.amount; 

   // Verify that too much stablecoin is not being drawn, both at the global level and the CDP level
   eosio_assert( st.debt_ceiling >= amt, "Can't reach the debt ceiling" );
   eosio_assert( st.global_ceil >= gmt, "Can't reach the global debt ceiling" );

   // Make sure the price feed exists and has fresh data
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( cdp_it.collateral.symbol.code().raw(), "Feed does not exist" );
   eosio_assert( fc.stamp >= now() - FEED_FRESH, "Collateral price feed data too stale" ); 
   
   // Assure that the draw does not cause the CDP to go below the liquidation ratio
   uint64_t liq = ( fc.price.amount * 100 / amt ) * ( cdp_it.collateral.amount ) / 10000;
   eosio_assert( liq >= st.liquid8_ratio, "Can't go below liquidation ratio" );
   const auto& fs = feedstable.get( quantity.symbol.code().raw(), "Feed does not exist" );

   // Increase owner's stablecoin balance
   add_balance( owner, quantity, _self ); 
   cdpstable.modify( it, same_payer, [&]( auto& p ) 
   {  p.stablecoin += quantity; });

   // Update amount of stablecoin in circulation for this CDP type
   stable.modify( st, same_payer,  [&]( auto& t ) 
   {  t.total_stablecoin += quantity; });

   // Update amount of this stablecoin in circulation globally 
   feedstable.modify( fs, same_payer, [&]( auto& f ) 
   {  f.total += quantity; });
}

// Reduce the stablecoin balance by relinquishing some to the CDP
ACTION bank.cdp::wipe( name owner, symbol_code symbl, asset quantity ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" ); 
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );

   // Get the stats for the stablecoin
   stats stable( _self, _self.value );

   // Make sure the price feed exists and has fresh data
   feeds feedstable(_self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( quantity.symbol == st.total_stablecoin.symbol, "(CDP type, symbol) mismatch" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdp_it = cdpstable.get( owner.value, "(CDP type, owner) mismatch" ); 
   eosio_assert( cdp_it.live, "CDP in liquidation" );

   // Make sure that you are not trying to wipe more stablecoin than is available in for the CDP
   eosio_assert( cdp_it.stablecoin > quantity, "Can't wipe this much, try shut?" ); 

   // Get the price feed
   const auto& fv = feedstable.get( IQ_SYMBOL.code().raw(), "No price data" );

   // Calculate the stability fee and create the asset object
   uint64_t apr = 1000000 * st.stability_fee * 
                  ( now() - cdp_it.created ) / SECYR *
                  ( quantity.amount * 100 / fv.price.amount ) / 100000;
   asset fee = asset( apr, IQ_SYMBOL );

   // Have the CDP owner pay the stability fee
   sub_balance( owner, fee );

   //add_balance( _self, fee, IQ_NAME ); //increase contract's total fee balance

   // Have the owner relinquish the stablecoins
   sub_balance( owner, quantity );

   // Update the global fee and outstanding stablecoin amounts 
   stable.modify( st, same_payer,  [&]( auto& t ) { 
      t.fee_balance += fee; //just to keep track
      t.total_stablecoin -= quantity;
   });

   // Lower the amount of stablecoins in the CDP
   cdpstable.modify( it, same_payer, [&]( auto& p ) 
   {  p.stablecoin -= quantity; });

   // Get the price feed
   const auto& fs = feedstable.get( quantity.symbol.code().raw(), "No price data" );

   // Update amount of this stablecoin in circulation globally 
   feedstable.modify( fs, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

// Add more collateral to the CDP
ACTION bank.cdp::lock( name owner, symbol_code symbl, asset quantity ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );

   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( st.live, "CDP type not yet live, or in global settlement" );
   eosio_assert( st.total_collateral.quantity.symbol == quantity.symbol, "(CDP type, collateral symbol) mismatch" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdp_it = cdpstable.get( owner.value, "This CDP does not exist" );
   eosio_assert( cdp_it.live, "CDP in liquidation" );

   // Subtract the collateral from the owner and make sure the token isn't an identically-named copy from a fake contract
   name contract = sub_balance( owner, quantity );
   eosio_assert( st.total_collateral.contract == contract, "No using tokens from fake contracts" );

   // Update amount of collateral in circulation for this CDP type
   stable.modify( st, same_payer,  [&]( auto& t ) 
   {  t.total_collateral.quantity += quantity; });

   // Update amount of collateral in circulation for this CDP type
   cdpstable.modify( it, same_payer, [&]( auto& p ) 
   {  p.collateral += quantity; });
   feeds feedstable(_self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "no price data" 
                                  );
   //update amount of this collateral in circulation globally
   feedstable.modify( fc, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

// Close out the CDP by paying off the required DAI plus the stability fee
ACTION bank.cdp::shut( name owner, symbol_code symbl ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );

   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdp_it = cdpstable.get( owner.value, "This CDP does not exist" );
   eosio_assert( cdp_it.live, "CDP in liquidation" );
   
   asset new_total_stabl = st.total_stablecoin;
   if (cdp_it.stablecoin.amount > 0) {
      sub_balance( owner, cdp_it.stablecoin ); 
      new_total_stabl -= cdp_it.stablecoin;
   }
   asset new_total_clatrl = st.total_collateral.quantity;
   if ( cdp_it.collateral.amount > 0 ) {
      add_balance( owner, cdp_it.collateral, 
                   st.total_collateral.contract 
                 );
      new_total_clatrl += cdp_it.collateral;
   }
   stable.modify( st, same_payer, [&]( auto& t ) { 
      t.total_stablecoin = new_total_stabl;
      t.total_collateral.quantity = new_total_clatrl; 
   });
   cdpstable.erase( cdp_it );
}

// Global settlement
ACTION bank.cdp::settle( name feeder, symbol_code symbl ) 
{  
   // Make sure the feeder is authenticated
   require_auth( feeder );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   
   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( st.total_stablecoin.amount > 0 && st.live, "CDP type already settled, or has 0 debt" );
   eosio_assert( st.feeder == feeder, "Only feeder can trigger global settlement" ); 

   // Make sure the price feed exists and has fresh data
   feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( st.total_collateral.quantity.symbol.code().raw(), "No price data" );
   
   // Make sure there is no liquidation hazard
   uint64_t liq = ( fc.price.amount * 100 * st.total_collateral.quantity.amount ) / ( st.total_stablecoin.amount * 10000 );    
   eosio_assert( st.liquid8_ratio >= liq, "No liquidation hazard for settlement" );  
   
   // Set the stablecoin status as not live                                
   stable.modify( st, same_payer, [&]( auto& t ) 
   {  t.live = false; });
}

// Vote for or against a CDP proposal
ACTION bank.cdp::vote( name voter, symbol_code symbl, bool against, asset quantity ) 
{  
   // Make sure the voter is authenticated
   require_auth( voter );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.is_valid() && quantity.amount > 0, "Invalid quantity" );
   eosio_assert( quantity.symbol == IQ_SYMBOL, "Must vote with governance token" );

   // Get the proposal for the symbl-denominated CDP
   props propstable( _self, _self.value );
   const auto& pt = propstable.get( symbl.raw(), "CDP type not proposed" );

   // Lower the voter's IQ balance
   sub_balance( voter, quantity );

   // This is used to keep track of which voters voted for what CDP symbol proposals and how much IQ they used to do so
   accounts acnts( _self, symbl.raw() );  
   auto at = acnts.find( voter.value );
   if ( at == acnts.end() )
      acnts.emplace( voter, [&]( auto& a ) {
         //we know the symbol is IQ anyway
         //so instead keep track of prop symbol
         a.owner = voter;
         a.balance = asset( quantity.amount, symbol( symbl, 3 ) ); 
      });
   else
      acnts.modify( at, same_payer, [&]( auto& a ) 
      {  a.balance += asset( quantity.amount, symbol( symbl, 3 ) ); });

   // Keep track of the vote quantity by updating the propstable.
   if (against)
      propstable.modify( pt, same_payer, [&]( auto& p ) 
      {  p.nay += quantity; });
   else
      propstable.modify( pt, same_payer, [&]( auto& p ) 
      {  p.yay += quantity; });
}

// Sell off some collateral if the market moves against it. Similar to a margin call.
ACTION bank.cdp::liquify( name bidder, name owner, symbol_code symbl, asset bidamt ) 
{  
   // Make sure the bidder is authenticated
   require_auth( bidder );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid CDP type symbol name" );
   eosio_assert( bidamt.is_valid() && bidamt.amount > 0, "Bid is invalid" );

   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );

   // Fetch the CDP
   cdps cdpstable( _self, symbl.raw() );

   // Make sure the price feed exists
   feeds feedstable( _self, _self.value );

   // Verify that the CDP exists
   const auto& cdp_it = cdpstable.get( owner.value, "This CDP does not exist" );

   // Start liquidation
   if ( cdp_it.live ) {
      // Make sure the stablecoin balance is not zero
      eosio_assert ( cdp_it.stablecoin.amount > 0, "Can't liquify this CDP" );

      // Make sure the price feed exists and has fresh data
      const auto& fc = feedstable.get( cdp_it.collateral.symbol.code().raw(), "No price data" );
      eosio_assert( fc.stamp >= now() - FEED_FRESH || !st.live, "Collateral price feed data too stale" ); 

      // Assert that the liquidation keeps the liquidation ratio for that CDP market in line
      uint64_t liq = ( fc.price.amount * 100 / cdp_it.stablecoin.amount ) * ( cdp_it.collateral.amount ) / 10000;
      eosio_assert( st.liquid8_ratio > liq, "Must exceed liquidation ratio" );

      // Update the CDP contract for that bidder
      cdpstable.modify( it, bidder, [&]( auto& p ) { // liquidators get the RAM
         p.live = false; 
         p.stablecoin.amount += ( p.stablecoin.amount * st.penalty_ratio ) / 100;
      });
   } 

   // Get the list of bids for that currency, from that owner
   bids bidstable( _self, symbl.raw() );
   auto bt = bidstable.find( owner.value );

   // If there are no bids, create one
   if ( bt == bidstable.end() ) {
      // Make sure the symbol in the bid matches the stablecoin symbol
      eosio_assert( bidamt.symbol == cdp_it.stablecoin.symbol, "Bid is of wrong symbol" );

      // Subtract the bid amount (in stablecoin) from the bidder
      sub_balance( bidder, bidamt );

      // Adjust the CDP stablecoin total
      cdpstable.modify( it, bidder, [&]( auto& p ) 
      {  p.stablecoin -= bidamt; });

      // Create a bid on the bids table
      bidstable.emplace( bidder, [&]( auto& b ) {
         b.owner = owner;         
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.started = now();
         b.lastbid = now();
      }); 
   } 
   // Otherwise, modify an existing one, as long as an auction is currently ongoing
   else if ( st.ttl > ( now() - bt->lastbid ) && st.tau > ( now() - bt->started ) ) {
      // Make sure the symbol in the bid matches the stablecoin symbol
      eosio_assert( bidamt.symbol == cdp_it.stablecoin.symbol, "Bid is of wrong symbol" ); name contract = _self;

      // Verify that the minimum bid amount is enough
      uint64_t min = bt->bidamt.amount + ( bt->bidamt.amount * st.beg / 100 );
      eosio_assert( bidamt.amount >= min, "Bid too small" ); 

      // Make sure the stablecoin is IQ
      if ( cdp_it.stablecoin.symbol == IQ_SYMBOL )     
         contract = IQ_NAME;      

      // Balance out the bid amount and the contract amount
      sub_balance( bidder, bidamt );
      add_balance( bt->bidder, bt->bidamt, contract ); 

      // Subtract difference between bids CDP balance
      cdpstable.modify( it, bidder, [&]( auto& p ) 
      {  p.stablecoin -= ( bidamt - bt->bidamt ); }); 

      // Update the bids table
      bidstable.modify( bt, bidder, [&]( auto& b ) { 
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.lastbid = now();
      });   
   } else {
      if ( cdp_it.collateral.amount ) {
         cdpstable.modify( it, same_payer, [&]( auto& p )
         {  p.collateral -= p.collateral; });
         const auto& fc = feedstable.get( cdp_it.collateral.symbol.code().raw(), 
                                          "Feed does not exist" 
                                        ); name contract = _self;
         //update amount of this collateral in circulation globally
         feedstable.modify( fc, same_payer, [&]( auto& f ) 
         {  f.total += cdp_it.collateral; });
         if ( cdp_it.collateral.symbol == IQ_SYMBOL )
            contract = IQ_NAME;
         //update amount of collateral locked up by cdps of type
         else if ( st.total_collateral.quantity.symbol == cdp_it.collateral.symbol ) {
            stable.modify( st, same_payer,  [&]( auto& t ) 
            {  t.total_collateral.quantity -= cdp_it.collateral; });
            contract = st.total_collateral.contract;
         }
         add_balance( bt->bidder, cdp_it.collateral, contract );
      } 
      if ( cdp_it.stablecoin.amount )  {
         const auto& fv = feedstable.get( IQ_SYMBOL.code().raw(), 
                                          "no price data" 
                                        );
         eosio_assert( fv.stamp >= now() - FEED_FRESH,
                       "iq price feed data too stale"
                     ); 
         if ( cdp_it.stablecoin.amount > 0 )
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               if ( cdp_it.stablecoin.symbol == IQ_SYMBOL )
                  p.collateral = asset( cdp_it.stablecoin.amount * fv.price.amount, 
                                        fv.price.symbol 
                                      ); 
               else
                  p.collateral = asset( cdp_it.stablecoin.amount  * 1000 / fv.price.amount,
                                        IQ_SYMBOL 
                                      );
            });   
         else if ( cdp_it.stablecoin.amount < 0 )
            cdpstable.modify( it, same_payer, [&]( auto& p ) {  
               p.collateral = -it.stablecoin;
               if ( cdp_it.stablecoin.symbol == IQ_SYMBOL )
                  p.stablecoin = asset( -( cdp_it.stablecoin.amount * fv.price.amount ), 
                                        fv.price.symbol 
                                      );
               else p.stablecoin = asset( -( cdp_it.stablecoin.amount * 1000 / fv.price.amount ), 
                                          IQ_SYMBOL 
                                        );
            });
         eosio_assert( bidamt.symbol == cdp_it.stablecoin.symbol, 
                       "bid is of wrong symbol" 
                     );
         sub_balance( bidder, bidamt );
         const auto& fs = feedstable.get( bidamt.symbol.code().raw(), 
                                          "no price data" 
                                        );
         feedstable.modify( fs, same_payer, [&]( auto& f ) 
         {  f.total -= bidamt; });
         cdpstable.modify( it, bidder, [&]( auto& p ) 
         {  p.stablecoin -= bidamt; });
         bidstable.modify( bt, bidder, [&]( auto& b ) { 
            b.bidder = bidder;
            b.bidamt = bidamt;
            b.started = now();
            b.lastbid = now();
         });
      }
   } 
   if ( cdp_it.stablecoin.amount == 0 ) {
      if ( cdp_it.collateral.amount ) {
         name contract = _self;
         if ( cdp_it.collateral.symbol == IQ_SYMBOL )
            contract = IQ_NAME;
         else if ( cdp_it.collateral.symbol == st.total_collateral.quantity.symbol )
            contract = st.total_collateral.contract;
         add_balance( bt->bidder, cdp_it.collateral, contract );
         const auto& fc = feedstable.get( cdp_it.collateral.symbol.code().raw(), 
                                          "Feed does not exist" 
                                        );
         feedstable.modify( fc, same_payer, [&]( auto& f ) 
         {  f.total += cdp_it.collateral; });
      }
      bidstable.erase( bt );
      cdpstable.erase( cdp_it );
   }
}

ACTION bank.cdp::propose( name proposer, symbol_code symbl, 
                              symbol clatrl, symbol_code stabl,
                              uint64_t max, uint64_t gmax, 
                              uint64_t pen, uint64_t fee,
                              uint64_t beg, uint64_t liq, 
                              uint32_t tau, uint32_t ttl,
                              name feeder, name contract ) 
{  
   // Make sure the proposer is authenticated
   require_auth( proposer );
   is_account( feeder );

   // Make sure the symbols and their values are valid
   eosio_assert( symbl.is_valid(), "Invalid CDP type symbol name" );
   eosio_assert( clatrl.is_valid(), "Invalid collateral symbol name" );
   eosio_assert( stabl.is_valid(), "Invalid stablecoin symbol name" );

   props propstable( _self, _self.value );
   eosio_assert( propstable.find( symbl.raw() ) == propstable.end(), "Proposal for this CDP type already in session" );

   /* scope into owner because self scope may already have this symbol.
    * if symbol exists in self scope and this proposal is voted into live,
    * then original symbol would be erased, and symbol in proposer's scope
    * will be moved into self scope, then erased from proposer's scope.
    */ 
   stats stable( _self, _self.value );   
   stats pstable( _self, proposer.value ); 
   eosio_assert( pstable.find( symbl.raw() ) == pstable.end(), "Proposer already claimed this CDP type" ); 
   feeds feedstable(_self, _self.value);
   eosio_assert( feedstable.find( symbl.raw() ) == feedstable.end(), "Can't propose collateral or governance symbols" ); 
   eosio_assert( feedstable.find( stabl.raw() ) != feedstable.end(), "Can't propose an unknown stablecoin symbol" );
   if ( !tau && !ttl ) { // passing 0 for ttl, tau = flip settlement
      const auto& st = stable.get( symbl.raw(), "CDP type does not exist" );
      eosio_assert( proposer == st.feeder, "Only priviliged accounts may propose global settlement" );
   } else {
      eosio_assert( max < gmax && max > 0 &&
                    gmax <= max * 1000000 &&
                    pen <= 100 && pen > 0 &&
                    fee <= 100 && fee > 0 &&
                    beg <= 100 && beg > 0 &&
                    liq <= 1000 && liq >= 100
                    && tau <= 3600 
                    && ttl <= 600, "bad props"
                  ); //TODO: are these realistic?
      pstable.emplace( proposer, [&]( auto& t ) {
         t.tau = tau;
         t.ttl = ttl;
         t.beg = beg;
         t.feeder = feeder;
         t.cdp_type = symbl;
         t.global_ceil = gmax;
         t.debt_ceiling = max;
         t.penalty_ratio = pen;
         t.liquid8_ratio = liq;
         t.stability_fee = fee;
         t.fee_balance = asset( 0, IQ_SYMBOL ); 
         t.total_stablecoin = asset( 0, symbol( stabl, 2 ) ); 
         t.total_collateral = extended_asset( asset( 0, clatrl ), 
                                              contract
                                            );
      });
   }
   propstable.emplace( proposer, [&]( auto& p ) {
      p.cdp_type = symbl;
      p.proposer = proposer;
      p.deadline = now() + VOTE_PERIOD;
      p.nay = asset( 0, IQ_SYMBOL );
      p.yay = asset( 0, IQ_SYMBOL );
   }); 
   transaction txn{};
   txn.actions.emplace_back(  permission_level { _self, "active"_n },
                              _self, "referended"_n, 
                              make_tuple( proposer, symbl )
                           ); txn.delay_sec = VOTE_PERIOD;
   uint128_t txid = (uint128_t(proposer.value) << 64) | now();
   txn.send(txid, _self); 
}

ACTION bank.cdp::referended( name proposer, symbol_code symbl ) 
{  
   // Make sure the contract is authenticated
   require_auth( _self );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
  
   props propstable( _self, _self.value );
   const auto& prop = propstable.get( symbl.raw(), "No such proposal" );
   eosio_assert( now() >= prop.deadline, "Too soon to be referended" );
      
   if ( prop.yay == prop.nay ) { //vote tie, so revote
      transaction txn{};
      txn.actions.emplace_back(  permission_level { _self, "active"_n },
                                 _self, "referended"_n, 
                                 make_tuple( proposer, symbl )
                              ); txn.delay_sec = VOTE_PERIOD;
      uint128_t txid = (uint128_t(proposer.value) << 64) | now();
      txn.send(txid, _self); 
   } else { //the proposal was voted in for or against
      stats pstable( _self, proposer.value );
      auto pstat = pstable.find( symbl.raw() );
      bool settlement = ( pstat == pstable.end() );
      if ( prop.yay > prop.nay ) { //implement proposed changes...
         stats stable( _self, _self.value );
         auto stat_exist = stable.find( symbl.raw() );
         if ( settlement )
            stable.modify( stat_exist, same_payer, [&]( auto& t ) 
            {  t.live = !t.live; });
         else {
            if ( stat_exist == stable.end() )
               stable.emplace( _self, [&]( auto& t ) {
                  t.live = true;
                  t.last_vote = now();
                  t.tau = pstat->tau;
                  t.ttl = pstat->ttl;
                  t.beg = pstat->beg;
                  t.feeder = pstat->feeder;
                  t.cdp_type = pstat->cdp_type;
                  t.global_ceil = pstat->global_ceil;
                  t.debt_ceiling = pstat->debt_ceiling;
                  t.stability_fee = pstat->stability_fee;
                  t.penalty_ratio = pstat->penalty_ratio;
                  t.liquid8_ratio = pstat->liquid8_ratio;
                  t.fee_balance = pstat->fee_balance;
                  t.total_stablecoin = pstat->total_stablecoin;
                  t.total_collateral = pstat->total_collateral;
               });
            else
               stable.modify( stat_exist, same_payer, [&]( auto& t ) {
                  t.live = true;
                  t.last_vote = now();
                  t.tau = pstat->tau;
                  t.ttl = pstat->ttl;
                  t.beg = pstat->beg;
                  t.feeder = pstat->feeder;
                  t.global_ceil = pstat->global_ceil;
                  t.debt_ceiling = pstat->debt_ceiling;
                  t.stability_fee = pstat->stability_fee;
                  t.penalty_ratio = pstat->penalty_ratio;
                  t.liquid8_ratio = pstat->liquid8_ratio;
               });
         }
      } //refund all voters' voting tokens
      accounts vacnts( _self, symbl.raw() );
      auto it = vacnts.begin();
      while ( it != vacnts.end() ) { //TODO: refactor, unsustainable loop
         add_balance( it->owner, asset( it->balance.amount, IQ_SYMBOL ), IQ_NAME );   
         it = vacnts.erase( it );
      } 
      if ( !settlement ) //safely erase the proposal
         pstable.erase( *pstat );
      propstable.erase( prop );
   }
}

ACTION bank.cdp::upfeed( name feeder, asset price, 
                             symbol_code cdp_type, symbol symbl ) 
{  
   // Make sure the feeder is authenticated
   require_auth( feeder );

   // Make sure the symbol and its value are valid
   eosio_assert( price.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( price.amount > 0, "Must use a positive quantity" );

   if ( !has_auth( _self ) ) {
      stats stable( _self, _self.value );
      const auto& st = stable.get( cdp_type.raw(), 
                                   "CDP type does not exist" 
                                 );
      eosio_assert( st.feeder == feeder, 
                    "account not authorized to be price feeder"  
                  );
      eosio_assert( st.total_stablecoin.symbol == price.symbol, 
                    "price symbol must match stablecoin symbol"
                  );
      eosio_assert( st.total_collateral.quantity.symbol == symbl, 
                    "asset symbol must match collateral symbol" 
                  );
   } feeds feedstable( _self, _self.value );
   auto ft = feedstable.find( symbl.code().raw() );
   if ( ft != feedstable.end() )
      feedstable.modify( ft, feeder, [&]( auto& f ) {
         f.price.amount *= (17 / 20);
         f.price.amount += (3 / 20) * price.amount;
         f.stamp = now();
      });
   else
      feedstable.emplace( _self, [&]( auto& f ) {
         f.total = asset( 0, symbl );
         f.price = price;
         f.stamp = now();
      }); 
}

ACTION bank.cdp::deposit( name from, name to,
                              asset quantity, string memo ) 
{  
   // Make sure the depositor is authenticated
   require_auth( from );
   name contract = get_code();
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   if ( from != _self ) {
      eosio_assert( to == _self, "can only deposit to self" ); 
      if ( quantity.symbol == IQ_SYMBOL )
         eosio_assert ( contract == IQ_NAME, 
                        "IQ deposit must come from IQ contract" 
                      );
      add_balance( from, quantity, contract );
      feeds feedstable( _self, _self.value );
      const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                       "Feed does not exist" 
                                     );
      feedstable.modify( fc, same_payer, [&]( auto& f ) 
      {  f.total += quantity; });
   }
}

ACTION bank.cdp::transfer( name from, name to,
                               asset quantity, string memo )
{  
   // Make sure the depositor is authenticated
   require_auth( from );
   eosio_assert( is_account( to ), "to account does not exist");
   eosio_assert( from != to, "cannot transfer to self" );
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
   name contract = sub_balance( from, quantity );
   eosio_assert( contract == _self, "can only transfer stablecoin");
   add_balance( to, quantity, contract);
}

ACTION bank.cdp::withdraw( name owner, asset quantity, string memo ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( quantity.amount >= 0, "must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
   if ( quantity.amount == 0 )
      quantity = get_balance( _self, quantity.symbol.code(), owner );
   name code = sub_balance( owner, quantity );
   eosio_assert( owner != _self && code != _self, 
                 "self cannot withdraw, cannot withdraw into self" 
               ); feeds feedstable( _self, _self.value );
   const auto& fc = feedstable.get( quantity.symbol.code().raw(), 
                                    "Feed does not exist" 
                                  );
   action(
      permission_level{ _self, name( "active" ) },
      code, name( "transfer" ),
      std::make_tuple( _self, owner, quantity, memo )
   ).send(); 
   feedstable.modify( fc, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

ACTION bank.cdp::close( name owner, symbol_code symbl ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );
   accounts acnts( _self, symbl.raw() );
   auto it = acnts.get( owner.value,
                        "no balance object found"
                      );
   eosio_assert( cdp_it.balance.amount == 0, 
                 "balance must be zero" 
               );
   acnts.erase( it );
}

name bank.cdp::sub_balance( name owner, asset value ) 
{  accounts from_acnts( _self, value.symbol.code().raw() );
   const auto& from = from_acnts.get( owner.value, 
                                      "no balance object found" 
                                    );
   eosio_assert( from.balance.amount >= value.amount, 
                 "overdrawn balance" 
               );
   from_acnts.modify( from, owner, [&]( auto& a ) 
   {  a.balance -= value; }); return from.code;
}

void bank.cdp::add_balance( name owner, asset value, name code ) 
{  accounts to_acnts( _self, value.symbol.code().raw() );
   auto to = to_acnts.find( owner.value );
   
   if ( to == to_acnts.end() ) 
      to_acnts.emplace( _self, [&]( auto& a ) { 
         a.balance = value;
         a.owner = owner; 
         a.code = code; 
      });
   else {
      eosio_assert( to->code == code, "wrong contract" );
      to_acnts.modify( to, same_payer, [&]( auto& a ) 
      { a.balance += value; });
   }
}

//checking all transfers, and not only from EOS system token
extern "C" void apply( uint64_t receiver, uint64_t code, uint64_t action ) 
{  if ( action == "transfer"_n.value && code != receiver )
      eosio::execute_action( eosio::name(receiver), eosio::name(code), &bank.cdp::deposit );
   if ( code == receiver )
      switch ( action ) {
         EOSIO_DISPATCH_HELPER( bank.cdp, /*(give)*/(open)(close)(shut)(lock)(bail)(draw)(wipe)(settle)(vote)(propose)(referended)(liquify)(upfeed)(withdraw) )
      }
}