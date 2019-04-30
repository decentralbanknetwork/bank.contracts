# eosio.pay2key

The UTXO token contract (also called Pay 2 Key contract) is a standardized EOSIO smart contract, which allows token transacting using only public-private key pairs without the need for an EOSIO account or system action. It is similar in feature to the Bitcoin transaction system. To move tokens in a UTXO based contract, an owner only needs the corresponding private key to the public key. A transaction fee is also paid to move the token. A "relayer" with an EOS account (can be a block producer or any individual account) accepts the fee then relays the signed UTXO message to the contract and updates the balance of the appropriate public key. A relayer is similar to a Bitcoin miner in that they update the state of the UTXO contract by appending the signed UTXO transaction to the contract database.   

There are 4 main actions in the contract: 1. Creating the UTXO token specs when the contract is first deployed 2. Sending tokens from EOS accounts to a public key in the Pay2Key contract 3. Transacting UTXO/Pay2Key tokens between public keys 4. Withdrawing your UTXOs from a public key back into an EOS account  

## 1. Creating a token with a maximum supply

Before you are able to issue a token to a public key, you must create the spendable output and give the token maximum supply to set up the contract. In this documentation, the ticker of the example token in this documentation is IQ with the Pay2Key ticker as IQUTXO. 

```
cleos push action {UTXO_contract_account} create \'["IQUTXO", "100.0000 IQUTXO"]\' -p {UTXO_contract_account}
```

## 2. Transferring tokens to issue the UTXO

Anyone can send the designated token to the UTXO contract, triggering an issuance to the public key of their choosing.
```
cleos push action everipediaiq transfer '["{EOS_account}", "{UTXO_contract_account}", "100.000 IQUTXO", "{PUBLIC_KEY}"]' -p {EOS_account}@active
```
Now that public key will have 100 IQUTXO to spend.

## 3. Transferring from one public key to another

To transfer from one public key to another, the transaction must be signed using your private key. For details of how to generate the signature, check out the code in frontend section. The frontend client makes generating this signature easy. There are a number of hosted solutions for UTXO front ends coming. 

```
cleos push action utxo transfer '["RELAYER_PUBLIC_KEY", "PUBLIC_KEY1", "PUBLIC_KEY2", "3.0000 IQUTXO", "1.0000 IQUTXO", {nonce}, "memo here", "{SIGNATURE}"]' -p utxo
```

### Generating Signatures 
To create a signed transaction to spend UTXOs, a signature must be created by the private key of a public key with spendable outputs. We've included a front end interface to easily do that in the repository for local usage. There are also hosted solutions of the front end at: 


The Signature is generated with the following fields:
Public key of the spender
Private key of the spender
Public key of the recipient  
Amount of UTXO to spend (denominated as uint64)
Amount of UTXO to pay as fee (denominated as uint64)
A nonce (we highly recommend incrementing the nonce by 1 starting from 0 for each public key) 
A memo/string field

In the future, we will add a token ID field to add replay protection to generated signatures. 

## 4. Withdrawing UTXOs out of the contract back into an EOS account

To withdraw tokens from an account, issue a transfer as normal, except make the receiving public key be the NULL address, EOS1111111111111111111111111111111114T1Anm, and the memo the receiving EOS account name.

The contract detects this, modifies the circulating supply, and triggers an inline action to send IQ to the EOS account specified in the memo field.

```
cleos push action utxo transfer '["{RELAYER_PUBLIC_KEY}", "{PUBLIC_KEY1}", "EOS1111111111111111111111111111111114T1Anm", "3.0000 IQUTXO", "1.0000 IQUTXO", {nonce}, "chrisaccount", "{SIGNATURE}"]' -p utxo
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
