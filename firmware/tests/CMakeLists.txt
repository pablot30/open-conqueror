# Unit tests for the firmware logic that doesn't touch hardware (D5).
# They build with the computer's own C++ compiler and run with CTest.
cmake_minimum_required(VERSION 3.20)
project(open_conqueror_firmware_tests CXX)

# C++11, to match the Arduino AVR toolchain.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../uno/src/core)

add_executable(core_tests
  main.cpp
  test_protocol.cpp
  test_drive.cpp
  test_battery.cpp
  test_debounce.cpp
  ${CORE_DIR}/protocol.cpp
  ${CORE_DIR}/drive.cpp
  ${CORE_DIR}/battery.cpp
  ${CORE_DIR}/debounce.cpp
)
target_include_directories(core_tests PRIVATE ${CORE_DIR})

if(MSVC)
  target_compile_options(core_tests PRIVATE /W4 /WX)
else()
  target_compile_options(core_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
endif()

enable_testing()
add_test(NAME core_tests COMMAND core_tests)
