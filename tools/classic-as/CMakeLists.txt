add_library(mf_classic STATIC
  src/values.c src/reader.c src/machine.c src/assemble.c src/object.c)
target_include_directories(mf_classic PUBLIC include)
add_executable(mf-classic-as src/main.c)
target_link_libraries(mf-classic-as PRIVATE mf_classic)

foreach(component values reader machine object)
  add_executable(test_${component} tests/test_${component}.c)
  target_link_libraries(test_${component} PRIVATE mf_classic)
  add_test(NAME classic_${component} COMMAND test_${component})
endforeach()
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_assemble.c)
  add_executable(test_assemble tests/test_assemble.c)
  target_link_libraries(test_assemble PRIVATE mf_classic)
  add_test(NAME classic_assemble COMMAND test_assemble)
else()
  message(STATUS "Checkpoint: engine unit suite is pending; see docs/CHECKPOINT.md")
endif()

add_executable(test_deck tests/test_deck.c)
add_test(NAME classic_cli_prepare
  COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/bootstrap.obj)
set_tests_properties(classic_cli_prepare PROPERTIES FIXTURES_SETUP bootstrap_path)
add_test(NAME classic_cli_bootstrap
  COMMAND mf-classic-as ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bootstrap.asm
    ${CMAKE_CURRENT_BINARY_DIR}/bootstrap.obj)
set_tests_properties(classic_cli_bootstrap PROPERTIES
  FIXTURES_REQUIRED bootstrap_path FIXTURES_SETUP bootstrap_deck)
add_test(NAME classic_deck_semantics COMMAND test_deck
  ${CMAKE_CURRENT_BINARY_DIR}/bootstrap.obj)
set_tests_properties(classic_deck_semantics PROPERTIES FIXTURES_REQUIRED bootstrap_deck)
