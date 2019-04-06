const ecc = require('eosjs-ecc');
const base58 = require('bs58');
const shell = require('shelljs');

function getKeys() {
  return Promise.all([ecc.randomKey(), ecc.randomKey(), ecc.randomKey()]);
}

function createKeyPair(privateKey) {
  return {
    public: ecc.privateToPublic(privateKey),
    private: privateKey,
  };
}

function transferCommand(relayer, fromKeys, to, amount, fee, nonce, memo) {
  const version = 1;
  const length = 92 + memo.length;

  const pkeyFrom = base58.decode(fromKeys.public.substring(3));
  const pkeyTo = base58.decode(to.substring(3));
  const amountBuf = uint64_to_little_endian(amount * 10000);
  const feeBuf = uint64_to_little_endian(fee * 10000);
  const nonceBuf = uint64_to_little_endian(nonce);
  const memoBuf = Buffer.from(memo);

  // create raw tx
  const buffer = Buffer.alloc(length);
  buffer[0] = version;
  buffer[1] = length;
  pkeyFrom.copy(buffer, 2, 0, 33);
  pkeyTo.copy(buffer, 35, 0, 33);
  amountBuf.copy(buffer, 68, 0, 8);
  feeBuf.copy(buffer, 76, 0, 8);
  nonceBuf.copy(buffer, 84, 0, 8);
  memoBuf.copy(buffer, 92, 0, memoBuf.length);

  // hash raw tx
  const hashed = ecc.sha256(buffer, 'hex');

  // sign transaction
  const sig = ecc.signHash(hashed, fromKeys.private);

  return `cleos push action utxo transfer '["${relayer}", "${fromKeys.public}", "${to}", "${amount}.0000 UTXO", "${fee}.0000 UTXO", ${nonce}, "${memo}", "${sig}"]' -p utxo`;
}

// due to JS limitaitons, this only has 48-bit precision,
// but that's good enough for what we need
function uint64_to_little_endian(num) {
  const buf = Buffer.alloc(8);
  buf.writeUIntLE(num, 0, 6);
  return buf;
}

async function test() {
  const privateKeys = await getKeys();
  const utxoKeys = createKeyPair(privateKeys[0]);
  const firstAccountKeys = createKeyPair(privateKeys[1]);
  const secondAccountKeys = createKeyPair(privateKeys[2]);

  // wallet setup
  shell.exec('rm ~/eosio-wallet/default.wallet');
  shell.exec('cleos wallet create --to-console');
  shell.exec('cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'); // EOS key
  shell.exec(`cleos wallet import --private-key ${utxoKeys.private}`);

  // account setup
  shell.exec(`cleos create account eosio utxo ${utxoKeys.public}`);
  shell.exec(`cleos create account eosio everipediaiq ${utxoKeys.public}`);
  shell.exec(`cleos create account eosio eosio.token ${utxoKeys.public}`);

  // bootstrap system contracts
  shell.exec(`cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/`)
  shell.exec(`cleos push action eosio.token create '["eosio", "1000.0000 EOS"]' -p eosio.token`)
  shell.exec(`cleos push action eosio.token issue '["eosio", "1000.0000 EOS", "memo"]' -p eosio`)

  // compilation and setting
  shell.exec('eosio-cpp -w -o ../verifier/verifier.wasm ../verifier/verifier.cpp');
  shell.exec('cleos set contract utxo ../verifier');

  // create
  shell.exec('cleos push action utxo create \'["utxo", "100.0000 UTXO"]\' -p utxo');
  shell.exec('cleos get table utxo 340784338180 stats');

  // issue
  shell.exec(`cleos transfer eosio utxo "1.0000 EOS" ${utxoKeys.public}`);
  shell.exec(`cleos transfer eosio utxo "5.0000 EOS" ${firstAccountKeys.public}`);
  shell.exec(`cleos transfer eosio utxo "5.0000 EOS" ${secondAccountKeys.public}`);
  shell.exec('cleos get table utxo utxo accounts');

  // transfer, expected balances is 2, 7, 2
  shell.exec(transferCommand(utxoKeys.public, secondAccountKeys, firstAccountKeys.public, 3, 1, 1, "transfer from second account to first"));
  shell.exec(transferCommand(utxoKeys.public, firstAccountKeys, secondAccountKeys.public, 1, 0, 1, "transfer from second account to first"));
  shell.exec('cleos get table utxo utxo accounts');

  // withdrawal, expected balances is 2, 2, 2
  shell.exec(`cleos set account permission utxo active '{ "threshold": 1, "keys": [{ "key": "${utxoKeys.public}", "weight": 1 }], "accounts": [{ "permission": { "actor":"utxo","permission":"eosio.code" }, "weight":1 } ] }' owner -p utxo`);
  shell.exec(transferCommand(utxoKeys.public, firstAccountKeys, 'EOS1111111111111111111111111111111114T1Anm', 5, 0, 2, 'eosio.token', 'utxo'));
  shell.exec('cleos get table utxo utxo accounts');

  shell.exec('cleos get table utxo 340784338180 stats');
}

test();
