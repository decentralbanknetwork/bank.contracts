# trap ctrl_c INT
# function ctrl_c {
#     exit 2;
# }

# if [ ! -e build.sh ]; then
#     echo "Building from wrong directory. Must be in project root"
#     exit 1;
# fi

# git submodule update

cd bank.cdp
echo "Building bank.cdp..."
eosio-cpp bank.cdp.cpp -o bank.cdp.wasm -I bank.cdp.clauses.md -I bank.cdp.contracts.md
# eosio-cpp -abigen daiqcontract.cpp -o daiqcontract.wasm -I daiqcontract.clauses.md -I daiqcontract.contracts.md

# cd ../Everipedia/everipediaiq
# echo "Building everipediaiq..."
# eosio-cpp -abigen everipediaiq.cpp -o everipediaiq.wasm -I everipediaiq.clauses.md -I everipediaiq.contracts.md

# cd ../../Bonds
# echo "Building Bonds..."
# eosio-cpp -abigen bond.cpp -o bond.wasm -I bond.clauses.md -I bond.contracts.md
# eosio-cpp -abigen everipediaiq.cpp -o everipediaiq.wasm -I everipediaiq.clauses.md -I everipediaiq.contracts.md

# cd ../everipediaiq
# echo "Building everipediaiq..."
# /usr/local/bin/eosio-cpp -abigen everipediaiq.cpp -o everipediaiq.wasm -I everipediaiq.clauses.md -I everipediaiq.contracts.md

# cd ../iqutxo
# echo "Building iqutxo..."
# /usr/local/bin/eosio-cpp -abigen iqutxo.cpp -o iqutxo.wasm 

cd ..