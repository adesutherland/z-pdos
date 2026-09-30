add_executable(test_tso31_source test_source.c)
add_test(NAME tso31_source_variant COMMAND test_tso31_source
  ${CMAKE_CURRENT_SOURCE_DIR}/reference-entry31.asm
  ${CMAKE_CURRENT_SOURCE_DIR}/entry31.asm)
add_executable(test_tso31_abi test_abi.c caller.c)
add_test(NAME tso31_caller_contract COMMAND test_tso31_abi)
add_executable(test_tso31_deck test_deck.c)
set(entry_deck ${CMAKE_CURRENT_BINARY_DIR}/entry31.obj)
add_test(NAME tso31_deck_prepare
  COMMAND ${CMAKE_COMMAND} -E rm -f ${entry_deck})
set_tests_properties(tso31_deck_prepare PROPERTIES FIXTURES_SETUP tso31_path)
add_test(NAME tso31_assemble
  COMMAND mf-classic-as --profile s370
    ${CMAKE_CURRENT_SOURCE_DIR}/entry31.asm ${entry_deck})
set_tests_properties(tso31_assemble PROPERTIES
  FIXTURES_REQUIRED tso31_path FIXTURES_SETUP tso31_deck)
add_test(NAME tso31_deck_semantics COMMAND test_tso31_deck ${entry_deck})
set_tests_properties(tso31_deck_semantics PROPERTIES FIXTURES_REQUIRED tso31_deck)
