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
};

BestMove selectMove(Board board, long timeLimitMS);

int evaluate(Board &board, Move &move);

int countPiece(Board &board, char piece);

#endif //CHESSENGINE_SEARCH_H
