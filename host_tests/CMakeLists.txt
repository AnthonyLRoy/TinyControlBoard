cmake_minimum_required(VERSION 3.20)

project(TinyControlBoardHostTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(tiny_control_board_host_tests
    test_logic.cpp
    ../lib/protocol/uartProtocol.cpp
    ../lib/app/ActionUartDispatcher.cpp
    ../lib/app/ControlBoardInputDispatcher.cpp
)

target_include_directories(tiny_control_board_host_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/actions
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/app
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/indicators
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/protocol
)

add_test(NAME tiny_control_board_host_tests COMMAND tiny_control_board_host_tests)