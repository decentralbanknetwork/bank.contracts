#pragma once

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;

using mvo = fc::mutable_variant_object;
using action_result = base_tester::action_result;

class token_test_api {
public:
  account_name contract;

  token_test_api(account_name acnt, tester* tester) {
	contract = acnt;
	_tester = tester;

	_tester->create_accounts({contract});
	_tester->set_code(contract, contracts::token_wasm());
	_tester->set_abi(contract, contracts::token_abi().data());

	const auto &accnt = _tester->control->db().get<account_object, by_name>(contract);

	abi_def abi;
	BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
	abi_ser.set_abi(abi, abi_serializer_max_time);
  }

  action_result transfer(const name& from, const name& to, const asset& amount, const string memo = "")
  {
	return push_action(
		from, contract, N(transfer),
		mvo()("from", from)("to", to)("quantity", amount)("memo", memo));
  }

  fc::variant get_account(const account_name& acc, const string& symbolname)
  {
	auto symb = eosio::chain::symbol::from_string(symbolname);
	auto symbol_code = symb.to_symbol_code().value;
	vector<char> data = _tester->get_row_by_account( contract, acc, N(accounts), symbol_code );
	return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account", data, abi_serializer_max_time );
  }

  action_result push_action(const account_name &signer,
							const account_name &cnt,
							const action_name &name,
							const variant_object &data) {
	string action_type_name = abi_ser.get_action_type(name);
	action act;
	act.account = cnt;
	act.name = name;
	act.data = abi_ser.variant_to_binary(action_type_name, data, abi_serializer_max_time);

	return _tester->push_action(std::move(act), uint64_t(signer));
  }

private:
  abi_serializer abi_ser;
  tester* _tester;
  fc::microseconds abi_serializer_max_time{1000*1000};
};