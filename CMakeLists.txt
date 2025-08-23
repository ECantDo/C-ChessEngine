cmake_minimum_required(VERSION 3.30)
project(chessEngine)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(chessEngine src/main.cpp
        src/Board/board.h
        src/Board/board.cpp
        tests/Board/testing_base.cpp
        tests/test_all.cpp
        tests/Board/test_base.h
)
