cmake_minimum_required(VERSION 3.30)
project(chessEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(chessEngine main.cpp
        Board/board.h
        Board/board.cpp)
