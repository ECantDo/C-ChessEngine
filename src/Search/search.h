//
// Created by ECanDo on 2025-12-06.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H

#include <string>
#include "Board/board.h"
#include "Board/generate_moves.h"

struct BestMove {
    Move bestMove;
    int score;
};

BestMove selectMove(Board board, long timeLimitMS);

#endif //CHESSENGINE_SEARCH_H
