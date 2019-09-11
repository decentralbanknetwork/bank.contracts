add_contract(frax.loans frax.loans ${CMAKE_CURRENT_SOURCE_DIR}/frax.loans.cpp)
target_include_directories(frax.loans.wasm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(frax.loans.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR})
