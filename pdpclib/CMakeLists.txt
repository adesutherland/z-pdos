# Focused host QA only; this does not build or link a complete target PDPCLIB.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.16)
  project(pdpclib_host_qa C)
  set(CMAKE_C_STANDARD 90)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_C_EXTENSIONS OFF)
  option(MF_SANITIZERS "Enable host address/undefined-behaviour QA" OFF)
  enable_testing()
  if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Werror -pedantic-errors)
    if(MF_SANITIZERS)
      add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
      add_link_options(-fsanitize=address,undefined)
    endif()
  endif()
endif()

# The fixture depends on GCC-compatible native varargs and symbol labels.
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  include(CheckTypeSize)
  check_type_size("unsigned long" PDPCLIB_HOST_ULONG_BYTES LANGUAGE C)
  if(PDPCLIB_HOST_ULONG_BYTES EQUAL 8)
    add_executable(test_pdpclib_limits tests/test_limits.c)
    add_test(NAME pdpclib_limits_long64 COMMAND test_pdpclib_limits)
  endif()
  add_library(pdpclib_qa_report OBJECT tests/report_failure.c)
  # Inherited stdio was written for historical compilers, not modern host
  # warning policy. Keep strict warnings on our harness and native stubs.
  set_source_files_properties(tests/stdio_under_test.c PROPERTIES
    COMPILE_OPTIONS "-w;-Wno-error;-Wno-pedantic")
  foreach(profile IN ITEMS pdos390 mvs_control)
    add_executable(test_pdpclib_stdio_${profile}
      tests/stdio_under_test.c tests/native_fixture.c tests/test_stdio.c
      $<TARGET_OBJECTS:pdpclib_qa_report>)
    target_include_directories(test_pdpclib_stdio_${profile} PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(test_pdpclib_stdio_${profile} PRIVATE
      __MVS__ __USEBIVA__)
    target_compile_options(test_pdpclib_stdio_${profile} PRIVATE -fno-builtin)
    add_executable(test_pdpclib_string_${profile} tests/test_string.c)
    target_compile_definitions(test_pdpclib_string_${profile} PRIVATE __MVS__)
    target_compile_options(test_pdpclib_string_${profile} PRIVATE -fno-builtin)
    if(profile STREQUAL "pdos390")
      target_compile_definitions(test_pdpclib_stdio_${profile} PRIVATE __PDOS390__)
      target_compile_definitions(test_pdpclib_string_${profile} PRIVATE __PDOS390__)
    endif()
    add_test(NAME pdpclib_stdio_${profile} COMMAND test_pdpclib_stdio_${profile})
    add_test(NAME pdpclib_string_${profile} COMMAND test_pdpclib_string_${profile})
  endforeach()
else()
  message(STATUS "PDPCLIB native-stub host QA requires GCC or Clang")
endif()
