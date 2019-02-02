#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

struct transfer_args {
    name   from;
    name   to;
    asset  quantity;
    std::string memo;
};

void safetransfer (const transfer_args& transfer) {
    if (transfer.to != "bank.safesnd"_n)
        eosio_exit(0);

    name to = name(transfer.memo);
    action(
        permission_level{ "bank.safesnd"_n, "active"_n },
        "bank.token", "transfer"_n,
        std::make_tuple( "bank.safesnd"_n, to, transfer.quantity, std::string("bank.safesnd") )
    ).send();
}

extern "C" {
    [[noreturn]] void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        eosio_assert( code == "bank.token"_n.value && action == "transfer"_n.value, "only bank.token transfers permitted");
        safetransfer( unpack_action_data<transfer_args>() );
        eosio_exit(0);
    }
}