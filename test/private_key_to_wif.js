const ecc = require('eosjs-ecc');
const base58 = require('bs58');

const private_key = process.argv[2];
const with_version = '80' + private_key;
const hash1 = ecc.sha256(Buffer.from(with_version, 'hex'));
const hash2 = ecc.sha256(Buffer.from(hash1, 'hex'));
const checked_key = with_version + hash2.slice(0,8);
const wif = base58.encode(Buffer.from(checked_key, 'hex'));

console.log(wif);
