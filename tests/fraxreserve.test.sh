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
cleos set contract frax.reserve ../frax.reserve/

echo -e "${CYAN}-----------------------ADD TOKENS-----------------------${NC}"
cleos push action frax.reserve addtoken '["tethertether", "4,USDT"]' -p frax.reserve
cleos push action frax.reserve addtoken '["fraxtokenfxs", "4,FXS"]' -p frax.reserve

echo -e "${CYAN}-----------------------DEPOSIT TOKENS-----------------------${NC}"
OLD_BALANCE1=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
OLD_BALANCE2=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
OLD_BALANCE3=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')
OLD_BALANCE4=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')

cleos push action tethertether transfer '["dcbtestusera", "frax.reserve", "2000.0000 USDT", "deposit USDT"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action tethertether transfer '["dcbtestuserb", "frax.reserve", "3000.0000 USDT", "deposit USDT"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestusera", "frax.reserve", "1000.0000 FXS", "deposit FXS"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestuserb", "frax.reserve", "1000.0000 FXS", "deposit FXS"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")

NEW_BALANCE1=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
NEW_BALANCE2=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
NEW_BALANCE3=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')
NEW_BALANCE4=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')

assert $(bc <<< "$NEW_BALANCE1 - $OLD_BALANCE1 == 2000")
assert $(bc <<< "$NEW_BALANCE2 - $OLD_BALANCE2 == 3000")
assert $(bc <<< "$NEW_BALANCE3 - $OLD_BALANCE3 == 1000")
assert $(bc <<< "$NEW_BALANCE4 - $OLD_BALANCE4 == 1000")

echo -e "${CYAN}-----------------------DEPOSIT UNSUPPORTED TOKENS-----------------------${NC}"
cleos transfer dcbtestusera frax.reserve "1 EOS" "deposit EOS"
assert $(bc <<< "$? == 1")
cleos push action everipediaiq transfer '["dcbtestusera", "frax.reserve", "1000.000 IQ", "deposit IQ"]' -p dcbtestusera
assert $(bc <<< "$? == 1")
cleos push action fraxtokenfxs transfer '["dcbtestusera", "frax.reserve", "1000.0000 FRAX", "deposit FRAX"]' -p dcbtestusera
assert $(bc <<< "$? == 1")


echo -e "${CYAN}-----------------------SET FULLY-BACKED TARGET-----------------------${NC}"
cleos push action frax.reserve settarget '["2000.0000 USDT", "0.0000 FXS", 1e4]' -p frax.reserve
assert $(bc <<< "$? == 0")

echo -e "${CYAN}-----------------------BUY FRAX WITH USDT-----------------------${NC}"
OLD_FRAX_BALANCE1=$(cleos get table fraxtokenfxs dcbtestusera accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
OLD_FRAX_BALANCE2=$(cleos get table fraxtokenfxs dcbtestuserb accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
OLD_USDT_BALANCE1=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
OLD_USDT_BALANCE2=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')

cleos push action frax.reserve buyfrax '["dcbtestusera", "1000.0000 FRAX"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action frax.reserve buyfrax '["dcbtestuserb", "1000.0000 FRAX"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")

NEW_FRAX_BALANCE1=$(cleos get table fraxtokenfxs dcbtestusera accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
NEW_FRAX_BALANCE2=$(cleos get table fraxtokenfxs dcbtestuserb accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
NEW_USDT_BALANCE1=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')
NEW_USDT_BALANCE2=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep USDT | tr -d '"' | awk '{ print $2 }')

assert $(bc <<< "$NEW_FRAX_BALANCE1 - $OLD_FRAX_BALANCE1 == 1000")
assert $(bc <<< "$NEW_FRAX_BALANCE2 - $OLD_FRAX_BALANCE2 == 1000")
assert $(bc <<< "$OLD_USDT_BALANCE1 - $NEW_USDT_BALANCE1 == 1000")
assert $(bc <<< "$OLD_USDT_BALANCE2 - $NEW_USDT_BALANCE2 == 1000")

echo -e "${CYAN}-----------------------SET PARTIALLY-BACKED TARGET-----------------------${NC}"
cleos push action frax.reserve settarget '["4000.0000 USDT", "10.0000 FXS", 1e4]' -p frax.reserve
assert $(bc <<< "$? == 0")

echo -e "${CYAN}-----------------------BUY FRAX WITH USDT + FXS-----------------------${NC}"
OLD_FRAX_BALANCE1=$(cleos get table fraxtokenfxs dcbtestusera accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
OLD_FRAX_BALANCE2=$(cleos get table fraxtokenfxs dcbtestuserb accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
OLD_FXS_BALANCE1=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')
OLD_FXS_BALANCE2=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')

cleos push action --force-unique frax.reserve buyfrax '["dcbtestusera", "1000.0000 FRAX"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action --force-unique frax.reserve buyfrax '["dcbtestuserb", "1000.0000 FRAX"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")

NEW_FRAX_BALANCE1=$(cleos get table fraxtokenfxs dcbtestusera accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
NEW_FRAX_BALANCE2=$(cleos get table fraxtokenfxs dcbtestuserb accounts | jq '.rows[]' | grep FRAX | tr -d '"' | awk '{ print $2 }')
NEW_FXS_BALANCE1=$(cleos get table frax.reserve dcbtestusera accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')
NEW_FXS_BALANCE2=$(cleos get table frax.reserve dcbtestuserb accounts | jq '.rows[]' | grep FXS | tr -d '"' | awk '{ print $2 }')


assert $(bc <<< "$NEW_FRAX_BALANCE1 - $OLD_FRAX_BALANCE1 == 1000")
assert $(bc <<< "$NEW_FRAX_BALANCE2 - $OLD_FRAX_BALANCE2 == 1000")
assert $(bc <<< "$OLD_FXS_BALANCE1 > $NEW_FXS_BALANCE1")
assert $(bc <<< "$OLD_FXS_BALANCE2 > $NEW_FXS_BALANCE2")


echo -e "${CYAN}-----------------------SET BAD PRICE-----------------------${NC}"
cleos push action frax.reserve settarget '["2000.0000 USDT", "0.0000 FXS", 0]' -p frax.reserve
assert $(bc <<< "$? == 1")

echo -e "${CYAN}-----------------------BAD FRAX PURCHASES-----------------------${NC}"
# target already reached
cleos push action frax.reserve buyfrax '["dcbtestuserb", "100.0000 FRAX"]' -p dcbtestuserb
assert $(bc <<< "$? == 1")
# not enough USDT
cleos push action frax.reserve buyfrax '["dcbtestusera", "10000.0000 FRAX"]' -p dcbtestusera
assert $(bc <<< "$? == 1")
