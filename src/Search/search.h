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
#include <thread>


#include "Board/piece.h"
#include "Board/board.h"
#include "generate_moves.h"
#include "quiescence_search.h"
#include "Evaluation/evaluation.h"
#include "transposition_table.h"

#include "Evaluation/opening_book.h"

extern bool useOpeningBook;
extern std::atomic<bool> stopSearch;

struct BestMove {
    Move bestMove;
    int score;
    unsigned long long nodes;
    unsigned long long tbHits;
    int depth;
    bool completed;
    std::vector<Move> pv;
};

struct ThreadResult {
    Move bestMove;
    int bestScore;
    int depth;
    std::vector<Move> pv;
    unsigned long long nodes;
};


BestMove selectMove(Board &board, int maxDepth, long timeLimitMS, int numThreads = 1);

BestMove iterativeDeepening(Board &board, int maxDepth);

int scoreMoveForOrdering(Move m, const Board &board);

bool isKingInCheck(const Board &board, int color);

void rootDebugAlphaBeta(const Board &board, int maxDepth);

void orderMoves(std::vector<Move> &moves, const Board &board, Move previousBest);

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads);


#endif //CHESSENGINE_SEARCH_H
