cmake_minimum_required(VERSION 3.16)
project(z_pdos_classic_tools C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(MF_SANITIZERS "Enable host address/undefined-behaviour QA" OFF)
option(MF_TRADITIONAL_MACROS "Build the optional traditional macro provider" ON)
option(MF_CLASSIC_LINKER "Build the inherited Mainframe Classic Linker sibling" ON)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Werror -pedantic-errors)
  if(MF_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
  endif()
endif()
enable_testing()
add_subdirectory(tools/classic-as)
add_subdirectory(runtime/tso31)
add_subdirectory(pdpclib)
if(MF_CLASSIC_LINKER)
  set(PDLD_SANITIZERS ${MF_SANITIZERS})
  add_subdirectory(tools/classic-ld)
endif()
find_program(CREXX_EXECUTABLE crexx)
if(CREXX_EXECUTABLE)
  if(MF_CLASSIC_LINKER)
    if(MF_TRADITIONAL_MACROS)
      foreach(variant mvs cms)
        set(compiler ${PROJECT_SOURCE_DIR}/build/classic-cc/${variant}/gcc/mf-classic-cc)
        if(EXISTS ${compiler})
          add_test(NAME classic_fullword_${variant}
            COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/classic-cc/check-fullword.crexx
              --args ${compiler} $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
              $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/fullword-${variant})
          set_tests_properties(classic_fullword_${variant} PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        endif()
      endforeach()
    endif()
    add_test(NAME classic_local_v_link
      COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check-local-v.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/local-v-link)
    set_tests_properties(classic_local_v_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_overlays_link
      COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check-overlays.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/overlays-link)
    set_tests_properties(classic_overlays_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_channel_word_link
      COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check-channel-word.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/channel-word-link)
    set_tests_properties(classic_channel_word_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    add_test(NAME classic_call_interface_link
      COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check-call-interface.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:mf-classic-ld>
        $<TARGET_FILE:test_cli_output> ${PROJECT_BINARY_DIR}/call-interface-link)
    set_tests_properties(classic_call_interface_link PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  endif()
  add_test(NAME pdpclib_profile_sources
    COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check-pdpclib-profiles.crexx
      --args build/pdpclib-profile-ctest)
  set_tests_properties(pdpclib_profile_sources PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  if(MF_TRADITIONAL_MACROS)
    add_test(NAME pdpclib_source_macros
      COMMAND ${CREXX_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check-pdpclib-macros.crexx
        --args $<TARGET_FILE:mf-classic-as> $<TARGET_FILE:test_macro_deck>
        ${PROJECT_SOURCE_DIR}/pdpclib/mvssupa.asm
        ${PROJECT_BINARY_DIR}/pdpclib-source-macros)
    set_tests_properties(pdpclib_source_macros PROPERTIES
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  endif()
endif()
