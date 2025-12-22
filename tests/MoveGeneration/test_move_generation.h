//
// Created by ECanDo on 2025-12-05.
//

#ifndef CHESSENGINE_TEST_MOVE_GENERATION_H
#define CHESSENGINE_TEST_MOVE_GENERATION_H

#include <cstdint>
#include "string"
#include <iostream>

#include "Search/generate_moves.h"
#include "Board/board.h"
#include "Board/move.h"

void testMoveGeneration(int depth);

uint64_t perft(int depth, Board &board);

void perftDivideTesting();

uint64_t perftDivide(Board &board, int depth);

#endif //CHESSENGINE_TEST_MOVE_GENERATION_H
