#!/bin/bash

#Kedar Iyer (mostly) and Travis Moore

CYAN='\033[1;36m'
NC='\033[0m'

trap ctrl_c INT
function ctrl_c {
    exit 11;
}

RECOMPILE_AND_RESET_EOSIO_CONTRACTS=0
HELP=0
EOSIO_CONTRACTS_ROOT=/home/kedar/eosio.contracts/build/contracts/
NODEOS_HOST="127.0.0.1"
NODEOS_PROTOCOL="http"
NODEOS_PORT="8888"
NODEOS_LOCATION="${NODEOS_PROTOCOL}://${NODEOS_HOST}:${NODEOS_PORT}"

alias cleos="cleos --url=${NODEOS_LOCATION}"

#######################################
## HELPERS

# function balance {
#     cleos get table bank.token $1 accounts | jq ".rows[0].balance" | tr -d '"' | awk '{print $1}'
# }

assert ()
{

    if [ $1 -eq 0 ]; then
        FAIL_LINE=$( caller | awk '{print $1}')
        echo -e "Assertion failed. Line $FAIL_LINE:"
        head -n $FAIL_LINE $BASH_SOURCE | tail -n 1
        exit 99
    fi
}

ipfsgen () {
    echo -e "Qm$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 44 | head -n 1)"
}

# Don't allow tests on mainnet
CHAIN_ID=$(cleos get info | jq ".chain_id" | tr -d '"')
if [ $CHAIN_ID = "aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906" ]; then
    >&2 echo -e "Cannot run test on mainnet"
    exit 1
fi

# Create BIOS accounts
rm ~/eosio-wallet/default.wallet
cleos wallet create --to-console

# EOSIO system-related keys
echo -e "${CYAN}-----------------------SYSTEM KEYS-----------------------${NC}"
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos wallet import --private-key 5JgqWJYVBcRhviWZB3TU1tN9ui6bGpQgrXVtYZtTG2d3yXrDtYX
cleos wallet import --private-key 5JjjgrrdwijEUU2iifKF94yKduoqfAij4SKk6X5Q3HfgHMS4Ur6
cleos wallet import --private-key 5HxJN9otYmhgCKEbsii5NWhKzVj2fFXu3kzLhuS75upN5isPWNL
cleos wallet import --private-key 5JNHjmgWoHiG9YuvX2qvdnmToD2UcuqavjRW5Q6uHTDtp3KG3DS
cleos wallet import --private-key 5JZkaop6wjGe9YY8cbGwitSuZt8CjRmGUeNMPHuxEDpYoVAjCFZ
cleos wallet import --private-key 5Hroi8WiRg3by7ap3cmnTpUoqbAbHgz3hGnGQNBYFChswPRUt26
cleos wallet import --private-key 5JbMN6pH5LLRT16HBKDhtFeKZqe7BEtLBpbBk5D7xSZZqngrV8o
cleos wallet import --private-key 5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC
cleos wallet import --private-key 5Ju1ree2memrtnq8bdbhNwuowehZwZvEujVUxDhBqmyTYRvctaF

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

if [ $RECOMPILE_AND_RESET_EOSIO_CONTRACTS -eq 1 ]; then
    # Compile the contracts manually due to an error in the build.sh script in eosio.contracts
    echo -e "${CYAN}-----------------------RECOMPILING CONTRACTS-----------------------${NC}"
    cd "${EOSIO_CONTRACTS_ROOT}/eosio.token"
    eosio-cpp -I ./include -o ./eosio.token.wasm ./src/eosio.token.cpp --abigen
    cd "${EOSIO_CONTRACTS_ROOT}/eosio.msig"
    eosio-cpp -I ./include -o ./eosio.msig.wasm ./src/eosio.msig.cpp --abigen
    cd "${EOSIO_CONTRACTS_ROOT}/eosio.system"
    eosio-cpp -I ./include -I "${EOSIO_CONTRACTS_ROOT}/eosio.token/include" -o ./eosio.system.wasm ./src/eosio.system.cpp --abigen
    cd "${EOSIO_CONTRACTS_ROOT}/eosio.wrap"
    eosio-cpp -I ./include -o ./eosio.wrap.wasm ./src/eosio.wrap.cpp --abigen
fi

# Bootstrap new system contracts
echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig/
cleos push action eosio.token create '[ "eosio", "10000000000.0000 EOS" ]' -p eosio.token
echo -e "      EOS TOKEN CREATED"
cleos push action eosio.token issue '[ "eosio", "1000000000.0000 EOS", "memo" ]' -p eosio
echo -e "      EOS TOKEN ISSUED"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.system/
echo -e "      SYSTEM SET"
cleos push action eosio setpriv '["eosio.msig", 1]' -p eosio@active
cleos push action eosio init '[0, "4,EOS"]' -p eosio@active

# Import user keys
echo -e "${CYAN}-----------------------USER KEYS-----------------------${NC}"
cleos wallet import --private-key 5JVvgRBGKXSzLYMHgyMFH5AHjDzrMbyEPRdj8J6EVrXJs8adFpK
cleos wallet import --private-key 5KBhzoszXcrphWPsuyTxoKJTtMMcPhQYwfivXxma8dDeaLG7Hsq
cleos wallet import --private-key 5J9UYL9VcDfykAB7mcx9nFfRKki5djG9AXGV6DJ8d5XPYDJDyUy
cleos wallet import --private-key 5HtnwWCbMpR1ATYoXY4xb1E4HAU9mzGvDrawyK5May68cYrJR7r
cleos wallet import --private-key 5Jjx6z5SJ7WhVU2bgG2si6Y1up1JTXHj7qhC9kKUXPXb1K1Xnj6 # dcbtestusera
cleos wallet import --private-key 5HyQUNxE9T83RLiS9HdZeJck5WRqNSSzVztZ3JwYvkYPrG8Ca1U # dcbtestuserb
cleos wallet import --private-key 5KZC9soBHR4AF1kt93pCNfjSLPJN9y51AKR4r4vvPsiPvFdLG3t # dcbtestuserc
cleos wallet import --private-key 5K9dtgQXBCggrMugEAxsBfcUZ8mmnbDpiZZYt7RvoxwChqiFdS1 # dcbtestuserd
cleos wallet import --private-key 5JU8qQMV3cD4HzA14meGEBWwWxNWAk9QAebSkQotv4wXHkKncNh # dcbtestusere
cleos wallet import --private-key 5K8juhLvjpPHDX1VkueBYKi2EeUVWMHACed9HF39pp6ussah3ig # bank.pay2key
cleos wallet import --private-key 5Js2o8RLjC3PcsEEEdprpgNEg4ZfjUQdiL2FHheWFFKgb4QSMX9 # everipediaiq
cleos wallet import --private-key 5JmBnFBTBHAuhgX7iAxNnb5SEUpyBiYqNrh789XWBujN6iXU8dG # frax.reserve
cleos wallet import --private-key 5Jka9J9sthYAGHAJYJEYVuZSSnuTgpqV5WMaMMxow5MRqDUWRGt # tethertether
cleos wallet import --private-key 5Jn4z4UhDh68GZREnDpBtMXPAx96d3CX7mLgSbUTca4eEUZNxho # fraxtokenfxs
cleos wallet import --private-key 5HpMNASust7Negki66DJ5benpQAsXBRM3jDpHe8eREddYctA5mF # frax.loans

# Create user accounts
echo -e "${CYAN}-----------------------USER ACCOUNTS-----------------------${NC}"
cleos system newaccount eosio bank.shares EOS6XeRbyHP1wkfEvFeHJNccr4NA9QhnAr6cU21Kaar32Y5aHM5FP --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bank.price EOS8dYVzNktdam3Vn31mSXcmbj7J7MzGNudqKb3MLW1wdxWJpEbrw --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bank.safesnd EOS68s2PrHPDeGWTKczrNZCn4MDMgoW6SFHuTQhXYUNLT1hAmJei8 --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bank.pay2key EOS65yGjjyeyduJMGpQMGy39NcJenXFQ52HgroYos4dwjbwxu5TTW --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio frax.reserve EOS6jA39vSMGVHVQzxhYhE9gMToCYG9U9QLZiWZschCgfQuKJ6nrF --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio frax.loans EOS6DcmhfH9RSEftsRa6k8uH7E76sLRtj3hFPyVRLoNDEBdZE1anc --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio tethertether EOS6RAcCwqUBjPzJrN5vYBF7JLDrcEQEWnD5mnr1NSvYaEuJ5DUZK --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio everipediaiq EOS5Fpz9W4xkBGrvAAm3FERwcBTGzafeQkAzY6CeAEaxX3CedPL4N --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio dcbtestusera EOS7LpZDPKwWWXgJnNYnX6LCBgNqCEqugW9oUQr7XqcSfz7aSFk8o --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio dcbtestuserb EOS6KnJPV1mDuS8pYuLucaWzkwbWjGPeJsfQDpqc7NZ4F7zTQh4Wt --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio dcbtestuserc EOS76Pcyw1Hd7hW8hkZdUE1DQ3UiRtjmAKQ3muKwidRqmaM8iNtDy --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio dcbtestuserd EOS7jnmGEK9i33y3N1aV29AYrFptyJ43L7pATBEuVq4fVXG1hzs3G --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio dcbtestusere EOS7vr4QpGP7ixUSeumeEahHQ99YDE5jiBucf1B2zhuidHzeni1dD --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio fraxtokenfxs EOS7vr4QpGP7ixUSeumeEahHQ99YDE5jiBucf1B2zhuidHzeni1dD --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

# Deploy eosio.wrap
echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
cleos wallet import --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio@active
cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/


# Transfer EOS to testing accounts
echo -e "${CYAN}-----------------------TRANSFERRING EOS-----------------------${NC}"
cleos transfer eosio dcbtestusera "1000 EOS"
cleos transfer eosio dcbtestuserb "1000 EOS"
cleos transfer eosio dcbtestuserc "1000 EOS"
cleos transfer eosio dcbtestuserd "1000 EOS"
cleos transfer eosio dcbtestusere "1000 EOS"

## Deploy contracts
echo -e "${CYAN}-----------------------DEPLOYING CONTRACTS-----------------------${NC}"
cleos set contract bank.shares ../bank.shares/
cleos set contract bank.price ../bank.price/
cleos set contract bank.safesnd ../bank.safesnd/
cleos set contract bank.cdp ../bank.cdp/
cleos set contract bank.pay2key ../bank.pay2key/
cleos set contract everipediaiq $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract tethertether $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract fraxtokenfxs $EOSIO_CONTRACTS_ROOT/eosio.token/

echo -e "${CYAN}-----------------------TRANSFERRING IQ-----------------------${NC}"
cleos push action everipediaiq create '[ "everipediaiq", "10000000000.000 IQ" ]' -p everipediaiq
cleos push action everipediaiq issue '[ "everipediaiq", "1000000000.000 IQ", "issue IQ" ]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dcbtestusera", "10000.000 IQ", "memo"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dcbtestuserb", "10000.000 IQ", "memo"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dcbtestuserc", "10000.000 IQ", "memo"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dcbtestuserd", "10000.000 IQ", "memo"]' -p everipediaiq
cleos push action everipediaiq transfer '["everipediaiq", "dcbtestusere", "10000.000 IQ", "memo"]' -p everipediaiq

echo -e "${CYAN}-----------------------TRANSFERRING TETHER-----------------------${NC}"
cleos push action tethertether create '[ "tethertether", "10000000000.0000 USDT" ]' -p tethertether
cleos push action tethertether issue '[ "tethertether", "1000000000.0000 USDT", "issue USDT" ]' -p tethertether
cleos push action tethertether transfer '["tethertether", "dcbtestusera", "10000.0000 USDT", "memo"]' -p tethertether
cleos push action tethertether transfer '["tethertether", "dcbtestuserb", "10000.0000 USDT", "memo"]' -p tethertether
cleos push action tethertether transfer '["tethertether", "dcbtestuserc", "10000.0000 USDT", "memo"]' -p tethertether
cleos push action tethertether transfer '["tethertether", "dcbtestuserd", "10000.0000 USDT", "memo"]' -p tethertether
cleos push action tethertether transfer '["tethertether", "dcbtestusere", "10000.0000 USDT", "memo"]' -p tethertether

echo -e "${CYAN}-----------------------TRANSFERRING FRAX-----------------------${NC}"
cleos push action fraxtokenfxs create '[ "frax.reserve", "10000000000.0000 FRAX" ]' -p fraxtokenfxs

echo -e "${CYAN}-----------------------TRANSFERRING FXS-----------------------${NC}"
cleos push action fraxtokenfxs create '[ "fraxtokenfxs", "1000000.0000 FXS" ]' -p fraxtokenfxs # 1M cap fixed supply
cleos push action fraxtokenfxs issue '[ "fraxtokenfxs", "1000000.0000 FXS", "issue FXS" ]' -p fraxtokenfxs
cleos push action fraxtokenfxs transfer '["fraxtokenfxs", "dcbtestusera", "10000.0000 FXS", "memo"]' -p fraxtokenfxs
cleos push action fraxtokenfxs transfer '["fraxtokenfxs", "dcbtestuserb", "10000.0000 FXS", "memo"]' -p fraxtokenfxs
cleos push action fraxtokenfxs transfer '["fraxtokenfxs", "dcbtestuserc", "10000.0000 FXS", "memo"]' -p fraxtokenfxs
cleos push action fraxtokenfxs transfer '["fraxtokenfxs", "dcbtestuserd", "10000.0000 FXS", "memo"]' -p fraxtokenfxs
cleos push action fraxtokenfxs transfer '["fraxtokenfxs", "dcbtestusere", "10000.0000 FXS", "memo"]' -p fraxtokenfxs

# Grant code permissions for contracts that need it
cleos set account permission bank.safesnd active '{ "threshold": 1, "keys": [{ "key": "EOS68s2PrHPDeGWTKczrNZCn4MDMgoW6SFHuTQhXYUNLT1hAmJei8", "weight": 1 }], "accounts": [{ "permission": { "actor":"bank.safesnd","permission":"eosio.code" }, "weight":1 }] }' owner -p bank.safesnd
cleos set account permission bank.pay2key active '{ "threshold": 1, "keys": [{ "key": "EOS65yGjjyeyduJMGpQMGy39NcJenXFQ52HgroYos4dwjbwxu5TTW", "weight": 1 }], "accounts": [{ "permission": { "actor":"bank.pay2key","permission":"eosio.code" }, "weight":1 } ] }'
cleos set account permission frax.reserve active '{ "threshold": 1, "keys": [{ "key": "EOS6jA39vSMGVHVQzxhYhE9gMToCYG9U9QLZiWZschCgfQuKJ6nrF", "weight": 1 }], "accounts": [{ "permission": { "actor":"frax.reserve","permission":"eosio.code" }, "weight":1 } ] }'
cleos set account permission frax.loans active '{ "threshold": 1, "keys": [{ "key": "EOS6DcmhfH9RSEftsRa6k8uH7E76sLRtj3hFPyVRLoNDEBdZE1anc", "weight": 1 }], "accounts": [{ "permission": { "actor":"frax.loans","permission":"eosio.code" }, "weight":1 } ] }'

echo -e "${CYAN}-----------------------COMPLETE-----------------------${NC}"
