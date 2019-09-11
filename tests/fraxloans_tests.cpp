#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include "contracts.hpp"
#include "token_test_api.hpp"
#include "fraxloans_api.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

#define SUCCESS(call) BOOST_REQUIRE_EQUAL(success(), call)
#define ERROR(msg, call) BOOST_REQUIRE_EQUAL(wasm_assert_msg(msg), call)

class fraxloans_tester: public tester
{
protected:
  token_test_api fraxtokens;
  token_test_api tether;
  token_test_api eos;
  fraxloans_api fraxloans;
public:
	fraxloans_tester(): 
        fraxtokens(N(frax.token), this), 
        tether(N(tethertether), this), 
        eos(N(eosio.token), this), 
        fraxloans(N(frax.loans), this)
	{
		create_accounts({ N(alice), N(bob), N(carol) });
	  	produce_blocks(2);
	  	init_tokens();
	}

private:
	void init_tokens()
	{
	  SUCCESS(eos.push_action(eos.contract, eos.contract, N(create), mvo()
		  ("issuer", eos.contract)
		  ("maximum_supply", asset::from_string("10000000000.0000 EOS"))
	  ));

	  SUCCESS(eos.push_action(eos.contract, eos.contract, N(issue), mvo()
		  ("to", eos.contract)
		  ("quantity", asset::from_string("100000.0000 EOS"))
		  ("memo", "")
	  ));

	  SUCCESS(tether.push_action(tether.contract, tether.contract, N(create), mvo()
		  ("issuer", tether.contract)
		  ("maximum_supply", asset::from_string("10000000000.0000 USDT"))
	  ));

	  SUCCESS(tether.push_action(tether.contract, tether.contract, N(issue), mvo()
		  ("to", tether.contract)
		  ("quantity", asset::from_string("100000.0000 USDT"))
		  ("memo", "")
	  ));

	  SUCCESS(fraxtokens.push_action(fraxtokens.contract, fraxtokens.contract, N(create), mvo()
		  ("issuer", fraxtokens.contract)
		  ("maximum_supply", asset::from_string("100000.0000 FRAX"))
	  ));

	  SUCCESS(fraxtokens.push_action(fraxtokens.contract, fraxtokens.contract, N(create), mvo()
		  ("issuer", fraxtokens.contract)
		  ("maximum_supply", asset::from_string("100000.0000 FXS"))
	  ));

	  SUCCESS(fraxtokens.push_action(fraxtokens.contract, fraxtokens.contract, N(issue), mvo()
		  ("to", fraxtokens.contract)
		  ("quantity", asset::from_string("100000.0000 FRAX"))
		  ("memo", "")
	  ));

	  SUCCESS(fraxtokens.push_action(fraxtokens.contract, fraxtokens.contract, N(issue), mvo()
		  ("to", fraxtokens.contract)
		  ("quantity", asset::from_string("100000.0000 FXS"))
		  ("memo", "")
	  ));

	  SUCCESS(fraxtokens.transfer(fraxtokens.contract, N(alice), asset::from_string("1000.0000 FRAX")));
	  SUCCESS(fraxtokens.transfer(fraxtokens.contract, N(bob), asset::from_string("1000.0000 FRAX")));
	  SUCCESS(fraxtokens.transfer(fraxtokens.contract, N(carol), asset::from_string("1000.0000 FRAX")));
	  SUCCESS(fraxtokens.transfer(fraxtokens.contract, N(alice), asset::from_string("1000.0000 FXS")));
	  SUCCESS(fraxtokens.transfer(fraxtokens.contract, N(bob), asset::from_string("1000.0000 FXS")));
	  SUCCESS(fraxtokens.transfer(fraxtokens.contract, N(carol), asset::from_string("1000.0000 FXS")));
	  SUCCESS(eos.transfer(eos.contract, N(alice), asset::from_string("100.0000 EOS")));
	  SUCCESS(eos.transfer(eos.contract, N(bob), asset::from_string("100.0000 EOS")));
	  SUCCESS(eos.transfer(eos.contract, N(carol), asset::from_string("100.0000 EOS")));

      // enable tokens in frax loans module
      SUCCESS(fraxloans.addtoken(fraxtokens.contract, "4,FRAX"));
      SUCCESS(fraxloans.addtoken(fraxtokens.contract, "4,FXS"));
      SUCCESS(fraxloans.addtoken(tether.contract,  "4,USDT"));
      SUCCESS(fraxloans.addtoken(eos.contract,  "4,EOS"));

      SUCCESS(fraxtokens.transfer(N(alice), fraxloans.contract,  asset::from_string("1000.0000 FRAX")));
      SUCCESS(fraxtokens.transfer(N(bob), fraxloans.contract,  asset::from_string("1000.0000 FRAX")));
      SUCCESS(fraxtokens.transfer(N(alice), fraxloans.contract,  asset::from_string("1000.0000 FXS")));
      SUCCESS(fraxtokens.transfer(N(bob), fraxloans.contract,  asset::from_string("1000.0000 FXS")));

	}
};

BOOST_AUTO_TEST_SUITE(fraxloans_tests)

BOOST_FIXTURE_TEST_CASE(deposit_test, fraxloans_tester) try {
	auto alice_frax = fraxloans.get_account(N(alice), "4,FRAX");
	auto bob_fxs = fraxloans.get_account(N(bob), "4,FXS");
	REQUIRE_MATCHING_OBJECT(bob_fxs, mvo()("balance", "1000.0000 FRAX"));
	REQUIRE_MATCHING_OBJECT(bob_fxs, mvo()("balance", "1000.0000 FXS"));
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
