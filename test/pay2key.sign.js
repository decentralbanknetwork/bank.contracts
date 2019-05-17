const ecc = require('eosjs-ecc');
const base58 = require('bs58');

// due to JS limitaitons, this only has 48-bit precision,
// but that's good enough for what we need
function uint64_to_little_endian(num) {
  const buf = Buffer.alloc(8);
  buf.writeUIntLE(num, 0, 6);
  return buf;
}

function uint32_to_little_endian(num) {
  const buf = Buffer.alloc(4);
  buf.writeUIntLE(num, 0, 4);
  return buf;
}

function sign(chain_id, from, to, amount, fee, nonce, memo, privkey) {
  const version = 3;
  const length = 80 + memo.length;

  const chainidBuf = uint32_to_little_endian(chain_id);
  const pkeyFrom = base58.decode(from);
  const pkeyTo = base58.decode(to);
  const amountBuf = uint64_to_little_endian(amount);
  const feeBuf = uint64_to_little_endian(fee);
  const nonceBuf = uint64_to_little_endian(nonce);
  const memoBuf = Buffer.from(memo);

  // create raw tx
  const buffer = Buffer.alloc(length);
  buffer[0] = version;
  buffer[1] = length;
  chainidBuf.copy(buffer, 2, 0, 4);
  pkeyFrom.copy(buffer, 6, 0, 25);
  pkeyTo.copy(buffer, 31, 0, 25);
  amountBuf.copy(buffer, 56, 0, 8);
  feeBuf.copy(buffer, 64, 0, 8);
  nonceBuf.copy(buffer, 72, 0, 8);
  memoBuf.copy(buffer, 80, 0, memoBuf.length);
  //console.log(buffer.toString('hex'));

  // hash raw tx
  const hashed = ecc.sha256(buffer, 'hex');
  //console.log(hashed);

  // sign transaction
  const sig = ecc.signHash(hashed, privkey);

  return sig;
}

///////////////////////////////
// Main Execution Thread
if (process.argv.length != 10) {
    console.log(`
        USAGE: node pay2key.sign.js [chain_id] [from] [to] [amount] [fee] [nonce] [memo] [privkey]
        Example: node pay2key.sign.js 0 1GkVXJKuphYRpGeSbjxWSD4S4kyPjvS5oh 15HQkwuxBnQgPzG3UMtoz7qMrNw7B3uKax 10000 10 1 "token transfer" 5JzQSvpmLHTvF3HDdfzaD8WMLFpWjBobCCy8qYwivcrzq1SdATW
    `);
}
else {
    const args = process.argv.slice(2);
    console.log(sign(...args));
}
