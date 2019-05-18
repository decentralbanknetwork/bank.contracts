// EXAMPLE: node wif_details.js 5JuHN27YsJktraQYGgLeihxZ2bwQbFANFdpc8gtDgqyyJsZJQB6

const ecc = require('eosjs-ecc');
const base58 = require('bs58');
const ripemd160 = require('ripemd160')

const wif = process.argv[2];
console.log("WIF: ", wif);

const privkey = ecc.PrivateKey.fromString(wif);
console.log("Private Key: ", privkey.toBuffer().toString('hex'));

const compressed_pubkey = privkey.toPublic();
const uncompressed_pubkey = compressed_pubkey.toUncompressed();
console.log("Public key (compressed): ", compressed_pubkey.toHex());
console.log("Public key: ", uncompressed_pubkey.toHex());

const hash1 = ecc.sha256(compressed_pubkey.toBuffer());
const hash2 = new ripemd160().update(Buffer.from(hash1, 'hex')).digest('hex');
const hash3 = ecc.sha256(uncompressed_pubkey.toBuffer());
const hash4 = new ripemd160().update(Buffer.from(hash3, 'hex')).digest('hex');
const with_prefix_compressed = '00' + hash2;
const with_prefix_uncompressed = '00' + hash4;

const hash5 = ecc.sha256(Buffer.from(with_prefix_compressed, 'hex'));
const hash6 = ecc.sha256(Buffer.from(hash5, 'hex'));
const hash7 = ecc.sha256(Buffer.from(with_prefix_uncompressed, 'hex'));
const hash8 = ecc.sha256(Buffer.from(hash7, 'hex'));
const binary_address_compressed = with_prefix_compressed + hash6.slice(0,8);
const binary_address_uncompressed = with_prefix_uncompressed + hash8.slice(0,8);
console.log("Binary address (compressed): ", binary_address_compressed);
console.log("Binary address: ", binary_address_uncompressed);

const bitcoin_address_compressed = base58.encode(Buffer.from(binary_address_compressed, 'hex'));
const bitcoin_address_uncompressed = base58.encode(Buffer.from(binary_address_uncompressed, 'hex'));
console.log("Bitcoin address (compressed): ", bitcoin_address_compressed);
console.log("Bitcoin address: ", bitcoin_address_uncompressed);
