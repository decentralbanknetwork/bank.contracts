echo "Building bank.shares..."
eosio-cpp -abigen bank.shares.cpp -o bank.shares.wasm -I bank.shares.clauses.md -I bank.shares.contracts.md

cd bank.price
echo "Building bank.price..."
eosio-cpp -abigen bank.price.cpp -o bank.price.wasm -I bank.price.clauses.md -I bank.price.contracts.md
cd ..

cd bank.utxo
echo "Building bank.utxo..."
eosio-cpp -abigen bank.utxo.cpp -o bank.utxo.wasm -I bank.utxo.clauses.md -I bank.utxo.contracts.md
cd ..

cd bank.token
echo "Building bank.token..."
eosio-cpp -abigen bank.token.cpp -o bank.token.wasm -I bank.token.clauses.md -I bank.token.contracts.md
cd ..

cd bank.safesnd
echo "Building bank.safesnd..."
eosio-cpp -abigen bank.safesnd.cpp -o bank.safesnd.wasm
cd ..

cd bank.cdp
echo "Building bank.cdp..."
eosio-cpp -abigen bank.cdp.cpp -o bank.cdp.wasm -I bank.cdp.clauses.md -I bank.cdp.contracts.md
cd ..

cd bank.pay2key
echo "Building bank.pay2key..."
eosio-cpp -abigen bank.pay2key.cpp -o bank.pay2key.wasm -I bank.pay2key.clauses.md -I bank.pay2key.contracts.md
cd ..
