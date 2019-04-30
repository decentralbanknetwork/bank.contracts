# UTXO Relay Server

This HTTP server exposes a single endpoint:

`/relay`: Relay a UTXO tranasction with the parameters specified in the `.env` file. Seem `SAMPLE.env` for how to configure the environment.

Example:

```
POST /relay
Content-Type: application/json

{
	"from": "EOS7PoGq46ssqeGh8ZNScWQxqbwg5RNvLAwVw3i5dQcZ3a1h9nRyr",
	"to": "EOS7VZbHpaEog944Wdw6XuddGBCYVGcy7W8QA5khtQP3dAgBVgB7T",
	"amount": "9.000 IQUTXO", 
	"fee": "1.000 IQUTXO",
	"nonce": 6,
	"memo": "testing",
	"sig": "SIG_K1_K17GrShyMD6A84D9YkZn2CNioY6rDrKGTz7uCduHZ663cNfLq7ecvZqY9vJsPMExetVMwKTQeNMceKtopMj4iSo8nyXLEW"
}
```

