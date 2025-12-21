//
// Created by ECanDo on 2025-12-06.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H

#define MATE_SCORE 100000 // 100_000
#define INF_SCORE  200000 // 200_000

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

#define MAX_EXTENSIONS 10
#define MAX_PLY 128

extern bool useOpeningBook;
extern std::atomic<bool> stopSearch;

struct BestMove {
	Move bestMove;
	int score;
	unsigned long long nodes;
	unsigned long long tbHits;
	int plys;
	bool completed;
	std::vector<Move> pv;
};

struct ThreadResult {
	Move bestMove;
	int bestScore;
	int depth;
	std::vector<Move> pv;
	unsigned long long nodes;
	unsigned long long tbHits;
};


BestMove selectMove(Board &board, int maxDepth, long timeLimitMS, int numThreads = 1);

BestMove iterativeDeepening(Board &board, int maxDepth);

bool isKingInCheck(const Board &board, int color);

void rootDebugAlphaBeta(const Board &board, int maxDepth);

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads);


inline int scoreMoveForOrdering(Move m, const Board &board, int ply,
								Move killers[MAX_PLY][2], unsigned long long history[2][64][64]) {
	int flags = getMoveFlags(m);

	int to = getMoveTo(m);
	int from = getMoveFrom(m);

	// 1. CAPTURES (highest priority)
	if (flags & MOVE_FLAG_CAPTURE) {

		Piece victim = board.pieceAtSquare(to);
		Piece attacker = board.pieceAtSquare(from);
		return 1000000 + getPieceValue(victim) * 10 - getPieceValue(attacker);
	}

	// 2. PROMOTIONS
	if (flags & MOVE_FLAG_PROMOTION) {
		int promoType = flags & 0x3;
		switch (promoType) {
			case PROMOTE_TO_QUEEN:
				return 900000;
			case PROMOTE_TO_ROOK:
				return 500000;
			case PROMOTE_TO_BISHOP:
				return 330000;
			case PROMOTE_TO_KNIGHT:
				return 300000;
			default:
				break;
		}
	}

	// 3. KILLER MOVES (non-captures that caused cutoffs at this plys)
	if (ply < MAX_PLY) {
		if (m == killers[ply][0]) return 90000;
		if (m == killers[ply][1]) return 80000;
	}

	// 4. CASTLING
	if (flags & MOVE_FLAG_CASTLING) return 10000;

	// 5. HISTORY (statistical goodness)
	int color = (board.turn == 1) ? 0 : 1;
	return history[color][from][to];
}


#endif //CHESSENGINE_SEARCH_H
