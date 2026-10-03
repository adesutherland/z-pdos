cmake_minimum_required(VERSION 3.16)
project(z_pdos_classic_tools VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(MF_SANITIZERS "Enable host address/undefined-behaviour QA" OFF)
option(MF_TRADITIONAL_MACROS "Build the optional traditional macro provider" ON)
option(MF_CLASSIC_LINKER "Build the inherited Mainframe Classic Linker sibling" ON)
option(MF_REQUIRE_FULL_CHECKS "Require cREXX and both Classic C variants" OFF)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Werror -pedantic-errors)
  if(MF_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()
enable_testing()
add_executable(test_classic_driver_quoting compiler/tests/test_driver_quoting.c)
add_test(NAME classic_driver_windows_quoting COMMAND test_classic_driver_quoting)
add_executable(test_classic_error_macros
  compiler/tests/test_host_error_macros.c compiler/src/libiberty/strerror.c)
target_include_directories(test_classic_error_macros PRIVATE
  compiler/tests/host-error-macros compiler/src/include)
# The inherited source and include_next CRT shim require GCC extensions.
set_source_files_properties(compiler/src/libiberty/strerror.c PROPERTIES
  COMPILE_OPTIONS "-Wno-pedantic;-Wno-error")
add_test(NAME classic_host_error_macros COMMAND test_classic_error_macros)
add_subdirectory(assembler)
add_subdirectory(tso31-bridge)
add_subdirectory(pdpclib)
if(MF_CLASSIC_LINKER)
  set(PDLD_SANITIZERS ${MF_SANITIZERS})
  add_subdirectory(linker)
endif()
find_program(CREXX_EXECUTABLE crexx)
if(MF_REQUIRE_FULL_CHECKS)
  if(NOT CREXX_EXECUTABLE OR NOT MF_CLASSIC_LINKER OR NOT MF_TRADITIONAL_MACROS)
    message(FATAL_ERROR "Full checks require cREXX, the linker and macros")
  endif()
  foreach(variant mvs cms)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/build/compiler/${variant}/gcc/mf-classic-cc${CMAKE_EXECUTABLE_SUFFIX}")
      message(FATAL_ERROR "Build the ${variant} Classic C compiler before configuring full checks")
    endif()
  endforeach()
endif()
if(CREXX_EXECUTABLE)
  if(MF_CLASSIC_LINKER)
    if(MF_TRADITIONAL_MACROS)
      foreach(variant mvs cms)
        set(compiler ${PROJECT_SOURCE_DIR}/build/compiler/${variant}/gcc/mf-classic-cc${CMAKE_EXECUTABLE_SUFFIX})
        if(EXISTS ${compiler})
          add_test(NAME classic_fullword_${variant}
            COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/compiler/scripts/check-fullword.crexx
              --args ${compiler} $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
              $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/fullword-${variant})
          set_tests_properties(classic_fullword_${variant} PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        endif()
      endforeach()
    endif()
    add_test(NAME classic_local_v_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-local-v.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/local-v-link)
    set_tests_properties(classic_local_v_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_overlays_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-overlays.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/overlays-link)
    set_tests_properties(classic_overlays_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_channel_word_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-channel-word.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/channel-word-link)
    set_tests_properties(classic_channel_word_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_call_interface_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-call-interface.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/call-interface-link)
    set_tests_properties(classic_call_interface_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_pdos31_maps_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-pdos31-maps.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/pdos31-maps-link)
    set_tests_properties(classic_pdos31_maps_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_pdos31_storage_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-pdos31-storage.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/pdos31-storage-link)
    set_tests_properties(classic_pdos31_storage_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_pdos31_io_link
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/tests/integration/check-pdos31-io.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/pdos31-io-link)
    set_tests_properties(classic_pdos31_io_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  endif()
  add_test(NAME pdpclib_profile_sources
    COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/pdpclib/scripts/check-profiles.crexx
      --args build/pdpclib-profile-ctest)
  set_tests_properties(pdpclib_profile_sources PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  if(MF_TRADITIONAL_MACROS)
    add_test(NAME pdpclib_source_macros
      COMMAND ${CREXX_EXECUTABLE} -nokeep ${PROJECT_SOURCE_DIR}/pdpclib/scripts/check-macros.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:test_macro_deck>
        $<TARGET_FILE:test_funhead_deck>
        ${PROJECT_BINARY_DIR}/pdpclib-source-macros)
    set_tests_properties(pdpclib_source_macros PROPERTIES
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  endif()
endif()
