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

PRIVKEY1="5KQicGMZkZpzsgxUpzNAWAHR8FUKEq2kxAgMNDkaTZugMEYt3FM"
PUBKEY1="1FyoJ7xbfECbKAoxRgGHqNz5hpNSJY7edc"
PRIVKEY2="5JtGdsayiwpVQ9Z8X5BiDcPrvEQzopppd72bTNBskqMtcmpCpDj"
PUBKEY2="15HQkwuxBnQgPzG3UMtoz7qMrNw7B3uKax"

echo -e "${CYAN}-----------------------DEPLOY CONTRACT-----------------------${NC}"
cleos set contract bank.pay2key ../bank.pay2key/

echo -e "${CYAN}-----------------------CREATE TOKENS-----------------------${NC}"
cleos push action bank.pay2key create '["eosio.token", "4,BANK", 1]' -p bank.pay2key
cleos push action bank.pay2key create '["everipediaiq", "3,IQ", 2]' -p bank.pay2key

echo -e "${CYAN}-----------------------DEPOSIT TOKENS-----------------------${NC}"
cleos transfer dcbtestusera bank.pay2key "100 BANK" "$PUBKEY1"
assert $(bc <<< "$? == 0")
cleos push action everipediaiq transfer "[\"dcbtestuserb\", \"bank.pay2key\", \"1000.000 IQ\", \"$PUBKEY2\"]" -p dcbtestuserb
assert $(bc <<< "$? == 0")

echo -e "${CYAN}-----------------------TRANSFER TOKENS-----------------------${NC}"
NONCE1=$(cleos get table bank.pay2key 0 btcaccounts | jq ".rows[0].last_nonce")
NONCE1=$(echo "$NONCE1 + 1" | bc)
MEMO1="BANK transfer"
CHAIN1=0
SIG1=$(node pay2key.sign.js $CHAIN1 $PUBKEY1 $PUBKEY2 10000 10 $NONCE1 "$MEMO1" $PRIVKEY1)
cleos push action bank.pay2key transfer "[$CHAIN1, \"dcbtestusera\", \"$PUBKEY1\", \"$PUBKEY1\", \"$PUBKEY2\", \"1.0000 BANK\", \"0.0010 BANK\", $NONCE1, \"$MEMO1\", \"$SIG1\"]" -p dcbtestusera
assert $(bc <<< "$? == 0")

NONCE2=$(cleos get table bank.pay2key 1 btcaccounts | jq ".rows[0].last_nonce")
NONCE2=$(echo "$NONCE1 + 1" | bc)
MEMO2="IQ transfer"
CHAIN2=1
SIG2=$(node pay2key.sign.js $CHAIN2 $PUBKEY2 $PUBKEY1 20300 15 $NONCE2 "$MEMO2" $PRIVKEY2)
cleos push action bank.pay2key transfer "[$CHAIN2, \"dcbtestuserb\", \"$PUBKEY2\", \"$PUBKEY2\", \"$PUBKEY1\", \"20.300 IQ\", \"0.015 IQ\", $NONCE2, \"$MEMO2\", \"$SIG2\"]" -p dcbtestuserb
assert $(bc <<< "$? == 0")

echo -e "${CYAN}-----------------REPLAY PROTECTION (TRANSFER SHOULD FAIL)------------------${NC}"
# Amount and fee are multiplied by 10 because they IQ has 3 decimal places and BANK has 4
cleos push action bank.pay2key transfer "[$CHAIN2, \"dcbtestusera\", \"$PUBKEY1\", \"$PUBKEY1\", \"$PUBKEY2\", \"10.000 IQ\", \"0.010 IQ\", $NONCE1, \"$MEMO1\", \"$SIG1\"]" -p dcbtestusera
assert $(bc <<< "$? == 1")
