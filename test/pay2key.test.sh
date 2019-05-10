#!/bin/bash

CYAN='\033[1;36m'
NC='\033[0m'

PRIVKEY1="5JW2ZpYaHA96yEpDXW1arVmEezV3XPF6aD2b7BB5EyGUEapdUTL"
PUBKEY1="EOS6u25uWXi52QLp1FurM8KPDJydbvxbrPM2XNrYPz7vPCQuGFCer"
PRIVKEY2="5KRKuMUFswLXmeA9wzWvn6f9a9fNH36oGkVG29X2VGpbMPCJ3eC"
PUBKEY2="EOS6wh7kToXbq6rt92bFVFz2mj9AcJf8NEFK3sFfYk78QTnEGAiBp"

echo -e "${CYAN}-----------------------DEPLOY CONTRACT-----------------------${NC}"
cleos set contract bank.pay2key ../bank.pay2key/

echo -e "${CYAN}-----------------------CREATE TOKENS-----------------------${NC}"
cleos push action bank.pay2key create '["eosio.token", "4,BANK", 1]' -p bank.pay2key
cleos push action bank.pay2key create '["everipediaiq", "3,IQ", 2]' -p bank.pay2key

echo -e "${CYAN}-----------------------DEPOSIT TOKENS-----------------------${NC}"
cleos transfer dcbtestusera bank.pay2key "1000 BANK" "$PUBKEY1"
cleos push action everipediaiq transfer "[\"dcbtestuserb\", \"bank.pay2key\", \"1000.000 IQ\", \"$PUBKEY2\"]" -p dcbtestuserb

echo -e "${CYAN}-----------------------TRANSFER TOKENS-----------------------${NC}"
