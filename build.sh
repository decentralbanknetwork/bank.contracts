cd bank.price
echo "Building bank.price..."
eosio-cpp -abigen bank.price.cpp -o bank.price.wasm -I bank.price.clauses.md -I bank.price.contracts.md
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

cd frax.reserve
echo "Building frax.reserve..."
eosio-cpp -abigen frax.reserve.cpp -o frax.reserve.wasm -I frax.reserve.clauses.md -I frax.reserve.contracts.md
cd ..
