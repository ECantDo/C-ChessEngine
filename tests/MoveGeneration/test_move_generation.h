//
// Created by ECanDo on 2025-12-05.
//

#ifndef CHESSENGINE_TEST_MOVE_GENERATION_H
#define CHESSENGINE_TEST_MOVE_GENERATION_H

#include <cstdint>
#include "string"
#include <iostream>

#include "Board/generate_moves.h"
#include "Board/board.h"

void testMoveGeneration(int depth);

uint64_t perft(int depth, Board &board);

#endif //CHESSENGINE_TEST_MOVE_GENERATION_H
