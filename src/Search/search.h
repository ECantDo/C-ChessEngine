//
// Created by ECanDo on 2025-12-06.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H

#include <string>
#include <numeric>
#include <cmath>
#include <algorithm>


#include "Board/piece.h"
#include "Board/board.h"
#include "Board/generate_moves.h"

struct BestMove {
    Move bestMove;
    int score;
    unsigned long long nodes;
    std::vector<Move> pv;
};

BestMove selectMove(Board &board, int maxDepth);

BestMove iterativeDeepening(Board &board, int maxDepth);

int scoreMoveForOrdering(Move m, const Board &board);

int evaluate(Board &board);

bool isKingInCheck(const Board &board, int color);


const int pawnTable[1]{0};

#endif //CHESSENGINE_SEARCH_H
