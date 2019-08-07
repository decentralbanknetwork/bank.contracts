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
cleos push action frax.reserve addtoken '["fraxtokenfxs", "4,FRAX"]' -p frax.reserve
cleos push action frax.reserve addtoken '["fraxtokenfxs", "4,FXS"]' -p frax.reserve

echo -e "${CYAN}-----------------------DEPOSIT TOKENS-----------------------${NC}"
cleos push action tethertether transfer '["dcbtestuserb", "frax.reserve", "1000.0000 USDT", "deposit USDT"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")
cleos push action tethertether transfer '["dcbtestusera", "frax.reserve", "1000.0000 USDT", "deposit USDT"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestuserb", "frax.reserve", "1000.0000 FRAX", "deposit FRAX"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestusera", "frax.reserve", "1000.0000 FRAX", "deposit FRAX"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestuserb", "frax.reserve", "1000.0000 FXS", "deposit FXS"]' -p dcbtestuserb
assert $(bc <<< "$? == 0")
cleos push action fraxtokenfxs transfer '["dcbtestusera", "frax.reserve", "1000.0000 FXS", "deposit FXS"]' -p dcbtestusera
assert $(bc <<< "$? == 0")
