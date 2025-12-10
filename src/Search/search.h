//
// Created by ECanDo on 2025-12-06.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H

#define MATE_SCORE 100000 // 100_000
#define INF_SCORE  100000000 // 100_000_000

#include <string>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <atomic>

#include "Board/piece.h"
#include "Board/board.h"
#include "generate_moves.h"
#include "Evaluation/evaluation.h"
#include "transposition_table.h"

struct BestMove {
    Move bestMove;
    int score;
    unsigned long long nodes;
    int depth;
    bool completed;
    std::vector<Move> pv;
};


BestMove selectMove(Board &board, int maxDepth, long timeLimitMS);

BestMove iterativeDeepening(Board &board, int maxDepth);

int scoreMoveForOrdering(Move m, const Board &board);

bool isKingInCheck(const Board &board, int color);

void rootDebugAlphaBeta(const Board &board, int maxDepth);

#endif //CHESSENGINE_SEARCH_H
