# Unit tests for the firmware units, built with the host computer's own C++
# compiler and run with CTest (D5). Every unit folder with a tests/ folder
# becomes one test executable and one CTest test, named after the unit.
# Sources ending in _hw.cpp are left out: they use the board's hardware.
# tests/uses.txt can name other units to compile in, for tests of units together.
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
  # Files ending in _hw.cpp talk to the hardware and only build for the board.
  list(FILTER UNIT_SOURCES EXCLUDE REGEX "_hw\\.cpp$")
  file(GLOB UNIT_TESTS CONFIGURE_DEPENDS ${UNIT_DIR}/tests/*.cpp)
  if(NOT UNIT_TESTS)
    continue()
  endif()
  # A test that checks two units working together lists the other unit's
  # name in tests/uses.txt, one per line, and gets that unit's sources too.
  if(EXISTS ${UNIT_DIR}/tests/uses.txt)
    file(STRINGS ${UNIT_DIR}/tests/uses.txt USED_UNITS)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${UNIT_DIR}/tests/uses.txt)
    foreach(USED ${USED_UNITS})
      file(GLOB USED_SOURCES CONFIGURE_DEPENDS ${UNO_DIR}/*/${USED}/src/*.cpp)
      list(FILTER USED_SOURCES EXCLUDE REGEX "_hw\\.cpp$")
      list(APPEND UNIT_SOURCES ${USED_SOURCES})
    endforeach()
  endif()
  add_executable(${UNIT}_tests main.cpp ${UNIT_TESTS} ${UNIT_SOURCES})
  target_include_directories(${UNIT}_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${UNIT_INCLUDE_DIRS})
  if(MSVC)
    target_compile_options(${UNIT}_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(${UNIT}_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
  endif()
  add_test(NAME ${UNIT} COMMAND ${UNIT}_tests)
endforeach()
