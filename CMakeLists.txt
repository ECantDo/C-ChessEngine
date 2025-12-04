cmake_minimum_required(VERSION 3.30)
project(chessEngine)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(chessEngine src/main.cpp
        src/Board/board.h
        src/Board/board.cpp
        src/Board/piece.h
        src/Board/piece.cpp
)

add_executable(testChessEngine tests/test_all.cpp
        tests/Board/test_board.cpp
        tests/test_all.cpp
        tests/Board/test_board.h
        src/Board/board.h
        src/Board/board.cpp
        src/Board/piece.h
        src/Board/piece.cpp
)

