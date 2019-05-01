import React, { Component } from 'react';
import './App.css';

const ecc = require('eosjs-ecc');
const base58 = require('bs58');

class App extends Component {
  constructor(props) {
    super(props);
    this.state = {
      from: '',
      fromPrivateKey: '',
      to: '',
      amount: 0,
      fee: 1,
      nonce: 1,
      memo: '',
      sig: '',
    };
  }

  targetValue = (target) => {
    switch(target.type) {
      case "number":
        return target.valueAsNumber;
      case "text":
        return target.value;
      default:
        throw new Error("Unexpected target type");
    }
  }

  handleChange = (event) => {
    this.setState({ [event.target.name]: this.targetValue(event.target) });
  }

  handleSubmit = (event) => {
    event.preventDefault();
    const sig = this.transactionSignature(this.state.from,
                                          this.state.fromPrivateKey,
                                          this.state.to,
                                          this.state.amount,
                                          this.state.fee,
                                          this.state.nonce,
                                          this.state.memo);
    this.setState({ sig: sig });
    console.log(sig);
    this.apiRequest(sig);
  }

  // due to JS limitaitons, this only has 48-bit precision,
  // but that's good enough for what we need
  uint64_to_little_endian(num) {
    const buf = Buffer.alloc(8);
    buf.writeUIntLE(num, 0, 6);
    return buf;
  }

  transactionSignature(fromPublicKey, fromPrivateKey, toPublicKey, amount, fee, nonce, memo) {
    const version = 1;
    const length = 92 + memo.length;

    const pkeyFrom = base58.decode(fromPublicKey.substring(3));
    const pkeyTo = base58.decode(toPublicKey.substring(3));
    const amountBuf = this.uint64_to_little_endian(amount);
    const feeBuf = this.uint64_to_little_endian(fee);
    const nonceBuf = this.uint64_to_little_endian(nonce);
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

    return ecc.signHash(hashed, fromPrivateKey);
  }

  async apiRequest(signature) {
    this.setState({ error: null });
    const RELAY_ENDPOINT = "http://epmainnet.libertyblock.io:6400/relay";
    const relay_response = await fetch(RELAY_ENDPOINT, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        from: this.state.from,
        to: this.state.to,
        amount: (this.state.amount / 1000).toFixed(3) + " IQUTXO",
        fee: (this.state.fee / 1000).toFixed(3) + " IQUTXO",
        nonce: this.state.nonce,
        memo: this.state.memo,
        sig: signature
      })
    })
    .then(response => response.json())

    console.log(relay_response);
    if (relay_response.error)
        this.setState({ error: relay_response.error });
    else
        this.setState({ txid: relay_response.transaction_id });
  }

  isSubmitDisabled() {
    return this.state.from === '' ||
           this.state.fromPrivateKey === '' ||
           this.state.to === '' ||
           isNaN(this.state.amount) ||
           isNaN(this.state.fee) ||
           isNaN(this.state.nonce);
  }

  render() {
    const { txid, error } = this.state;

    return (
      <div className="App">
        <h1>IQ Pay-to-key</h1>
        <form onSubmit={this.handleSubmit}>
          <label className="label">
            From Public Key:
            <input type="text"
                   value={this.state.from}
                   name="from"
                   placeholder="Enter from public key"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <label className="label">
            From Private Key:
            <input type="text"
                   value={this.state.fromPrivateKey}
                   name="fromPrivateKey"
                   placeholder="Enter from private key"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <label className="label">
            To Public Key:
            <input type="text"
                   value={this.state.to}
                   name="to"
                   placeholder="Enter to public key"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <label className="label">
            Amount:
            <input type="number"
                   value={this.state.amount}
                   name="amount"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <label className="label">
            Fee:
            <input type="number"
                   value={this.state.fee}
                   name="fee"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <label className="label">
            Nonce:
            <input type="number"
                   value={this.state.nonce}
                   name="nonce"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <label className="label">
            Memo:
            <input type="text"
                   value={this.state.memo}
                   name="memo"
                   onChange={this.handleChange}
                   className="input" />
          </label>
          <input type="submit" value="Transfer" disabled={this.isSubmitDisabled()} />
        </form>

        {error ? <div style={{ maxWidth: '600px', wordWrap: 'break-word', color: 'red', marginTop: '5px' }}>{error}</div> : null }
        {txid ? 
            <div style={{ maxWidth: '600px', wordWrap: 'break-word', marginTop: '5px' }}>
                You transfer was successful. View it <a href={ "https://eosflare.io/tx/" + txid }>here</a>
            </div> : null 
        }
      </div>
    );
  }
}

export default App;
