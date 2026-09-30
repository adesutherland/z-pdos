add_library(mf_classic STATIC
  src/values.c src/reader.c src/machine.c src/assemble.c src/object.c)
target_include_directories(mf_classic PUBLIC include)
add_executable(mf-classic-as src/main.c)
target_link_libraries(mf-classic-as PRIVATE mf_classic)

foreach(component values reader machine object assemble contracts)
  add_executable(test_${component} tests/test_${component}.c)
  target_link_libraries(test_${component} PRIVATE mf_classic)
  add_test(NAME classic_${component} COMMAND test_${component})
endforeach()

add_executable(test_deck tests/test_deck.c)
add_executable(test_cli_output tests/test_cli_output.c)
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

function(classic_cli_fixture name source)
  set(output ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
  add_test(NAME classic_cli_${name}_prepare
    COMMAND ${CMAKE_COMMAND} -E rm -f ${output})
  set_tests_properties(classic_cli_${name}_prepare PROPERTIES
    FIXTURES_SETUP ${name}_path)
  add_test(NAME classic_cli_${name}
    COMMAND mf-classic-as ${ARGN}
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${source} ${output})
  set_tests_properties(classic_cli_${name} PROPERTIES
    FIXTURES_REQUIRED ${name}_path FIXTURES_SETUP ${name}_deck)
  add_test(NAME classic_cli_${name}_semantics
    COMMAND test_cli_output ${name} ${output})
  set_tests_properties(classic_cli_${name}_semantics PROPERTIES
    FIXTURES_REQUIRED ${name}_deck)
endfunction()
classic_cli_fixture(constants constants.asm)
classic_cli_fixture(forms forms.asm)
classic_cli_fixture(profile cli-profile.asm --profile s370)

# Rich desktop CLI failure/path checks use the optional development runtime.
# Pure C unit tests and independent positive deck checks do not depend on it.
find_program(CREXX_EXECUTABLE crexx)
if(CREXX_EXECUTABLE)
  add_test(NAME classic_cli_contracts
    COMMAND ${CREXX_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cli.crexx
      --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:test_cli_output>
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures
      ${CMAKE_CURRENT_BINARY_DIR}/cli-contracts)
  set_tests_properties(classic_cli_contracts PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endif()
