add_contract(frax.reserve frax.reserve ${CMAKE_CURRENT_SOURCE_DIR}/frax.reserve.cpp)
target_include_directories(frax.reserve.wasm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(frax.reserve.wasm PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR})
