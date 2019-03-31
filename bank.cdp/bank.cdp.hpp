
/*
 * @ author
 *   richard tiutiun 
 * @ copyright 
 *   defined in ../README.md
 * @ file 
 *   contract header for CDP engine 
 */
#pragma once

#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

using namespace eosio;
using std::make_tuple;
using std::string;

CONTRACT bank.cdp : public contract 
{  using contract::contract;
   public:
      static constexpr uint32_t SECYR = 10; //31557600; // TODO: Secs per avf year
      static constexpr uint32_t FEED_FRESH = 300; // Seconds in a 5 minute period
      static constexpr uint32_t VOTE_PERIOD = 5; //604800; //TODO: Secs per week
      const symbol IQ_SYMBOL = symbol( "IQ", 3 );
      const name IQ_NAME = name( "everipediaiq" );

      /* The owner of an urn can transfer
       * its ownership at any time using give.  
       */ //ACTION give( name giver, name taker, symbol_code symbl );

      /* Global settlement:
       * The system uses the market price of collateral 
       * at the time of settlement to compute how much
       * collateral each user gets. 
       */ ACTION settle( name feeder, symbol_code symbl );

      /* Publish a proposal that either motions for the 
       * enactment of a new CDP type, or modification
       * of an existing one.
       */ ACTION propose( name proposer, symbol_code symbl, 
                          symbol clatrl, symbol_code stabl,
                          uint64_t max, uint64_t gmax, 
                          uint64_t pen, uint64_t fee,
                          uint64_t beg, uint64_t liq, 
                          uint32_t tau, uint32_t ttl,
                          name feeder, name contract );

      /* Take a position (for/against) on a CDP proposal
       */ ACTION vote( name voter, symbol_code symbl, 
                       bool against, asset quantity );

      /* Called either automatically after VOTE_PERIOD from the moment
       * of proposal via "propose", via deferred action with proposer's 
       * auth, or (if deferred action fails) may be called directly by
       * proposer after VOTE_PERIOD has passed. Proposal may be enacted
       * if there are more votes in favor than against. Cleanup must be
       * done regardless on relevant multi_index tables.
       */ ACTION referended( name proposer, symbol_code symbl );

      /* liquify action 
       * If the collateral value in a CDP drops below 150% of the outstanding Dai, 
       * the contract automatically sells enough of your collateral to buy back as
       * many Dai as you issued. The issued Dai is thus taken out of circulation. Similar to a margin call.
       */ ACTION liquify( name bidder, name owner, 
                          symbol_code symbl, asset bidamt );
      
      /* Unless CDP is in liquidation, 
       * its owner can use lock to lock
       * more collateral. 
       */ ACTION lock( name owner, symbol_code symbl, asset quantity );

      /* Reclaims collateral from an overcollateralized CDP, 
       * without taking the CDP below its liquidation ratio. 
       */ ACTION bail( name owner, symbol_code symbl, asset quantity );
               
      /* Issue fresh stablecoin from this CDP.
       */ ACTION draw( name owner, symbol_code symbl, asset quantity );

      /* Owner can send back dai and reduce 
       * the CDP's issued stablecoin balance.
       */ ACTION wipe( name owner, symbol_code symbl, asset quantity );            

      /* Owner can close this CDP, if the price feed
       * is up to date and the CDP is not in liquidation. 
       * Reclaims all collateral and cancels all issuance
       * plus fee. 
       */ ACTION shut( name owner, symbol_code symbl );

      /* Price feed data update action
       */ ACTION upfeed( name feeder, asset price, 
                         symbol_code cdp_type, symbol symbl );

      // Open CDP or initiate balances for account
      ACTION open( name owner, symbol_code symbl, name ram_payer );

      ACTION withdraw( name owner, asset quantity, string memo );

      // Close balance of given symbol for account
      ACTION close( name owner, symbol_code symbl );
      
      ACTION deposit( name from, name to,
                      asset quantity, string memo );

      ACTION transfer( name from, name to,
                       asset quantity, string memo );

      static asset get_balance( name contract, symbol_code sym_code, name owner )
      {  accounts accountstable( contract, sym_code.raw() );
         const auto& ac = accountstable.get( owner.value );
         return ac.balance;
      }

   private:
      TABLE bid //SCOPE: CDP type symbol
      {  name     owner;
         name     bidder;
         asset    bidamt;
         
         uint32_t started;
         uint32_t lastbid;

         uint64_t primary_key() const { return owner.value; }
      }; typedef  eosio::multi_index< "bid"_n, bid > bids;

      TABLE cdp // SCOPE: CDP type symbol
      {  name     owner;
         uint32_t created;
         asset    collateral; // Amount of collateral currently locked by this CDP
         asset    stablecoin; // Stablecoin issued
         bool     live = true; // In liquidation or not

         uint64_t primary_key() const { return owner.value; }
      }; typedef  eosio::multi_index< "cdp"_n, cdp > cdps;

      TABLE prop
      {  symbol_code cdp_type;
         uint32_t    deadline;
         name        proposer;
         asset       yay;
         asset       nay;
         
         uint64_t    primary_key() const { return cdp_type.raw(); }
      }; typedef     eosio::multi_index< "prop"_n, prop > props;

      TABLE stat //SCOPE: Either _self or proposer
      {  bool           live = false;
         uint32_t       last_vote = 0; 
         uint32_t       tau; // Auctions will terminate after tau seconds have passed from start
         uint32_t       ttl; // And after ttl seconds have passed since placement of last bid
         
         //TODO: Feeders could be vector of accounts
         name           feeder; // Designated account for providing price feeds
         symbol_code    cdp_type;  // CDT Type Symbol e.g... DBEOS / DBKARMA...24*23*22*21*20 = over 5M different variants
         uint64_t       global_ceil; // Max total stablecoin issuable by all CDPs of this type
         uint64_t       debt_ceiling; 
         uint64_t       stability_fee; // Units are APR %
         uint64_t       beg; // Minimum bid increase in %
         uint64_t       penalty_ratio; // By default, 13% of the collateral in the CDP. Units are %
         uint64_t       liquid8_ratio; // Collateral asset units needed per issued stable tokens. Units are %
         asset          fee_balance; // Total stability fees paid
         asset          total_stablecoin; // Total amount of stablecoin in circulation for CDPs of this type
         extended_asset total_collateral; // Total amount locked as collateral for all CDPs of this type
         
         uint64_t       primary_key()const { return cdp_type.raw(); }
      }; typedef        eosio::multi_index< "stat"_n, stat > stats;
      
      TABLE feed
      {  asset    total; // Amount in circulation, not locked in CDPs
         asset    price;
         uint32_t stamp;
         
         uint64_t primary_key() const { return total.symbol.code().raw(); }
      }; typedef  eosio::multi_index< "feed"_n, feed > feeds;

      TABLE account //SCOPE: Balance symbol
      {  asset    balance; // Can be proposed CDP type symbol voted on
         name     owner;
         name     code;
         
         uint64_t primary_key() const { return owner.value; }
      }; typedef  eosio::multi_index< "accounts"_n, account > accounts;

      name sub_balance( name owner, asset value ); // Returns balance contract
      void add_balance( name owner, asset value, name code );
};
