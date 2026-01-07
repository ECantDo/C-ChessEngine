cmake_minimum_required(VERSION 3.28)
project(chessEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -mavx2 -march=native")

include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(Threads REQUIRED)

add_executable(chessEngine
        src/main.cpp

        src/Board/board.h
        src/Board/board.cpp

        src/Moves/generate_moves.h
        src/Moves/generate_moves.cpp

        src/Board/move.h
        src/Board/move.cpp

        src/Board/piece.h
        src/Board/piece.cpp


        src/Search/search.h
        src/Search/search.cpp
        src/Board/zobrist_hash.cpp
        src/Board/zobrist_hash.h
        src/Search/transposition_table.h
        src/Search/transposition_table.cpp
        src/Evaluation/evaluation.cpp
        src/Evaluation/evaluation.h
        src/Search/quiescence_search.cpp
        src/Search/quiescence_search.h

        tests/MoveGeneration/test_move_generation.cpp
        tests/MoveGeneration/test_move_generation.h
        src/Evaluation/opening_book.cpp
        src/Evaluation/opening_book.h
        src/Moves/magicBitboards.cpp
        src/Moves/magicBitboards.h
        src/Moves/move_list.cpp
        src/Moves/move_list.h
        src/NNUE/nnue_eval.h
        src/NNUE/nnue_eval.cpp
        src/NNUE/self_play_training.cpp
        src/NNUE/self_play_training.h

)

add_executable(testChessEngine tests/test_all.cpp
        tests/test_all.cpp

        tests/Board/test_board.cpp
        tests/Board/test_board.h
        src/Board/board.h
        src/Board/board.cpp

        src/Board/piece.h
        src/Board/piece.cpp

        tests/Board/test_moves.h
        tests/Board/test_moves.cpp
        tests/testing_essentials.h

        src/Board/zobrist_hash.cpp
        src/Board/zobrist_hash.h
        src/Board/move.h
        src/Board/move.cpp

        src/Moves/magicBitboards.cpp
        src/Moves/magicBitboards.h

        src/Moves/generate_moves.cpp
        src/Moves/generate_moves.h
        tests/MoveGeneration/test_move_generation.cpp
        tests/MoveGeneration/test_move_generation.h

        src/Search/search.cpp

        tests/Board/test_zobrist.cpp
        tests/Board/test_zobrist.h
        src/Search/transposition_table.h
        src/Search/transposition_table.cpp
        src/Evaluation/evaluation.cpp
        src/Evaluation/evaluation.h
        src/Search/quiescence_search.cpp
        src/Search/quiescence_search.h

        src/Evaluation/opening_book.cpp
        src/Evaluation/opening_book.h
)

target_compile_options(chessEngine PRIVATE
        -O3
        -mavx2
        -march=native
)

target_compile_options(testChessEngine PRIVATE
        -O3
        -mavx2
        -march=native
)

target_link_libraries(chessEngine PRIVATE Threads::Threads)

