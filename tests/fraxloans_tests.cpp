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
	  SUCCESS(eos.transfer(eos.contract, N(alice), asset::from_string("1000.0000 EOS")));
	  SUCCESS(eos.transfer(eos.contract, N(bob), asset::from_string("1000.0000 EOS")));
	  SUCCESS(eos.transfer(eos.contract, N(carol), asset::from_string("1000.0000 EOS")));
	  SUCCESS(tether.transfer(tether.contract, N(alice), asset::from_string("10000.0000 USDT")));
	  SUCCESS(tether.transfer(tether.contract, N(bob), asset::from_string("10000.0000 USDT")));
	  SUCCESS(tether.transfer(tether.contract, N(carol), asset::from_string("10000.0000 USDT")));

      // enable tokens in frax loans module
      SUCCESS(fraxloans.addtoken(fraxtokens.contract, "4,FRAX"));
      SUCCESS(fraxloans.addtoken(fraxtokens.contract, "4,FXS"));
      SUCCESS(fraxloans.addtoken(tether.contract,  "4,USDT"));
      SUCCESS(fraxloans.addtoken(eos.contract,  "4,EOS"));

	}
};

BOOST_AUTO_TEST_SUITE(fraxloans_tests)

BOOST_FIXTURE_TEST_CASE(fraxloans_full, fraxloans_tester) try {
    // deposit for supply
    SUCCESS(tether.transfer(N(carol), fraxloans.contract,  asset::from_string("1000.0000 USDT")));
    SUCCESS(eos.transfer(N(carol), fraxloans.contract,  asset::from_string("1000.0000 EOS")));
    SUCCESS(fraxtokens.transfer(N(alice), fraxloans.contract,  asset::from_string("1000.0000 FRAX")));
    SUCCESS(fraxtokens.transfer(N(bob), fraxloans.contract,  asset::from_string("1000.0000 FXS")));

    auto tether_stats = fraxloans.get_tokenstats("4,USDT");
    auto eos_stats = fraxloans.get_tokenstats("4,EOS");
    auto frax_stats = fraxloans.get_tokenstats("4,FRAX");
    auto fxs_stats = fraxloans.get_tokenstats("4,FXS");
    auto alice_frax = fraxloans.get_account(N(alice), "4,FRAX");
    auto carol_tether = fraxloans.get_account(N(carol), "4,USDT");
    auto carol_eos = fraxloans.get_account(N(carol), "4,EOS");
    auto bob_fxs = fraxloans.get_account(N(bob), "4,FXS");
	BOOST_REQUIRE_EQUAL(tether_stats["available"], "1000.0000 USDT");
	BOOST_REQUIRE_EQUAL(eos_stats["available"], "1000.0000 EOS");
	BOOST_REQUIRE_EQUAL(fxs_stats["available"], "1000.0000 FXS");
	BOOST_REQUIRE_EQUAL(frax_stats["available"], "1000.0000 FRAX");
	BOOST_REQUIRE_EQUAL(alice_frax["balance"], "1000.0000 FRAX");
	BOOST_REQUIRE_EQUAL(carol_tether["balance"], "1000.0000 USDT");
	BOOST_REQUIRE_EQUAL(carol_eos["balance"], "1000.0000 EOS");
	BOOST_REQUIRE_EQUAL(bob_fxs["balance"], "1000.0000 FXS");

    // deposit for collateral
    SUCCESS(eos.transfer(N(alice), fraxloans.contract,  asset::from_string("1000.0000 EOS")));
    SUCCESS(tether.transfer(N(bob), fraxloans.contract,  asset::from_string("2000.0000 USDT")));

    // set token prices
    SUCCESS(fraxloans.setprice(asset::from_string("1.000 FRAX")));
    SUCCESS(fraxloans.setprice(asset::from_string("3.500 EOS")));
    SUCCESS(fraxloans.setprice(asset::from_string("1.000 FXS")));
    SUCCESS(fraxloans.setprice(asset::from_string("1.000 USDT")));

    // borrow
    SUCCESS(fraxloans.borrow(N(alice), asset::from_string("1000.0000 USDT")));
    SUCCESS(fraxloans.borrow(N(bob), asset::from_string("300.0000 EOS")));

    auto alice_usdt = fraxloans.get_account(N(alice), "4,USDT");
    auto bob_eos = fraxloans.get_account(N(bob), "4,EOS");
	BOOST_REQUIRE_EQUAL(alice_usdt["balance"], "1000.0000 USDT");
	BOOST_REQUIRE_EQUAL(alice_usdt["borrowing"], "1000.0000 USDT");
	BOOST_REQUIRE_EQUAL(bob_eos["balance"], "300.0000 EOS");
	BOOST_REQUIRE_EQUAL(bob_eos["borrowing"], "300.0000 EOS");

    // accrue interest
    produce_blocks(1000);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
