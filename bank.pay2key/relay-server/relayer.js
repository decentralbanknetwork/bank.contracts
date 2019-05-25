const express = require('express');
const bodyParser = require('body-parser');
const Joi = require('@hapi/joi');
const fetch = require('node-fetch');
const dotenv = require('dotenv');
const eosjs = require('eosjs');
const jssig = require('eosjs/dist/eosjs-jssig');
const morgan = require('morgan');

const Api = eosjs.Api;
const JsonRpc = eosjs.JsonRpc;
const JsSignatureProvider = jssig.JsSignatureProvider;

require('dotenv').config()

const app = express();
app.use(morgan('combined'));

// CORS
app.use(function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
  next();
});

const rpc = new JsonRpc(process.env.EOS_API_BASE, { fetch });
const signatureProvider = new JsSignatureProvider([process.env.RELAYER_PRIVATE_KEY]);
const eosapi = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

const eosSchema = Joi.object().keys({
    chain_id: Joi.number().min(0).max(1000).required(),
    from: Joi.string().length(53).regex(/EOS.+/).required(),
    to: Joi.string().length(53).regex(/EOS.+/).required(),
    amount: Joi.string(),
    fee: Joi.string(),
    nonce: Joi.number().integer().min(0).required(),
    memo: Joi.string().min(0).max(163).required(),
    sig: Joi.string()
});

const btcSchema = Joi.object().keys({
    chain_id: Joi.number().min(0).max(1000).required(),
    from: Joi.string().min(25).max(35).startsWith("1").required(),
    to: Joi.string().min(25).max(35).startsWith("1").required(),
    amount: Joi.string(),
    fee: Joi.string(),
    nonce: Joi.number().integer().min(0).required(),
    memo: Joi.string().min(0).max(163).required(),
    sig: Joi.string()
});

app.use(bodyParser.json());
 
app.post('/relay/eos', function (req, res) {
    const msg = req.body;
    const validates = Joi.validate(msg, eosSchema);
    if (validates.error) {
        const error = validates.error.details[0].message;
        res.status(400).send({ error: error });
        console.error(error);
        return;
    }

    msg.relayer = process.env.RELAYER_PUBLIC_KEY;
    
    const result = eosapi.transact({
        actions: [{
            account: process.env.UTXO_ACCOUNT,
            name: 'transfer',
            authorization: [{
                actor: process.env.RELAYER_ACCOUNT,
                permission: 'active',
            }],
            data: msg
        }]
    }, {
        blocksBehind: 3,
        expireSeconds: 30,
    }).then(response => {
        res.status(200).send(response);
    }).catch(err => {
        console.error(err);
        res.status(500).send({ error: err.message });
    });

})

app.post('/relay/btc', function (req, res) {
    const msg = req.body;
    const validates = Joi.validate(msg, btcSchema);
    if (validates.error) {
        const error = validates.error.details[0].message;
        res.status(400).send({ error: error });
        console.error(error);
        return;
    }

    msg.relayer = process.env.RELAYER_PUBLIC_KEY;
    
    const result = eosapi.transact({
        actions: [{
            account: process.env.UTXO_ACCOUNT,
            name: 'transfer',
            authorization: [{
                actor: process.env.RELAYER_ACCOUNT,
                permission: 'active',
            }],
            data: msg
        }]
    }, {
        blocksBehind: 3,
        expireSeconds: 30,
    }).then(response => {
        res.status(200).send(response);
    }).catch(err => {
        console.error(err);
        res.status(500).send({ error: err.message });
    });

})

app.listen(6400, (e) => {
    if (e) console.error(e);
    else console.log("Relay server listening on port 6400...");
});
