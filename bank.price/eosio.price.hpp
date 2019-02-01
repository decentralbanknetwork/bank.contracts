#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/time.hpp>

using namespace eosio;
using namespace std;

class [[eosio::contract("bank.price")]] bankprice : public contract {

private:

    struct [[eosio::table]] price_t {
        asset price;
        uint64_t primary_key() const { return price.symbol.code().raw(); }
    };

    typedef multi_index<"price"_n, price_t> prices_table;

public:

    using contract::contract;

    ACTION create(asset initprice);

    ACTION update(name updater, asset price);

};

// Table info from eosio.system contract
// Needed to determine top 21 producers
namespace eosiosystem {
   struct [[eosio::table, eosio::contract("eosio.system")]] producer_info {
      name                  owner;
      double                total_votes = 0;
      eosio::public_key     producer_key; /// a packed public key object
      bool                  is_active = true;
      std::string           url;
      uint32_t              unpaid_blocks = 0;
      time_point            last_claim_time;
      uint16_t              location = 0;

      uint64_t primary_key()const { return owner.value;                             }
      double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
      bool     active()const      { return is_active;                               }
      void     deactivate()       { producer_key = public_key(); is_active = false; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)(is_active)(url)
                        (unpaid_blocks)(last_claim_time)(location) )
   };

   typedef eosio::multi_index< "producers"_n, producer_info,
                               indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>  >
                             > producers_table;
}
