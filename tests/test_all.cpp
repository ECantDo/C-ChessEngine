//
// Created by ECanDo on 2025-08-23.
//

#include "Board/test_board.h"
#include "Board/test_moves.h"
#include "MoveGeneration/test_move_generation.h"
#include "Board/test_zobrist.h"


#include <chrono>

int main() {

    testBoard();
    testMoveMaking();

    testZobrist();

//    std::cout << "\nTESTING MOVE GENERATION\n";
//    auto start = std::chrono::high_resolution_clock::now();
//    testMoveGeneration(5);
//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    std::cout << "Took " << duration.count() << " ms\n";

//    perftDivideTesting();

    return 0;
}