//
// Created by ECanDo on 2025-12-06.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H

#include <string>
#include <numeric>
#include <cmath>

#include "Board/piece.h"
#include "Board/board.h"
#include "Board/generate_moves.h"

struct BestMove {
    Move bestMove;
    int score;
    unsigned long long nodes;
};

BestMove selectMove(Board &board, int depth, int maxDepth);

int evaluate(Board &board);

bool isKingInCheck(const Board &board, int color);

#endif //CHESSENGINE_SEARCH_H
