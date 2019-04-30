const express = require('express');
const bodyParser = require('body-parser');
const Joi = require('@hapi/joi');
const fetch = require('node-fetch');
const dotenv = require('dotenv');
const eosjs = require('eosjs');
const jssig = require('eosjs/dist/eosjs-jssig');

const Api = eosjs.Api;
const JsonRpc = eosjs.JsonRpc;
const JsSignatureProvider = jssig.JsSignatureProvider;

require('dotenv').config()

const app = express();

const rpc = new JsonRpc(process.env.EOS_API_BASE, { fetch });
const signatureProvider = new JsSignatureProvider([process.env.RELAYER_PRIVATE_KEY]);
const eosapi = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

const msgSchema = Joi.object().keys({
    from: Joi.string().length(53).regex(/EOS.+/).required(),
    to: Joi.string().length(53).regex(/EOS.+/).required(),
    amount: Joi.string(),
    fee: Joi.string(),
    nonce: Joi.number().integer().min(0).required(),
    memo: Joi.string().min(0).max(163).required(),
    sig: Joi.string()
});

app.use(bodyParser.json());
 
app.post('/relay', function (req, res) {
    const msg = req.body;
    msgSchema.validate(msg);

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
        res.status(500).send(err.message);
    });

})
 
app.listen(6400, (e) => {
    if (e) console.error(e);
    else console.log("Relay server listening on port 6400...");
});
