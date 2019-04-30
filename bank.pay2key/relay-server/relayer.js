const express = require('express');
const bodyParser = require('body-parser');
const joi = require('@hapi/joi');
const fetch = require('node-fetch');
const dotenv = require('dotenv');
const eosjs = require('eosjs');
const jssig = require('eosjs/dist/eosjs-jssig');

const Api = eosjs.Api();
const JsonRpc = eosjs.JsonRpc();
const JsSignatureProvider = jssig.JsSignatureProvider;

require('dotenv').config()

const app = express();

const rpc = new JsonRpc(process.env.EOS_API_BASE, { fetch });
const defaultPrivateKey = process.env.SIGNING_KEY;
const signatureProvider = new JsSignatureProvider([defaultPrivateKey]);
const eosapi = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

const msgSchema = Joi.schema({
    from: Joi.string().length(53).regex(/EOS.+/).required(),
    to: Joi.string().length(53).regex(/EOS.+/).required(),
    amount: Joi.number().integer().min(0).required(),
    fee: Joi.number().integer().min(0).required(),
    nonce: Joi.number().integer().min(0).required(),
    memo: Joi.string().min(0).max(163).required(),
    sig: Joi.string()
});

app.use(bodyParser.json({ type: 'application/*+json' }));
 
app.get('/relay', function (req, res) {
    const msg = req.body;
    msgSchema.validate(msg);

    msg.relayer = process.env.RELAYER_KEY;
    
    const result = api.transact({
        actions: [{
            account: 'iqutxoiqutxo',
            name: 'transfer',
            authorization: [{
                actor: 'evrpdcronjob',
                permission: 'active',
            }],
            data: body
        }]
    }, {
        blocksBehind: 3,
        expireSeconds: 30,
    })

})
 
app.listen(6400);
