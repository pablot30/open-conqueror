# Unit tests for the firmware units, built with the host computer's own C++
# compiler and run with CTest (D5). Every unit folder with a tests/ folder
# becomes one test executable and one CTest test, named after the unit.
cmake_minimum_required(VERSION 3.20)
project(open_conqueror_unit_tests CXX)

# C++11, to match the Arduino AVR toolchain.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(UNO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../uno)
file(GLOB UNIT_DIRS LIST_DIRECTORIES true CONFIGURE_DEPENDS ${UNO_DIR}/*/*)

# Every unit's headers are visible, so a unit's tests can use another
# unit's types. Each test executable compiles only its own unit's sources.
set(UNIT_INCLUDE_DIRS "")
foreach(UNIT_DIR ${UNIT_DIRS})
  if(IS_DIRECTORY ${UNIT_DIR}/src)
    list(APPEND UNIT_INCLUDE_DIRS ${UNIT_DIR}/src)
  endif()
endforeach()

enable_testing()

foreach(UNIT_DIR ${UNIT_DIRS})
  if(NOT IS_DIRECTORY ${UNIT_DIR}/tests)
    continue()
  endif()
  get_filename_component(UNIT ${UNIT_DIR} NAME)
  file(GLOB UNIT_SOURCES CONFIGURE_DEPENDS ${UNIT_DIR}/src/*.cpp)
  file(GLOB UNIT_TESTS CONFIGURE_DEPENDS ${UNIT_DIR}/tests/*.cpp)
  add_executable(${UNIT}_tests main.cpp ${UNIT_TESTS} ${UNIT_SOURCES})
  target_include_directories(${UNIT}_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${UNIT_INCLUDE_DIRS})
  if(MSVC)
    target_compile_options(${UNIT}_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(${UNIT}_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
  endif()
  add_test(NAME ${UNIT} COMMAND ${UNIT}_tests)
endforeach()
