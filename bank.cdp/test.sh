#!/bin/bash
# Usage: ./test.sh

# Assumptions: nodeos already running somewhere

# Run nodeos locally with command:
# nodeos -e -p eosio --http-validate-host=false --delete-all-blocks --contracts-console --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::producer_plugin --plugin eosio::http_plugin

#=================================================================================#
# Config Constants

CYAN='\033[1;36m'
NC='\033[0m'

# CHANGE PATH
EOSIO_CONTRACTS_ROOT=/home/ricardo/Documents/eosio.contracts/build

NODEOS_HOST="127.0.0.1"
NODEOS_PROTOCOL="http"
NODEOS_PORT="8888"
NODEOS_LOCATION="${NODEOS_PROTOCOL}://${NODEOS_HOST}:${NODEOS_PORT}"

alias cleos="cleos --url=${NODEOS_LOCATION}"

# Helper scripts
COMPILE="./compile.sh"

if [[ -z "${WALLET}" ]]; then
  echo "ERROR: run 'export WALLET=[wallet_name] to set your wallet name'"
  exit 1
else
  WALLET="${WALLET}"
fi

# Wallet password prereq...
# set PASSWORD environment variable via:
# export PASSWORD="PW5JjGjm4FjESLYWUqgpEP4sfVCFoCLfNRccSv2XZNd4cgBJBXnDV"

if [[ -z "${PASSWORD}" ]]; then
  echo "ERROR: run 'export PASSWORD=[password] to set your wallet password'"
  exit 1
else
  PASSWORD="${PASSWORD}"
fi
# 0) Unlock wallet
echo "=== UNLOCKING WALLET ==="
cleos wallet unlock -n $WALLET --password $PASSWORD

CONTRACT="bank.cdp"
CONTRACT_WASM="$CONTRACT.wasm"
CONTRACT_ABI="$CONTRACT.abi"

OWNER_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"
ACTIVE_ACCT="5J3TQGkkiRQBKcg8Gg2a7Kk5a2QAQXsyGrkCnnq4krSSJSUkW12"
# DAIQ Account public keys
OWNER_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"
ACTIVE_KEY="EOS6TnW2MQbZwXHWDHAYQazmdc3Sc1KGv4M9TSgsKZJSo43Uxs2Bx"

# 3) Compile DaIQ
echo -e "${CYAN}-----------------------COMPILING CONTRACT-----------------------${NC}"
"$COMPILE" $CONTRACT

# EOSIO system-related keys
echo -e "${CYAN}-----------------------SYSTEM KEYS-----------------------${NC}"
cleos wallet import -n $WALLET --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos wallet import -n $WALLET --private-key 5JgqWJYVBcRhviWZB3TU1tN9ui6bGpQgrXVtYZtTG2d3yXrDtYX
cleos wallet import -n $WALLET --private-key 5JjjgrrdwijEUU2iifKF94yKduoqfAij4SKk6X5Q3HfgHMS4Ur6
cleos wallet import -n $WALLET --private-key 5HxJN9otYmhgCKEbsii5NWhKzVj2fFXu3kzLhuS75upN5isPWNL
cleos wallet import -n $WALLET --private-key 5JNHjmgWoHiG9YuvX2qvdnmToD2UcuqavjRW5Q6uHTDtp3KG3DS
cleos wallet import -n $WALLET --private-key 5JZkaop6wjGe9YY8cbGwitSuZt8CjRmGUeNMPHuxEDpYoVAjCFZ
cleos wallet import -n $WALLET --private-key 5Hroi8WiRg3by7ap3cmnTpUoqbAbHgz3hGnGQNBYFChswPRUt26
cleos wallet import -n $WALLET --private-key 5JbMN6pH5LLRT16HBKDhtFeKZqe7BEtLBpbBk5D7xSZZqngrV8o
cleos wallet import -n $WALLET --private-key 5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC
cleos wallet import -n $WALLET --private-key 5Ju1ree2memrtnq8bdbhNwuowehZwZvEujVUxDhBqmyTYRvctaF

# Create system accounts
echo -e "${CYAN}-----------------------SYSTEM ACCOUNTS-----------------------${NC}"
cleos create account eosio eosio.bpay EOS7gFoz5EB6tM2HxdV9oBjHowtFipigMVtrSZxrJV3X6Ph4jdPg3
cleos create account eosio eosio.msig EOS6QRncHGrDCPKRzPYSiWZaAw7QchdKCMLWgyjLd1s2v8tiYmb45
cleos create account eosio eosio.names EOS7ygRX6zD1sx8c55WxiQZLfoitYk2u8aHrzUxu6vfWn9a51iDJt
cleos create account eosio eosio.ram EOS5tY6zv1vXoqF36gUg5CG7GxWbajnwPtimTnq6h5iptPXwVhnLC
cleos create account eosio eosio.ramfee EOS6a7idZWj1h4PezYks61sf1RJjQJzrc8s4aUbe3YJ3xkdiXKBhF
cleos create account eosio eosio.saving EOS8ioLmKrCyy5VyZqMNdimSpPjVF2tKbT5WKhE67vbVPcsRXtj5z
cleos create account eosio eosio.stake EOS5an8bvYFHZBmiCAzAtVSiEiixbJhLY8Uy5Z7cpf3S9UoqA3bJb
cleos create account eosio eosio.token EOS7JPVyejkbQHzE9Z4HwewNzGss11GB21NPkwTX2MQFmruYFqGXm
cleos create account eosio eosio.vpay EOS6szGbnziz224T1JGoUUFu2LynVG72f8D3UVAS25QgwawdH983U

# Bootstrap new system contracts
echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig/
cleos push action eosio.token create '[ "eosio", "100000000000.0000 EOS" ]' -p eosio.token
cleos push action eosio.token create '[ "eosio", "100000000000.0000 SYS" ]' -p eosio.token
echo -e "      EOS TOKEN CREATED"
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 EOS", "memo" ]' -p eosio
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 SYS", "memo" ]' -p eosio
echo -e "      EOS TOKEN ISSUED"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.bios/
echo -e "      BIOS SET"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.system/
echo -e "      SYSTEM SET"
cleos push action eosio setpriv '["eosio.msig", 1]' -p eosio@active
cleos push action eosio init '[0, "4,EOS"]' -p eosio@active
#cleos push action eosio init '[0, "4,SYS"]' -p eosio@active

# Deploy eosio.wrap
echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
cleos wallet import -n $WALLET --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio@active
cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/

# Import user keys
echo -e "${CYAN}-----------------------CONTRACT / USER KEYS-----------------------${NC}"
cleos wallet import -n $WALLET --private-key 5JVvgRBGKXSzLYMHgyMFH5AHjDzrMbyEPRdj8J6EVrXJs8adFpK # everipediaiq
## Make sure DAIQ keys are in the wallet
cleos wallet import -n $WALLET --private-key $OWNER_ACCT
cleos wallet import -n $WALLET --private-key $ACTIVE_ACCT

# Create Accounts
echo -e "${CYAN}-----------------------CONTRACT / USER ACCOUNTS-----------------------${NC}"
cleos system newaccount eosio everipediaiq EOS6XeRbyHP1wkfEvFeHJNccr4NA9QhnAr6cU21Kaar32Y5aHM5FP --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio $CONTRACT $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio fakeos $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos system newaccount eosio rick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio dick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio nick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio sick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio lick $OWNER_KEY --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

## Deploy contracts
echo -e "${CYAN}-----------------------DEPLOYING CONTRACTS-----------------------${NC}"
cleos set contract everipediaiq . everipediaiq.wasm everipediaiq.abi
cleos set contract $CONTRACT . $CONTRACT_WASM $CONTRACT_ABI
cleos set contract fakeos $EOSIO_CONTRACTS_ROOT/eosio.token/

cleos set account permission $CONTRACT active ./perm.json -p $CONTRACT@active
cleos set account permission everipediaiq active '{ "threshold": 1, "keys": [{ "key": "EOS6XeRbyHP1wkfEvFeHJNccr4NA9QhnAr6cU21Kaar32Y5aHM5FP", "weight": 1 }], "accounts": [{ "permission": { "actor":"everipediaiq","permission":"eosio.code" }, "weight":1 }] }' owner -p everipediaiq

# Transfer EOS to testing accounts
echo -e "${CYAN}-----------------------TRANSFERRING EOS-----------------------${NC}"

cleos push action eosio.token transfer '["eosio", "rick", "1000.0000 EOS", "memo"]' -p eosio
cleos push action eosio.token transfer '["eosio", "dick", "1000.0000 EOS", "memo"]' -p eosio
cleos push action eosio.token transfer '["eosio", "nick", "1000.0000 EOS", "memo"]' -p eosio
cleos push action eosio.token transfer '["eosio", "sick", "1000.0000 EOS", "memo"]' -p eosio

# Create and issue token
echo -e "${CYAN}-----------------------CREATING IQ TOKEN-----------------------${NC}"
cleos push action everipediaiq create '["everipediaiq", "100000000000.000 IQ"]' -p everipediaiq@active
cleos push action everipediaiq issue '["everipediaiq", "10000000000.000 IQ", "initial supply"]' -p everipediaiq@active

echo -e "${CYAN}-----------------------CREATING FAKE EOS-----------------------${NC}"
cleos push action fakeos create '[ "eosio", "10000000000.0000 EOS" ]' -p fakeos
cleos push action fakeos issue '[ "eosio", "1000000000.0000 EOS", "memo" ]' -p eosio

#=================================================================================#
# CREATE PRICE FEEDS
echo -e "${CYAN}-----------------------Creating daiq feeds-----------------------${NC}"

cleos push action $CONTRACT upfeed '["bank.cdp", "1.00 USD", "FUD", "4,EOS"]' -f -p bank.cdp
cleos push action $CONTRACT upfeed '["bank.cdp", "1.00 USD", "FUD", "3,IQ"]' -f -p bank.cdp
cleos push action $CONTRACT upfeed '["bank.cdp", "1.00 USD", "FUD", "2,USD"]' -f -p bank.cdp

# verify that price was updated
cleos get table $CONTRACT $CONTRACT feed

#=================================================================================#
# PERFORM TRANSFERS
echo -e "${CYAN}-----------------------MOVING TOKENS FROM THE CONTRACT TO SOME USERS -----------------------${NC}"

cleos push action fakeos transfer '["eosio", "lick", "1000.0000 EOS", "memo"]' -p eosio
cleos push action fakeos transfer '["lick", "bank.cdp", "100.0000 EOS", "test"]' -p lick

cleos push action everipediaiq transfer '["everipediaiq", "rick", "100000.000 IQ", "test"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dick", "100000.000 IQ", "test"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "nick", "100000.000 IQ", "test"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "sick", "100000.000 IQ", "test"]' -p everipediaiq

cleos push action everipediaiq transfer '["rick", "bank.cdp", "10000.000 IQ", "test"]' -p rick
cleos push action everipediaiq transfer '["dick", "bank.cdp", "10000.000 IQ", "test"]' -p dick
cleos push action everipediaiq transfer '["nick", "bank.cdp", "10000.000 IQ", "test"]' -p nick
cleos push action everipediaiq transfer '["sick", "bank.cdp", "10000.000 IQ", "test"]' -p sick

cleos push action eosio.token transfer '["rick", "bank.cdp", "100.0000 EOS", "test"]' -p rick
cleos push action eosio.token transfer '["dick", "bank.cdp", "100.0000 EOS", "test"]' -p dick
cleos push action eosio.token transfer '["nick", "bank.cdp", "100.0000 EOS", "test"]' -p nick
cleos push action eosio.token transfer '["sick", "bank.cdp", "100.0000 EOS", "test"]' -p sick

# verify that balances have entry for tracking tokens
cleos get table $CONTRACT "IQ" accounts 
cleos get table $CONTRACT "EOS" accounts
cleos get table $CONTRACT $CONTRACT feed

#=================================================================================#
# PROPOSE
echo -e "${CYAN}-----------------------Proposing new CDP types-----------------------${NC}"

cleos push action $CONTRACT propose '["rick", "FUD", "4,EOS", "USD", 100000, 100000000000, 50, 10, 20, 150, 30, 5, "dick", "eosio.token"]' -p rick
# proposal will get more no votes than yes
cleos push action $CONTRACT propose '["nick", "HEY", "4,EOS", "USD", 30000, 30000000000, 50, 10, 20, 150, 30, 5, "dick", "eosio.token"]' -p nick
# proposal will tie out and get re-voted
cleos push action $CONTRACT propose '["sick", "YOU", "4,EOS", "USD", 20000, 20000000000, 50, 10, 20, 150, 30, 5, "dick", "eosio.token"]' -p sick

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# proposal for this cdp type already in session
cleos push action $CONTRACT propose '["sick", "FUD", "4,EOS", "USD", 20000, 20000000000, 50, 10, 20, 150, 30, 5, "dick", "eosio.token"]' -p sick
# can't propose IQ symbol, or known stablecoin / cdp collateral
cleos push action $CONTRACT propose '["sick", "USD", "4,EOS", "USD", 20000, 20000000000, 50, 10, 20, 150, 30, 5, "dick", "eosio.token"]' -p sick
# bad parameters, eg liquidation ratio must be at least 100%
cleos push action $CONTRACT propose '["sick", "DUH", "4,EOS", "USD", 20000, 20000000000, 50, 10, 20, 1500, 30, 5, "dick", "eosio.token"]' -p sick
# can't propose global settlement for cdp type that doesnt exist
cleos push action $CONTRACT propose '["sick", "FUD", "4,EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick", "eosio.token"]' -p sick
# TRIVIAL
# invalid cdp type symbol
# ---------------------------------------------------- #

#=================================================================================#
# VOTE
echo -e "${CYAN}-----------------------Voting on proposals-----------------------${NC}"

# winner
cleos push action $CONTRACT vote '["rick", "FUD", true, "0.001 IQ"]' -p rick
cleos push action $CONTRACT vote '["nick", "FUD", false, "0.002 IQ"]' -p nick
# loser
cleos push action $CONTRACT vote '["rick", "HEY", true, "0.002 IQ"]' -p rick
cleos push action $CONTRACT vote '["nick", "HEY", false, "0.001 IQ"]' -p nick
# tie
cleos push action $CONTRACT vote '["rick", "YOU", true, "0.001 IQ"]' -p rick
cleos push action $CONTRACT vote '["nick", "YOU", false, "0.001 IQ"]' -p nick

# verify that balances have entry for tracking votes
cleos get table $CONTRACT "FUD" accounts
cleos get table $CONTRACT "HEY" accounts
cleos get table $CONTRACT "YOU" accounts
cleos get table $CONTRACT $CONTRACT prop

echo -e "${CYAN}----------------------Waiting for referendum-----------------------${NC}"
# verify that balances have entry for tracking votes
cleos get table $CONTRACT "FUD" accounts
cleos get table $CONTRACT "HEY" accounts
cleos get table $CONTRACT "YOU" accounts
cleos get table $CONTRACT $CONTRACT prop
# wait for referended to get called automatically via deferred transaction
sleep 5

# verify that YOU is still up for proposal
cleos get table $CONTRACT $CONTRACT prop

# for tie
cleos push action $CONTRACT vote '["rick", "YOU", false, "0.001 IQ"]' -p rick
sleep 5

# ----------------- SUPPOSED TO FAIL ----------------- #
# TRIVIAL
# not voting with the right symbol, governance token is IQ
# invalid cdp type symbol, or proposal for cdp type doesn't exist
# ---------------------------------------------------- #

# verify that the vote tokens were refunded to rick after referended
cleos get table $CONTRACT "FUD" accounts
cleos get table $CONTRACT "HEY" accounts
cleos get table $CONTRACT "YOU" accounts
# verify that prop entry was deleted
cleos get table $CONTRACT $CONTRACT prop
# verify that stat entry for this cdp type was referended into existence
cleos get table $CONTRACT $CONTRACT stat

#=================================================================================#
# PROPOSE GLOBAL SETTLEMENT - ON

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# unprivileged proposer (not feeder) can't propose global settlement for cdp type
cleos push action $CONTRACT propose '["rick", "YOU", "4,EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick", "eosio.token"]' -p rick
# ---------------------------------------------------- #

echo -e "${CYAN}-----------------------Proposing TURN ON global settlement-----------------------${NC}"

cleos push action $CONTRACT propose '["dick", "YOU", "4,EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick", "eosio.token"]' -p dick
cleos push action $CONTRACT vote '["rick", "YOU", false, "0.001 IQ" ]' -p rick

sleep 5

# verify that stat entry for this cdp type's live bool was flipped
cleos get table $CONTRACT $CONTRACT stat

#=================================================================================#
# CREATE EOS PRICE FEED by unauthorized account
echo -e "${CYAN}----------------------SUPPOSED TO FAIL-----------------------${NC}"
# rick is not the feeder account for FUD
cleos push action $CONTRACT upfeed '["rick", "1.00 USD", "FUD", "4,EOS"]' -f -p rick

#=================================================================================#
# OPEN
echo -e "${CYAN}-----------------------OPENING CDPS-----------------------${NC}"

cleos push action $CONTRACT open '["rick", "FUD", "rick"]' -p rick
cleos push action $CONTRACT open '["dick", "FUD", "dick"]' -p dick
cleos push action $CONTRACT open '["nick", "FUD", "nick"]' -p nick
cleos push action $CONTRACT open '["sick", "FUD", "sick"]' -p sick
cleos push action $CONTRACT open '["lick", "FUD", "lick"]' -p lick

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# cdp owner already has a cdp of this type
cleos push action $CONTRACT open '["rick", "FUD", "rick"]' -p rick
sleep 1
# cdp type not yet live, or in global settlement
cleos push action $CONTRACT open '["rick", "YOU", "rick"]' -p rick
# TRIVIAL
# price feed data doesn't exist or is too stale
# invalid cdp type symbol, or cdp type doesn't exist
# ---------------------------------------------------- #

# verify that dick's cdp was transfered back to rick
cleos get table $CONTRACT "FUD" cdp

#=================================================================================#
# PROPOSE GLOBAL SETTLEMENT - OFF then ON again
echo -e "${CYAN}----------------------- Proposing TURN OFF global settlement-----------------------${NC}"

cleos push action $CONTRACT propose '["dick", "YOU", "4,EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick", "eosio.token"]' -p dick
cleos push action $CONTRACT vote '["rick", "YOU", false, "0.001 IQ" ]' -p rick
sleep 5

cleos push action $CONTRACT open '["rick", "YOU", "rick"]' -p rick

echo -e "${CYAN}-----------------------Proposing TURN ON global settlement-----------------------${NC}"

cleos push action $CONTRACT propose '["dick", "YOU", "4,EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick", "eosio.token"]' -p dick
cleos push action $CONTRACT vote '["rick", "YOU", false, "0.001 IQ" ]' -p rick
sleep 5

#=================================================================================#
# LOCK
echo -e "${CYAN}-----------------------Push collateral into CDP-----------------------${NC}"

cleos push action $CONTRACT lock '["rick", "FUD", "6.0000 EOS"]' -p rick
cleos push action $CONTRACT lock '["dick", "FUD", "6.0000 EOS"]' -p dick
cleos push action $CONTRACT lock '["nick", "FUD", "6.0000 EOS"]' -p nick
cleos push action $CONTRACT lock '["sick", "FUD", "100.0000 EOS"]' -p sick

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# locking collateral from balance of contract not approved by cdp type
cleos push action $CONTRACT lock '["lick", "FUD", "6.0000 EOS"]' -p lick
# locking more collateral than owner's balance available
cleos push action $CONTRACT lock '["rick", "FUD", "60000000.0000 EOS"]' -p rick
# cdp is in liquidation, or cdp type not live yet / in global settlement
cleos push action $CONTRACT lock '["rick", "YOU", "6.0000 EOS"]' -p rick
# TRIVIAL
# invalid quantity symbol or amount
# locking more collateral than available in account balance
# invalid cdp type symbol; cdp type or cdp doesn't exist
# ---------------------------------------------------- #

cleos get table $CONTRACT $CONTRACT feed
# verify that rick's clatrl balance was updated
cleos get table $CONTRACT "EOS" accounts
# verify that clatrl amount was pushed
cleos get table $CONTRACT "FUD" cdp


#=================================================================================#
# DRAW
echo -e "${CYAN}-----------------------Pull Stablecoin from CDP-----------------------${NC}"

cleos push action $CONTRACT draw '["rick", "FUD", "4.00 USD"]' -p rick
cleos push action $CONTRACT draw '["dick", "FUD", "4.00 USD"]' -p dick
cleos push action $CONTRACT draw '["nick", "FUD", "4.00 USD"]' -p nick
cleos push action $CONTRACT draw '["sick", "FUD", "50.00 USD"]' -p sick

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# cdp is in liquidation, or cdp type not live yet / in global settlement
cleos push action $CONTRACT draw '["rick", "YOU", "4.00 USD"]' -p rick
# drawing this much debt will exceed liquidation ratio for this cdp type
cleos push action $CONTRACT draw '["rick", "FUD", "10.00 USD"]' -p rick
# drawing this much debt will surpass the cdp's debt ceiling
cleos push action $CONTRACT draw '["sick", "FUD", "200000.00 USD"]' -p sick
# TRIVIAL
# invalid quantity symbol or amount
# price feed data doesn't exist or is too stale
# invalid cdp type symbol; cdp type or cdp doesn't exist
# drawing this much debt will surpass the cdp type's total debt ceiling
# ---------------------------------------------------- #

# verify that stabl amount was pulled
cleos get table $CONTRACT "FUD" cdp
# verify that rick's stabl balance was updated
cleos get table $CONTRACT "USD" accounts

#=================================================================================#
# SHUT
echo -e "${CYAN}-----------------------Closing CDP-----------------------${NC}"

cleos push action $CONTRACT shut '["rick", "YOU"]' -p rick

#----------------SUPPOSED TO FAIL----------------------#
# TRIVIAL
# cdp is in liquidation
# invalid cdp type symbol; cdp type or cdp doesn't exist
# ---------------------------------------------------- #

# verify that rick's cdp was in fact shut
cleos get table $CONTRACT "FUD" cdp

#=================================================================================#
# WIPE 
echo -e "${CYAN}-----------------------Push Stablecoin into CDP-----------------------${NC}"

cleos push action $CONTRACT wipe '["rick", "FUD", "2.00 USD"]' -p rick
cleos push action $CONTRACT wipe '["dick", "FUD", "2.00 USD"]' -p dick
cleos push action $CONTRACT wipe '["nick", "FUD", "2.00 USD"]' -p nick

cleos push action $CONTRACT withdraw '["nick", "0.000 IQ", "test"]' -p nick

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# trying to wipe more debt than is outstanding in the cdp
cleos push action $CONTRACT wipe '["nick", "FUD", "20.00 USD"]' -p nick
# insufficient IQ balance available to pay fee, 0 will withdraw all
cleos push action $CONTRACT wipe '["nick", "FUD", "1.00 USD"]' -p nick
# TRIVIAL
# invalid wipe quantity symbol / amount not > 0
# invalid cdp type symbol; cdp type or cdp doesn't exist
# invalid fee quantity symbol (not IQ) / amount not > 0
# invalid wipe quantity symbol / amount not > 0
# ---------------------------------------------------- #

# verify that stabl amount was pushed
cleos get table $CONTRACT "FUD" cdp
#verify that rick's stabl balance was updated
cleos get table $CONTRACT "USD" accounts

#=================================================================================#
#BAIL
echo -e "${CYAN}-----------------------Pull collateral from cdp-----------------------${NC}"

cleos push action $CONTRACT bail '["rick", "FUD", "2.0000 EOS"]' -p rick
cleos push action $CONTRACT bail '["nick", "FUD", "2.0000 EOS"]' -p nick
cleos push action $CONTRACT bail '["dick", "FUD", "2.0000 EOS"]' -p dick

# verify that clatrl amount was pulled
cleos get table $CONTRACT "FUD" cdp
# verify that rick's clatrl balance was updated
cleos get table $CONTRACT "EOS" accounts

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# bailing this much collateral will exceed liquidation ratio for this cdp type
cleos push action $CONTRACT bail '["rick", "FUD", "3.0000 EOS"]' -p rick
# trying to bail more collateral than is contained in the cdp
cleos push action $CONTRACT bail '["rick", "FUD", "5.0000 EOS"]' -p rick
# TRIVIAL
# price feed data doesn't exist or is too stale
# invalid bail quantity symbol / amount not > 0
# invalid cdp type symbol; cdp type or cdp doesn't exist
# bail quantity symbol doesnt match cdp's collateral symbol
# ---------------------------------------------------- #

#=================================================================================#
# FAIL AUTOMATIC SETTLE
echo -e "${CYAN}------------------TOGGLE SETTLE OFF FIRST----------------------------${NC}"

cleos push action $CONTRACT propose '["dick", "YOU", "4,EOS", "USD", 0, 0, 0, 0, 0, 0, 0, 0, "dick", "eosio.token"]' -p dick
cleos push action $CONTRACT vote '["rick", "YOU", false, "0.001 IQ" ]' -p rick

cleos get table $CONTRACT $CONTRACT stat
sleep 5

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL - Automatic Global settlement-----------------------${NC}"
# brand new cdp type, no debt issued yet
cleos push action $CONTRACT settle '["dick", "YOU"]' -p dick
sleep 1

# liquidation ratio has not been exceeded
cleos push action $CONTRACT settle '["dick", "FUD"]' -p dick
# TRIVIAL
# cdp type already in global settlement
# no price feed for cdp type's collateral
# invalid cdp symbol, or cdp type doesn't exist
# ---------------------------------------------------- #

#=================================================================================#
# LIQUIFY
echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# liquidation ratio has not been exceeded 
cleos push action $CONTRACT liquify '["sick", "nick", "FUD", "2.25 USD"]' -p sick
# ---------------------------------------------------- #

echo -e "${CYAN}-----------------------Make EOS price go down....-----------------------${NC}"
# several times due to smoothing function
cleos push action $CONTRACT upfeed '["dick", "0.01 USD", "FUD", "4,EOS"]' -f -p dick
sleep 2
cleos push action $CONTRACT upfeed '["bank.cdp", "0.02 USD", "FUD", "4,EOS"]' -f -p bank.cdp
sleep 2
cleos push action $CONTRACT upfeed '["dick", "0.02 USD", "FUD", "4,EOS"]' -f -p dick
sleep 2
cleos push action $CONTRACT upfeed '["dick", "0.01 USD", "FUD", "4,EOS"]' -f -p dick

# AUTOMATIC SETTLE
echo -e "${CYAN}-----------------------Automatic Global settlement-----------------------${NC}"

cleos push action $CONTRACT settle '["dick", "FUD"]' -p dick
#verify that FUD cdp type is not live
cleos get table $CONTRACT $CONTRACT stat

echo -e "${CYAN}-----------------------Liquifying rick's cdp-----------------------${NC}"
# verify balances before liquidation
cleos get table $CONTRACT "USD" accounts
cleos get table $CONTRACT "FUD" cdp
cleos get table $CONTRACT "EOS" accounts
cleos get table $CONTRACT "IQ" accounts

##### ROUND 1 #####

# will go for..
# just one round, remainder 0.75
cleos push action $CONTRACT liquify '["sick", "nick", "FUD", "2.25 USD"]' -p sick
# two rounds, remainder 0.75
cleos push action $CONTRACT liquify '["sick", "rick", "FUD", "2.25 USD"]' -p sick
# all three rounds, remainder 0.75
cleos push action $CONTRACT liquify '["sick", "dick", "FUD", "2.25 USD"]' -p sick

echo -e "${CYAN}-----------------------SUPPOSED TO FAIL-----------------------${NC}"
# bid quantity amount is not at least beg% greater/eq than last bid
cleos push action $CONTRACT liquify '["sick", "dick", "FUD", "2.27 USD"]' -p sick
# bid quantity symbol is not the same as cdp's debt symbol
cleos push action $CONTRACT liquify '["sick", "dick", "FUD", "2.400 IQ"]' -p sick
# TRIVIAL
# price feed data doesn't exist or is too stale
# invalid cdp type symbol; cdp type or cdp doesn't exist
# invalid bid quantity symbol invalid / amount not > 0
# ---------------------------------------------------- #

# for one round, remainder 0
cleos push action $CONTRACT liquify '["sick", "nick", "FUD", "3.00 USD"]' -p sick
# for two rounds, remainder 0.25
cleos push action $CONTRACT liquify '["sick", "rick", "FUD", "2.75 USD"]' -p sick
# for three rounds, remainder will be 0.30
cleos push action $CONTRACT liquify '["sick", "dick", "FUD", "2.70 USD"]' -p sick

# wait for round to expire and switch to selling off IQ for the remaining balance
echo "=== First round auction done, waiting for round to expire... ==="
cleos get table $CONTRACT "FUD" cdp
cleos get table $CONTRACT "FUD" bid
sleep 5

##### ROUND 2 #####

# for two rounds, remainder 0.05
cleos push action $CONTRACT liquify '["sick", "rick", "FUD", "0.20 USD"]' -p sick
# for three rounds, overpaid by 0.10
cleos push action $CONTRACT liquify '["sick", "dick", "FUD", "0.40 USD"]' -p sick

echo "=== Second round auction done, waiting for round to expire... ==="
sleep 5
cleos get table $CONTRACT "FUD" cdp
cleos get table $CONTRACT "FUD" bid

cleos push action $CONTRACT liquify '["sick", "rick", "FUD", "0.05 USD"]' -p sick

##### ROUND 3 #####
# auction covered more than the cdp debt, proceed to take bids in IQ for the diff

cleos push action $CONTRACT liquify '["sick", "dick", "FUD", "0.100 IQ"]' -p sick
sleep 5

# TODO covered not enough in round 2
# covered not enough in round 3 
# covered too much in round 3 

echo "=== Third round auction done, balance closed ==="

#verify that bid entry and cdp were in fact erased
cleos get table $CONTRACT "FUD" cdp
cleos get table $CONTRACT "FUD" bid
#verify rick's balances after liquidation
cleos get table $CONTRACT "USD" accounts
cleos get table $CONTRACT "EOS" accounts
cleos get table $CONTRACT "IQ" accounts

echo -e "${CYAN}-----------------------COMPLETE-----------------------${NC}"

# Exit on success
exit 0