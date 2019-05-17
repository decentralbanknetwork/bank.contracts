const ecc = require('eosjs-ecc');
const base58 = require('bs58');
const ripemd160 = require('ripemd160')

const wif = process.argv[2];
console.log("WIF: ", wif);

const privkey = ecc.PrivateKey.fromString(wif);
console.log("Private Key: ", privkey.toBuffer().toString('hex'));

const pubkey = privkey.toPublic();
console.log("Public key: ", pubkey.toHex());

const hash1 = ecc.sha256(pubkey.toBuffer());
const hash2 = new ripemd160().update(Buffer.from(hash1, 'hex')).digest('hex');
const with_prefix = '00' + hash2;

const hash3 = ecc.sha256(Buffer.from(with_prefix, 'hex'));
const hash4 = ecc.sha256(Buffer.from(hash3, 'hex'));
const binary_address = with_prefix + hash4.slice(0,8);
console.log("Binary address: ", binary_address);

const bitcoin_address = base58.encode(Buffer.from(binary_address, 'hex'));
console.log("Bitcoin address: ", bitcoin_address);
