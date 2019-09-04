#!/bin/bash

CYAN='\033[1;36m'
NC='\033[0m'

assert ()
{

    if [ $1 -eq 0 ]; then
        FAIL_LINE=$( caller | awk '{print $1}')
        echo -e "Assertion failed. Line $FAIL_LINE:"
        head -n $FAIL_LINE $BASH_SOURCE | tail -n 1
        exit 99
    fi
}

echo -e "${CYAN}-----------------------DEPLOY CONTRACT-----------------------${NC}"
cleos set contract frax.loans ../frax.loans/

echo -e "${CYAN}-----------------------ADD TOKENS-----------------------${NC}"
cleos push action frax.loans addtoken '["tethertether", "4,USDT"]' -p frax.loans
cleos push action frax.loans addtoken '["fraxtokenfxs", "4,FXS"]' -p frax.loans
cleos push action frax.loans addtoken '["fraxtokenfxs", "4,FRAX"]' -p frax.loans
cleos push action frax.loans addtoken '["everipediaiq", "3,IQ"]' -p frax.loans
cleos push action frax.loans addtoken '["eosio.token", "4,EOS"]' -p frax.loans

echo -e "${CYAN}-----------------------DEPOSIT TOKENS-----------------------${NC}"
OLD_BALANCE1=$(cleos get table frax.loans dcbtestusera accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
OLD_BALANCE2=$(cleos get table frax.loans dcbtestuserb accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
OLD_BALANCE3=$(cleos get table frax.loans dcbtestusera accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')
OLD_BALANCE4=$(cleos get table frax.loans dcbtestuserb accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')

cleos push action tethertether transfer '["dcbtestusera", "frax.loans", "2000.0000 USDT", "deposit USDT"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action tethertether transfer '["dcbtestuserb", "frax.loans", "3000.0000 USDT", "deposit USDT"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestusera", "frax.loans", "1000.0000 FXS", "deposit FXS"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestuserb", "frax.loans", "1000.0000 FXS", "deposit FXS"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")

NEW_BALANCE1=$(cleos get table frax.loans dcbtestusera accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
NEW_BALANCE2=$(cleos get table frax.loans dcbtestuserb accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
NEW_BALANCE3=$(cleos get table frax.loans dcbtestusera accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')
NEW_BALANCE4=$(cleos get table frax.loans dcbtestuserb accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')

assert $(bc <<< "$NEW_BALANCE1 - $OLD_BALANCE1 == 2000")
assert $(bc <<< "$NEW_BALANCE2 - $OLD_BALANCE2 == 3000")
assert $(bc <<< "$NEW_BALANCE3 - $OLD_BALANCE3 == 1000")
assert $(bc <<< "$NEW_BALANCE4 - $OLD_BALANCE4 == 1000")

echo -e "${CYAN}-----------------------DEPOSIT UNSUPPORTED TOKENS-----------------------${NC}"
cleos transfer dcbtestusera frax.loans "1 EOS" "deposit EOS"
assert $(bc <<< "$? == 1")
cleos push action everipediaiq transfer '["dcbtestusera", "frax.loans", "1000.000 IQ", "deposit IQ"]' -p dcbtestusera
assert $(bc <<< "$? == 1")
cleos push action fraxtokenfxs transfer '["dcbtestusera", "frax.loans", "1000.0000 FRAX", "deposit FRAX"]' -p dcbtestusera
assert $(bc <<< "$? == 1")
