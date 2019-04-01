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

// ACTION bankcdp::give( name giver, name taker, symbol_code symbl )
// {  require_auth( giver ); 
//    is_account( taker );
//    eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   
//    stats stable( _self, _self.value );
//    const auto& stats_it = stable.get( symbl.raw(), 
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
ACTION bankcdp::open( name owner, symbol_code symbl, name ram_payer )
{  
   // Make sure the RAM payer is authenticated
   require_auth( ram_payer );

   // Check for a valid symbol
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );

   // Get the stats for the stablecoin
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );

   // Make sure the CDP is live and not in global settlement
   eosio_assert( stats_it.live, "CDP type not yet live, or in global settlement" );

   // Make sure the price feed exists and has fresh data
   feeds feedstable( _self, _self.value );
   const auto& feeds_it_collateral = feedstable.get( stats_it.total_collateral.quantity.symbol.code().raw(), "collateral Feed does not exist" );
   eosio_assert( feeds_it_collateral.stamp >= now() - FEED_FRESH, "Collateral price feed data too stale" );

   // If the owner already has a CDP, they cannot create another one
   cdps cdpstable( _self, symbl.raw() );
   eosio_assert( cdpstable.find( owner.value ) == cdpstable.end(), "CDP of this type already exists for owner" );

   // Create the CDP
   cdpstable.emplace( ram_payer, [&]( auto& p ) {
      p.owner = owner;
      p.created = now();
      p.collateral = asset( 0, stats_it.total_collateral.quantity.symbol );
      p.stablecoin = asset( 0, stats_it.total_stablecoin.symbol );
   }); 
}

// Reclaim some collateral from an overcollateralized CDP
ACTION bankcdp::bail( name owner, symbol_code symbl, asset quantity )
{  
   // Make sure the owner is authenticated
   require_auth(owner);

   // Make sure the symbol and its value are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );
   
   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( quantity.symbol == stats_it.total_collateral.quantity.symbol, "(CDP type, collateral symbol) mismatch" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdps_it = cdpstable.get( owner.value, "(CDP type, owner) mismatch" );
   eosio_assert( cdps_it.live, "CDP is in liquidation" );

   // Verify the person is not trying to free too much collateral
   eosio_assert( cdps_it.collateral > quantity, "Can't free this much collateral, try shut?" );

   // Check that the price feed exists and is not stale
   feeds feedstable( _self, _self.value );
   const auto& feeds_it_collateral = feedstable.get( quantity.symbol.code().raw(), "Feed does not exist" );
   eosio_assert( feeds_it_collateral.stamp >= now() - FEED_FRESH || !stats_it.live, "Collateral price feed data too stale" ); 

   // Affirm that the amount being bailed does not cause the CDP to fall below the liquidation ratio
   uint64_t amt = stats_it.liquid8_ratio;
   if ( cdps_it.stablecoin.amount ) // just safety against divide by 0
      amt = ( feeds_it_collateral.price.amount * 100 / cdps_it.stablecoin.amount ) *
            ( cdps_it.collateral.amount - quantity.amount ) / 10000;
   eosio_assert( amt >= stats_it.liquid8_ratio, "Can't go below liquidation ratio" );
   add_balance( owner, quantity, stats_it.total_collateral.contract );
   
   // Update the collateral amount for this specific CDP
   cdpstable.modify( cdps_it, owner, [&]( auto& p ) 
   {  p.collateral -= quantity; });

   // Update the total amount of collateral locked up by CDPs of this type
   stable.modify( stats_it, same_payer,  [&]( auto& t ) 
   {  t.total_collateral.quantity -= quantity; });

   // Update the total amount of collateral in circulation globally 
   feedstable.modify( feeds_it_collateral, same_payer, [&]( auto& f ) 
   {  f.total += quantity; });
}

// Issue fresh stablecoin from this CDP
ACTION bankcdp::draw( name owner, symbol_code symbl, asset quantity )
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );

   // Get the stats for the stablecoin
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );

   // Make sure the CDP is live and not in global settlement
   eosio_assert( quantity.symbol == stats_it.total_stablecoin.symbol, "(CDP type, stablecoin symbol) mismatch" );
   eosio_assert( stats_it.live, "CDP type not yet live, or in global settlement" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdps_it = cdpstable.get( owner.value, "(CDP type, owner) mismatch");
   eosio_assert( cdps_it.live, "CDP is in liquidation" );
   
   // Increase the stablecoin amount in the CDP
   uint64_t amt = cdps_it.stablecoin.amount + quantity.amount;

   // Increase the stablecoin amount in the global stats
   uint64_t gmt = stats_it.total_stablecoin.amount + quantity.amount; 

   // Verify that too much stablecoin is not being drawn, both at the global level and the CDP level
   eosio_assert( stats_it.debt_ceiling >= amt, "Can't reach the debt ceiling" );
   eosio_assert( stats_it.global_ceil >= gmt, "Can't reach the global debt ceiling" );

   // Make sure the price feed exists and has fresh data
   feeds feedstable( _self, _self.value );
   const auto& feeds_it_collateral = feedstable.get( cdps_it.collateral.symbol.code().raw(), "Feed does not exist" );
   eosio_assert( feeds_it_collateral.stamp >= now() - FEED_FRESH, "Collateral price feed data too stale" ); 
   
   // Assure that the draw does not cause the CDP to go below the liquidation ratio
   uint64_t liq = ( feeds_it_collateral.price.amount * 100 / amt ) * ( cdps_it.collateral.amount ) / 10000;
   eosio_assert( liq >= stats_it.liquid8_ratio, "Can't go below liquidation ratio" );
   const auto& feeds_it_stablecoin = feedstable.get( quantity.symbol.code().raw(), "Feed does not exist" );

   // Increase owner's stablecoin balance
   add_balance( owner, quantity, _self ); 
   cdpstable.modify( cdps_it, same_payer, [&]( auto& p ) 
   {  p.stablecoin += quantity; });

   // Update amount of stablecoin in circulation for this CDP type
   stable.modify( stats_it, same_payer,  [&]( auto& t ) 
   {  t.total_stablecoin += quantity; });

   // Update amount of this stablecoin in circulation globally 
   feedstable.modify( feeds_it_stablecoin, same_payer, [&]( auto& f ) 
   {  f.total += quantity; });
}

// Reduce the stablecoin balance by relinquishing some to the CDP
ACTION bankcdp::wipe( name owner, symbol_code symbl, asset quantity ) 
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
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( quantity.symbol == stats_it.total_stablecoin.symbol, "(CDP type, symbol) mismatch" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdps_it = cdpstable.get( owner.value, "(CDP type, owner) mismatch" ); 
   eosio_assert( cdps_it.live, "CDP in liquidation" );

   // Make sure that you are not trying to wipe more stablecoin than is available in for the CDP
   eosio_assert( cdps_it.stablecoin > quantity, "Can't wipe this much, try shut?" ); 

   // Get the price feed
   const auto& feeds_it = feedstable.get( IQ_SYMBOL.code().raw(), "No price data" );

   // Calculate the stability fee and create the asset object
   uint64_t apr = 1000000 * stats_it.stability_fee * ( now() - cdps_it.created ) / SECYR *
                  ( quantity.amount * 100 / feeds_it.price.amount ) / 100000;
   asset fee = asset( apr, IQ_SYMBOL );

   // Have the CDP owner pay the stability fee
   sub_balance( owner, fee );

   //add_balance( _self, fee, IQ_NAME ); //increase contract's total fee balance

   // Have the owner relinquish the stablecoins
   sub_balance( owner, quantity );

   // Update the global fee and outstanding stablecoin amounts 
   stable.modify( stats_it, same_payer,  [&]( auto& t ) { 
      t.fee_balance += fee; //just to keep track
      t.total_stablecoin -= quantity;
   });

   // Lower the amount of stablecoins in the CDP
   cdpstable.modify( cdps_it, same_payer, [&]( auto& p ) 
   {  p.stablecoin -= quantity; });

   // Get the price feed
   const auto& feeds_it_stablecoin = feedstable.get( quantity.symbol.code().raw(), "No price data" );

   // Update amount of this stablecoin in circulation globally 
   feedstable.modify( feeds_it_stablecoin, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

// Add more collateral to the CDP
ACTION bankcdp::lock( name owner, symbol_code symbl, asset quantity ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.amount > 0, "Must use a positive quantity" );

   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( stats_it.live, "CDP type not yet live, or in global settlement" );
   eosio_assert( stats_it.total_collateral.quantity.symbol == quantity.symbol, "(CDP type, collateral symbol) mismatch" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdps_it = cdpstable.get( owner.value, "This CDP does not exist" );
   eosio_assert( cdps_it.live, "CDP in liquidation" );

   // Subtract the collateral from the owner and make sure the token isn't an identically-named copy from a fake contract
   name contract = sub_balance( owner, quantity );
   eosio_assert( stats_it.total_collateral.contract == contract, "No using tokens from fake contracts" );

   // Update amount of collateral in circulation for this CDP type
   stable.modify( stats_it, same_payer,  [&]( auto& t ) 
   {  t.total_collateral.quantity += quantity; });

   // Update amount of collateral in circulation for this CDP type
   cdpstable.modify( cdps_it, same_payer, [&]( auto& p ) 
   {  p.collateral += quantity; });
   feeds feedstable(_self, _self.value );
   const auto& feeds_it = feedstable.get( quantity.symbol.code().raw(), "No price data" );

   //update amount of this collateral in circulation globally
   feedstable.modify( feeds_it, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

// Close out the CDP by paying off the required stablecoin plus the stability fee. Give the collateral back to the owner.
ACTION bankcdp::shut( name owner, symbol_code symbl ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );

   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );

   // Fetch the CDP, validate the owner, and make sure it is not being liquidated
   cdps cdpstable( _self, symbl.raw() );
   const auto& cdps_it = cdpstable.get( owner.value, "This CDP does not exist" );
   eosio_assert( cdps_it.live, "CDP in liquidation" );
   
   // Remove the stablecoin from the owner and calculate the new total stablecoin in circulation
   asset new_total_stabl = stats_it.total_stablecoin;
   if (cdps_it.stablecoin.amount > 0) {
      sub_balance( owner, cdps_it.stablecoin ); 
      new_total_stabl -= cdps_it.stablecoin;
   }

   // Give the collateral back to the owner and calculate the new total collateral locked up
   asset new_total_clatrl = stats_it.total_collateral.quantity;
   if ( cdps_it.collateral.amount > 0 ) {
      add_balance( owner, cdps_it.collateral, stats_it.total_collateral.contract );
      new_total_clatrl -= cdps_it.collateral;
   }

   // Update the stablecoin and collateral stats
   stable.modify( stats_it, same_payer, [&]( auto& t ) { 
      t.total_stablecoin = new_total_stabl;
      t.total_collateral.quantity = new_total_clatrl; 
   });

   // Remove the CDP contract entr y for this owner
   cdpstable.erase( cdps_it );
}

// Global settlement
ACTION bankcdp::settle( name feeder, symbol_code symbl ) 
{  
   // Make sure the feeder is authenticated
   require_auth( feeder );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   
   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );
   eosio_assert( stats_it.total_stablecoin.amount > 0 && stats_it.live, "CDP type already settled, or has 0 debt" );
   eosio_assert( stats_it.feeder == feeder, "Only feeder can trigger global settlement" ); 

   // Make sure the price feed exists and has fresh data
   feeds feedstable( _self, _self.value );
   const auto& feeds_it_collateral = feedstable.get( stats_it.total_collateral.quantity.symbol.code().raw(), "No price data" );
   
   // Make sure there is no liquidation hazard
   uint64_t liq = ( feeds_it_collateral.price.amount * 100 * stats_it.total_collateral.quantity.amount ) / ( stats_it.total_stablecoin.amount * 10000 );    
   eosio_assert( stats_it.liquid8_ratio >= liq, "No liquidation hazard for settlement" );  
   
   // Set the stablecoin status as not live                                
   stable.modify( stats_it, same_payer, [&]( auto& t ) 
   {  t.live = false; });
}

// Vote for or against a CDP proposal
ACTION bankcdp::vote( name voter, symbol_code symbl, bool against, asset quantity ) 
{  
   // Make sure the voter is authenticated
   require_auth( voter );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( quantity.is_valid() && quantity.amount > 0, "Invalid quantity" );
   eosio_assert( quantity.symbol == IQ_SYMBOL, "Must vote with governance token" );

   // Get the proposal for the symbl-denominated CDP
   props propstable( _self, _self.value );
   const auto& props_it = propstable.get( symbl.raw(), "CDP type not proposed" );

   // Lower the voter's IQ balance
   sub_balance( voter, quantity );

   // This is used to keep track of which voters voted for what CDP symbol proposals and how much IQ they used to do so
   accounts acnts( _self, symbl.raw() );  
   auto accounts_it = acnts.find( voter.value );
   if ( accounts_it == acnts.end() )
      acnts.emplace( voter, [&]( auto& a ) {
         //we know the symbol is IQ anyway
         //so instead keep track of prop symbol
         a.owner = voter;
         a.balance = asset( quantity.amount, symbol( symbl, 3 ) ); 
      });
   else
      acnts.modify( accounts_it, same_payer, [&]( auto& a ) 
      {  a.balance += asset( quantity.amount, symbol( symbl, 3 ) ); });

   // Keep track of the vote quantity by updating the propstable.
   if (against)
      propstable.modify( props_it, same_payer, [&]( auto& p ) 
      {  p.nay += quantity; });
   else
      propstable.modify( props_it, same_payer, [&]( auto& p ) 
      {  p.yay += quantity; });
}

// Sell off some collateral for IQ if the market moves against the former. Similar to a margin call.
ACTION bankcdp::liquify( name bidder, name owner, symbol_code symbl, asset bidamt ) 
{  
   // Make sure the bidder is authenticated
   require_auth( bidder );

   // Make sure the symbol and its value are valid
   eosio_assert( symbl.is_valid(), "Invalid CDP type symbol name" );
   eosio_assert( bidamt.is_valid() && bidamt.amount > 0, "Bid is invalid" );

   // Get the stats for the stablecoin and validate
   stats stable( _self, _self.value );
   const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );

   // Fetch the CDP
   cdps cdpstable( _self, symbl.raw() );

   // Make sure the price feed exists
   feeds feedstable( _self, _self.value );

   // Verify that the CDP exists
   const auto& cdps_it = cdpstable.get( owner.value, "This CDP does not exist" );

   // Start liquidation
   if ( cdps_it.live ) {
      // Make sure the stablecoin balance is not zero
      eosio_assert ( cdps_it.stablecoin.amount > 0, "Can't liquify this CDP" );

      // Make sure the price feed exists and has fresh data
      const auto& feeds_it_collateral = feedstable.get( cdps_it.collateral.symbol.code().raw(), "No price data" );
      eosio_assert( feeds_it_collateral.stamp >= now() - FEED_FRESH || !stats_it.live, "Collateral price feed data too stale" ); 

      // Assert that the liquidation keeps the liquidation ratio for that CDP market in line
      uint64_t liq = ( feeds_it_collateral.price.amount * 100 / cdps_it.stablecoin.amount ) * ( cdps_it.collateral.amount ) / 10000;
      eosio_assert( stats_it.liquid8_ratio > liq, "Must exceed liquidation ratio" );

      // Update the CDP contract for that bidder
      cdpstable.modify( cdps_it, bidder, [&]( auto& p ) { // liquidators get the RAM
         p.live = false; 
         p.stablecoin.amount += ( p.stablecoin.amount * stats_it.penalty_ratio ) / 100;
      });
   } 

   // Get the list of bids for that currency, from that owner
   bids bidstable( _self, symbl.raw() );
   auto bids_it = bidstable.find( owner.value );

   // If there are no bids, create one
   if ( bids_it == bidstable.end() ) {
      // Make sure the symbol in the bid matches the stablecoin symbol
      eosio_assert( bidamt.symbol == cdps_it.stablecoin.symbol, "Bid is of wrong symbol" );

      // Subtract the bid amount (in stablecoin) from the bidder
      sub_balance( bidder, bidamt );

      // Adjust the CDP stablecoin total
      cdpstable.modify( cdps_it, bidder, [&]( auto& p ) 
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
   else if ( stats_it.ttl > ( now() - bids_it->lastbid ) && stats_it.tau > ( now() - bids_it->started ) ) {
      // Make sure the symbol in the bid matches the stablecoin symbol
      eosio_assert( bidamt.symbol == cdps_it.stablecoin.symbol, "Bid is of wrong symbol" ); name contract = _self;

      // Verify that the minimum bid amount is enough
      uint64_t min = bids_it->bidamt.amount + ( bids_it->bidamt.amount * stats_it.beg / 100 );
      eosio_assert( bidamt.amount >= min, "Bid too small" ); 

      // Make sure the stablecoin is IQ
      if ( cdps_it.stablecoin.symbol == IQ_SYMBOL )     
         contract = IQ_NAME;      

      // Balance out the bid amount and the contract amount
      sub_balance( bidder, bidamt );
      add_balance( bids_it->bidder, bids_it->bidamt, contract ); 

      // Subtract difference between bids CDP balance
      cdpstable.modify( cdps_it, bidder, [&]( auto& p ) 
      {  p.stablecoin -= ( bidamt - bids_it->bidamt ); }); 

      // Update the bids table
      bidstable.modify( bids_it, bidder, [&]( auto& b ) { 
         b.bidder = bidder;
         b.bidamt = bidamt;
         b.lastbid = now();
      });   
   // TODO: NEED DESCRIPTION HERE
   } else {
      // Make sure there is some collateral
      if ( cdps_it.collateral.amount ) {
         // Reduce the amount of collateral in the CDP
         cdpstable.modify( cdps_it, same_payer, [&]( auto& p )
         {  p.collateral -= p.collateral; });

         // Make sure the price feed exists
         const auto& feeds_it_collateral = feedstable.get( cdps_it.collateral.symbol.code().raw(), "Feed does not exist" ); 
         name contract = _self;

         // Update amount of this collateral in circulation globally
         feedstable.modify( feeds_it_collateral, same_payer, [&]( auto& f ) 
         {  f.total += cdps_it.collateral; });
         if ( cdps_it.collateral.symbol == IQ_SYMBOL )
            contract = IQ_NAME;

         // Update amount of collateral locked up by CDPs of type
         else if ( stats_it.total_collateral.quantity.symbol == cdps_it.collateral.symbol ) {
            stable.modify( stats_it, same_payer,  [&]( auto& t ) 
            {  t.total_collateral.quantity -= cdps_it.collateral; });
            contract = stats_it.total_collateral.contract;
         }

         // Give the collateral to the bidder
         add_balance( bids_it->bidder, cdps_it.collateral, contract );
      } 
      // Verify that the amount of stablecoin amount is not null or zero
      if ( cdps_it.stablecoin.amount )  {
         // Make sure the price feed exists and is not stale
         const auto& feeds_it_iq = feedstable.get( IQ_SYMBOL.code().raw(), "No price data" );
         eosio_assert( feeds_it_iq.stamp >= now() - FEED_FRESH, "IQ price feed data too stale" );

         // Verify that the amount of stablecoin is nonzero
         if ( cdps_it.stablecoin.amount > 0 )
            // Update the CDP with the new collateral amount, based on the sale of the collateral for IQ
            cdpstable.modify( cdps_it, same_payer, [&]( auto& p ) {  
               if ( cdps_it.stablecoin.symbol == IQ_SYMBOL )
                  p.collateral = asset( cdps_it.stablecoin.amount * feeds_it_iq.price.amount, feeds_it_iq.price.symbol ); 
               else
                  p.collateral = asset( cdps_it.stablecoin.amount  * 1000 / feeds_it_iq.price.amount, IQ_SYMBOL );
            });   
         // TODO: NEED DESCRIPTION HERE. Is this accounting for excess stablecoin in the CDP?
         else if ( cdps_it.stablecoin.amount < 0 )
            cdpstable.modify( cdps_it, same_payer, [&]( auto& p ) {  
               p.collateral = -cdps_it.stablecoin;
               if ( cdps_it.stablecoin.symbol == IQ_SYMBOL )
                  p.stablecoin = asset( -( cdps_it.stablecoin.amount * feeds_it_iq.price.amount ), feeds_it_iq.price.symbol );
               else p.stablecoin = asset( -( cdps_it.stablecoin.amount * 1000 / feeds_it_iq.price.amount ), IQ_SYMBOL );
            });

         // Make sure the bid has the correct symbol
         eosio_assert( bidamt.symbol == cdps_it.stablecoin.symbol, "Bid is of wrong symbol" );

         // Subtract the bid from the bidder
         sub_balance( bidder, bidamt );

         // Get the price of the stablecoin
         const auto& feeds_it_stablecoin = feedstable.get( bidamt.symbol.code().raw(), "No price data" );

         // Update the feed table
         feedstable.modify( feeds_it_stablecoin, same_payer, [&]( auto& f ) 
         {  f.total -= bidamt; });

         // Lower the amount of stablecoins in the CDP
         cdpstable.modify( cdps_it, bidder, [&]( auto& p ) 
         {  p.stablecoin -= bidamt; });

         // Update the bid in the bids table
         bidstable.modify( bids_it, bidder, [&]( auto& b ) { 
            b.bidder = bidder;
            b.bidamt = bidamt;
            b.started = now();
            b.lastbid = now();
         });
      }
   } 

   // If there is no more stablecoin owed in the CDP, close it out and give the bidder the remaining collateral
   if ( cdps_it.stablecoin.amount == 0 ) {
      // If there is any more collateral left, give it to the bidder
      if ( cdps_it.collateral.amount ) {
         name contract = _self;
         if ( cdps_it.collateral.symbol == IQ_SYMBOL )
            contract = IQ_NAME;
         else if ( cdps_it.collateral.symbol == stats_it.total_collateral.quantity.symbol )
            contract = stats_it.total_collateral.contract;
         add_balance( bids_it->bidder, cdps_it.collateral, contract );

         // Get the current price of the collateral
         const auto& feeds_it_collateral = feedstable.get( cdps_it.collateral.symbol.code().raw(), "Feed does not exist" );

         // Update the feeds table by adding the collateral amount
         feedstable.modify( feeds_it_collateral, same_payer, [&]( auto& f ) 
         {  f.total += cdps_it.collateral; });
      }

      // Remove the bid
      bidstable.erase( bids_it );

      // Remove the CDP
      cdpstable.erase( cdps_it );
   }
}

// Propose a new CDP type, or modification of an existing one
ACTION bankcdp::propose( name proposer, symbol_code symbl, symbol clatrl, symbol_code stabl, uint64_t max, uint64_t gmax, 
                              uint64_t pen, uint64_t fee, uint64_t beg, uint64_t liq, uint32_t tau, uint32_t ttl,
                              name feeder, name contract ) 
{  
   // Make sure the proposer is authenticated
   require_auth( proposer );
   is_account( feeder );

   // Make sure the symbols and their values are valid
   eosio_assert( symbl.is_valid(), "Invalid CDP type symbol name" );
   eosio_assert( clatrl.is_valid(), "Invalid collateral symbol name" );
   eosio_assert( stabl.is_valid(), "Invalid stablecoin symbol name" );

   // Get the proposals
   props propstable( _self, _self.value );
   eosio_assert( propstable.find( symbl.raw() ) == propstable.end(), "Proposal for this CDP type already in session" );

   /* Scope into owner because self scope may already have this symbol.
    * if symbol exists in self scope and this proposal is voted into live,
    * then original symbol would be erased, and symbol in proposer's scope
    * will be moved into self scope, then erased from proposer's scope.
    */ 
   stats stable( _self, _self.value );   
   stats pstable( _self, proposer.value ); 

   // Make sure the symbol is not already being proposed
   eosio_assert( pstable.find( symbl.raw() ) == pstable.end(), "Proposer already claimed this CDP type" ); 

   // Make sure the collateral and stablecoin symbols exist
   feeds feedstable(_self, _self.value);
   eosio_assert( feedstable.find( symbl.raw() ) == feedstable.end(), "Can't propose collateral or governance symbols" ); 
   eosio_assert( feedstable.find( stabl.raw() ) != feedstable.end(), "Can't propose an unknown stablecoin symbol" );

   // Make sure that only privileged accounts can propose settlement on an existing CDP type
   if ( !tau && !ttl ) { // passing 0 for ttl, tau = flip settlement
      const auto& stats_it = stable.get( symbl.raw(), "CDP type does not exist" );
      eosio_assert( proposer == stats_it.feeder, "Only privileged accounts may propose global settlement" );
   } 
   // If a global settlement is not being proposed and an existing CDP type, proceed
   else {
      // Do some tests on the proposal values 
      eosio_assert( max < gmax && max > 0 &&
                    gmax <= max * 1000000 &&
                    pen <= 100 && pen > 0 &&
                    fee <= 100 && fee > 0 &&
                    beg <= 100 && beg > 0 &&
                    liq <= 1000 && liq >= 100
                    && tau <= 3600 
                    && ttl <= 600, "Bad proposal value(s)"
                  ); //TODO: are these realistic?

      // Add the proposal to the table
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
         t.total_collateral = extended_asset( asset( 0, clatrl ), contract );
      });
   }

   // Add the proposal to the table
   propstable.emplace( proposer, [&]( auto& p ) {
      p.cdp_type = symbl;
      p.proposer = proposer;
      p.deadline = now() + VOTE_PERIOD;
      p.nay = asset( 0, IQ_SYMBOL );
      p.yay = asset( 0, IQ_SYMBOL );
   }); 

   // Create a transaction
   transaction txn{};
   txn.actions.emplace_back( permission_level { _self, "active"_n }, _self, "referended"_n, make_tuple( proposer, symbl ) );
   txn.delay_sec = VOTE_PERIOD;

   // Create the transaction and send it
   uint128_t txid = (uint128_t(proposer.value) << 64) | now();
   txn.send(txid, _self); 
}

// Implement a completed CDP proposal
ACTION bankcdp::referended( name proposer, symbol_code symbl ) 
{  
   // Make sure the contract is authenticated
   require_auth( _self );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
  
   // Get the proposal and make sure the deadline has been reached
   props propstable( _self, _self.value );
   const auto& props_it = propstable.get( symbl.raw(), "No such proposal" );
   eosio_assert( now() >= props_it.deadline, "Too soon to be referended" );
      
   // If there is a tie, extend the voting period
   // TODO: This if block may need to actually change props_it.deadline...
   if ( props_it.yay == props_it.nay ) { 
      transaction txn{};
      txn.actions.emplace_back(  permission_level { _self, "active"_n },
                                 _self, "referended"_n, 
                                 make_tuple( proposer, symbl )
                              ); txn.delay_sec = VOTE_PERIOD;
      uint128_t txid = (uint128_t(proposer.value) << 64) | now();
      txn.send(txid, _self); 
   } 
   // The proposal was voted for or against
   else { 
      // Get the proposal
      stats pstable( _self, proposer.value );
      auto pstats_it = pstable.find( symbl.raw() );

      // Determine if a global settlement was voted on
      bool settlement = ( pstats_it == pstable.end() );

      // If the yays have it...
      if ( props_it.yay > props_it.nay ) {
         // Make sure the stablecoin exists
         stats stable( _self, _self.value );
         auto stat_exist = stable.find( symbl.raw() );

         // If a global settlement was voted on, turn it on or off
         if ( settlement )
            stable.modify( stat_exist, same_payer, [&]( auto& t ) 
            {  t.live = !t.live; });
         else {
            // Otherwise, create the CDP type if it doesn't exist
            if ( stat_exist == stable.end() )
               stable.emplace( _self, [&]( auto& t ) {
                  t.live = true;
                  t.last_vote = now();
                  t.tau = pstats_it->tau;
                  t.ttl = pstats_it->ttl;
                  t.beg = pstats_it->beg;
                  t.feeder = pstats_it->feeder;
                  t.cdp_type = pstats_it->cdp_type;
                  t.global_ceil = pstats_it->global_ceil;
                  t.debt_ceiling = pstats_it->debt_ceiling;
                  t.stability_fee = pstats_it->stability_fee;
                  t.penalty_ratio = pstats_it->penalty_ratio;
                  t.liquid8_ratio = pstats_it->liquid8_ratio;
                  t.fee_balance = pstats_it->fee_balance;
                  t.total_stablecoin = pstats_it->total_stablecoin;
                  t.total_collateral = pstats_it->total_collateral;
               });
            // Or modify an existing one
            else
               stable.modify( stat_exist, same_payer, [&]( auto& t ) {
                  t.live = true;
                  t.last_vote = now();
                  t.tau = pstats_it->tau;
                  t.ttl = pstats_it->ttl;
                  t.beg = pstats_it->beg;
                  t.feeder = pstats_it->feeder;
                  t.global_ceil = pstats_it->global_ceil;
                  t.debt_ceiling = pstats_it->debt_ceiling;
                  t.stability_fee = pstats_it->stability_fee;
                  t.penalty_ratio = pstats_it->penalty_ratio;
                  t.liquid8_ratio = pstats_it->liquid8_ratio;
               });
         }
      }
      // Refund all voters' voting tokens
      accounts vacnts( _self, symbl.raw() );
      auto accounts_it = vacnts.begin();

      //TODO: refactor, unsustainable loop
      while ( accounts_it != vacnts.end() ) { 
         add_balance( accounts_it->owner, asset( accounts_it->balance.amount, IQ_SYMBOL ), IQ_NAME );   
         accounts_it = vacnts.erase( accounts_it );
      } 

      // Safely erase the proposal 
      if ( !settlement ) 
         pstable.erase( *pstats_it );
      propstable.erase( props_it );
   }
}

// Add price data to the price feed
ACTION bankcdp::upfeed( name feeder, asset price, symbol_code cdp_type, symbol symbl ) 
{  
   // Make sure the feeder is authenticated
   require_auth( feeder );

   // Make sure the symbol and its value are valid
   eosio_assert( price.is_valid(), "Invalid quantity" );
   eosio_assert( symbl.is_valid(), "Invalid symbol name" );
   eosio_assert( price.amount > 0, "Must use a positive quantity" );

   // If the feeder is not _self, do some further checks on the input
   if ( !has_auth( _self ) ) {
      // Get the CDP stats and make sure it exists
      stats stable( _self, _self.value );
      const auto& stats_it = stable.get( cdp_type.raw(), "CDP type does not exist" );

      // Double check that the CDP feeder matches the feeder calling this action
      eosio_assert( stats_it.feeder == feeder, "Account not authorized to be price feeder" );

      // Check that the stablecoin symbol provided and the asset price match those in the CDP type
      eosio_assert( stats_it.total_stablecoin.symbol == price.symbol, "Price symbol must match stablecoin symbol" );
      eosio_assert( stats_it.total_collateral.quantity.symbol == symbl, "Asset symbol must match collateral symbol" );
   }
   
   // Get the price feed data for the input symbol
   feeds feedstable( _self, _self.value );
   auto feeds_it = feedstable.find( symbl.code().raw() );

   // Update the price feed for an existing feeds table entry
   if ( feeds_it != feedstable.end() )
      feedstable.modify( feeds_it, feeder, [&]( auto& f ) {
         f.price.amount *= (17 / 20);
         f.price.amount += (3 / 20) * price.amount;
         f.stamp = now();
      });
   // Create a new feeds table entry
   else {
      feedstable.emplace( _self, [&]( auto& f ) {
         f.total = asset( 0, symbl );
         f.price = price;
         f.stamp = now();
      }); 
   }

}

// NEED DESCRIPTIONS FOR THE FUNCTIONS BELOW
// NEED DESCRIPTIONS FOR THE FUNCTIONS BELOW
// NEED DESCRIPTIONS FOR THE FUNCTIONS BELOW
// NEED DESCRIPTIONS FOR THE FUNCTIONS BELOW

// Deposit IQ into the contract?
// TODO: Not sure about the description here
ACTION bankcdp::deposit( name from, name to, asset quantity, string memo ) 
{  
   // Make sure the depositor is authenticated
   require_auth( from );

   // Get the contract information
   name contract = get_code();

   // Verify that the transfer parameters are valid
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( quantity.amount > 0, "Must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "Memo has more than 256 bytes" );

   // Verify that the sender is not _self
   if ( from != _self ) {
      // Assert that 'to' is the contract
      eosio_assert( to == _self, "Can only deposit to self" ); 
      if ( quantity.symbol == IQ_SYMBOL )
         eosio_assert ( contract == IQ_NAME, "IQ deposit must come from IQ contract" );

      // Add the balance to the contract
      // TODO: does this need a subtract from the sender as well?
      add_balance( from, quantity, contract );

      // Get the price feed
      feeds feedstable( _self, _self.value );
      const auto& feeds_it = feedstable.get( quantity.symbol.code().raw(), "Feed does not exist" );

      // Update the price feed total amount
      feedstable.modify( feeds_it, same_payer, [&]( auto& f ) 
      {  f.total += quantity; });
   }
}

// Transfer stablecoin
ACTION bankcdp::transfer( name from, name to, asset quantity, string memo )
{  
   // Make sure the depositor is authenticated
   require_auth( from );

   // Validate the sending parameters
   eosio_assert( is_account( to ), "To account does not exist");
   eosio_assert( from != to, "Cannot transfer to self" );
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( memo.size() <= 256, "Memo has more than 256 bytes" );
   eosio_assert( quantity.amount > 0, "Must transfer positive quantity" );

   // Subtract stablecoin from the 'from' and give it to the 'to'
   name contract = sub_balance( from, quantity );
   eosio_assert( contract == _self, "Can only transfer stablecoin");
   add_balance( to, quantity, contract);
}

// Withdraw IQ from the contract
ACTION bankcdp::withdraw( name owner, asset quantity, string memo ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );

   // Validate the parameters
   eosio_assert( quantity.is_valid(), "Invalid quantity" );
   eosio_assert( quantity.amount >= 0, "Must transfer positive quantity" );
   eosio_assert( memo.size() <= 256, "Memo has more than 256 bytes" );


   if ( quantity.amount == 0 )
      quantity = get_balance( _self, quantity.symbol.code(), owner );
   name code = sub_balance( owner, quantity );
   eosio_assert( owner != _self && code != _self, 
                 "self cannot withdraw, cannot withdraw into self" 
               ); feeds feedstable( _self, _self.value );
   const auto& feeds_it = feedstable.get( quantity.symbol.code().raw(), 
                                    "Feed does not exist" 
                                  );
   action(
      permission_level{ _self, name( "active" ) },
      code, name( "transfer" ),
      std::make_tuple( _self, owner, quantity, memo )
   ).send(); 
   feedstable.modify( feeds_it, same_payer, [&]( auto& f ) 
   {  f.total -= quantity; });
}

ACTION bankcdp::close( name owner, symbol_code symbl ) 
{  
   // Make sure the owner is authenticated
   require_auth( owner );
   accounts acnts( _self, symbl.raw() );
   auto accounts_it = acnts.get( owner.value, "No balance object found" );
   eosio_assert( accounts_it.balance.amount == 0, "Balance must be zero" );
   acnts.erase( accounts_it );
}

name bankcdp::sub_balance( name owner, asset value ) 
{  accounts from_acnts( _self, value.symbol.code().raw() );
   const auto& from_account_it = from_acnts.get( owner.value, "No balance object found" );
   eosio_assert( from_account_it.balance.amount >= value.amount, 
                 "overdrawn balance" 
               );
   from_acnts.modify( from_account_it, owner, [&]( auto& a ) 
   {  a.balance -= value; }); return from_account_it.code;
}

void bankcdp::add_balance( name owner, asset value, name code ) 
{  accounts to_acnts( _self, value.symbol.code().raw() );
   auto accounts_to_it = to_acnts.find( owner.value );
   
   if ( accounts_to_it == to_acnts.end() ) 
      to_acnts.emplace( _self, [&]( auto& a ) { 
         a.balance = value;
         a.owner = owner; 
         a.code = code; 
      });
   else {
      eosio_assert( accounts_to_it->code == code, "wrong contract" );
      to_acnts.modify( accounts_to_it, same_payer, [&]( auto& a ) 
      { a.balance += value; });
   }
}

//checking all transfers, and not only from EOS system token
extern "C" void apply( uint64_t receiver, uint64_t code, uint64_t action ) 
{  if ( action == "transfer"_n.value && code != receiver )
      eosio::execute_action( eosio::name(receiver), eosio::name(code), &bankcdp::deposit );
   if ( code == receiver )
      switch ( action ) {
         EOSIO_DISPATCH_HELPER( bankcdp, /*(give)*/(open)(close)(shut)(lock)(bail)(draw)(wipe)(settle)(vote)(propose)(referended)(liquify)(upfeed)(withdraw) )
      }
}