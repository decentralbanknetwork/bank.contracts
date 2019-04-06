# bank.pay2key

The UTXO token contract is a standardized EOSIO smart contract, which allows token transacting using only public-private key pairs without the need for an EOSIO account or system action. It is similar in feature to the Bitcoin transaction system. To move tokens in a UTXO based contract, an owner only needs the corresponding private key to the public key. A transaction fee is also paid to move the token. No account or EOSIO network interaction is required.

## Creating a token with a maximum supply

Before you are able to issue a token to a public key, you must create the token and give a maximum supply.
```
cleos push action utxo create \'["utxo", "100.0000 UTXO"]\' -p utxo
```

## Transferring EOS to issue the token

Anyone can send EOS to the contract, triggering an issuance to the key of their choosing.
```
cleos transfer chrisaccount eosio.utxo "10 EOS" "PUBLIC_KEY"
```
Now that public key will have 10 UTXO.

## Transferring from one public key to another

To transfer from one public key to another, the transaction must be signed using your private key. For details of how to generate the signature, check out the code in frontend/src/App.js:55

The frontend client makes generating this signature easy.

```
cleos push action utxo transfer '["RELAYER_PUBLIC_KEY", "PUBLIC_KEY1", "PUBLIC_KEY2", "3.0000 UTXO", "1.0000 UTXO", 1, "transfer from PUBLIC_KEY1 to PUBLIC_KEY2", "SIGNATURE_GENERATED_USING_PUBLIC_KEY1_PRIVATE_KEY"]' -p utxo
```

## Withdrawing

To withdraw tokens from an account, issue a transfer as normal, except make the receiving public key be the NULL address, EOS1111111111111111111111111111111114T1Anm, and the memo the receiving account name.

The contract detects this, modifies the circulating supply, and triggers an inline action to send EOS to the user specified in the memo field.

```
cleos push action utxo transfer '["RELAYER_PUBLIC_KEY", "PUBLIC_KEY1", "EOS1111111111111111111111111111111114T1Anm", "3.0000 UTXO", "1.0000 UTXO", 1, "chrisaccount", "SIGNATURE_GENERATED_USING_PUBLIC_KEY1_PRIVATE_KEY"]' -p utxo
```
## Running tests
```
cd js_test
npm install
EOSIO_CONTRACTS_ROOT=_path_to_eosio_contracts_ node test.js
```

## Running frontend
```
cd frontend
yarn install
yarn start
```
