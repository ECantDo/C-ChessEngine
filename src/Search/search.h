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
#include "Moves/generate_moves.h"
#include "quiescence_search.h"
#include "Evaluation/evaluation.h"
#include "transposition_table.h"

#include "Evaluation/opening_book.h"

#define MAX_PLY 128

extern bool g_printInfo;
extern bool useOpeningBook;
extern std::atomic<bool> stopSearch;

struct BestMove {
	Move bestMove;
	int score;
	int plys;
	int selDepth;
	bool completed;
	std::vector<Move> pv;
};

struct SearchValues {
	uint64_t nodes;
	uint64_t tbHits;
};

struct ThreadResult {
	Move bestMove;
	int bestScore;
	int depth;
	int selDepth;
	std::vector<Move> pv;
	unsigned long long nodes;
	unsigned long long tbHits;
};


BestMove selectMove(Board &board, int maxDepth, long timeLimitMS, SearchValues &searchValues, int numThreads = 1);

bool isKingInCheck(const Board &board, int color);

bool insufficientMaterial(Board &board);

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads);


inline int scoreMoveForOrdering(Move m, const Board &board, int ply,
								Move killers[MAX_PLY][2],
								unsigned long long history[2][64][64]) {
	int flags = getMoveFlags(m);
	int to = getMoveTo(m);
	int from = getMoveFrom(m);

	// 1. PROMOTIONS (especially capturing promotions)
	if (flags & MOVE_FLAG_PROMOTION) {
		int promoType = flags & 0x3;
		int baseScore = 0;
		switch (promoType) {
			case PROMOTE_TO_QUEEN:
				baseScore = 9000000;
				break;
			case PROMOTE_TO_ROOK:
				baseScore = 5000000;
				break;
			case PROMOTE_TO_BISHOP:
				baseScore = 3300000;
				break;
			case PROMOTE_TO_KNIGHT:
				baseScore = 3000000;
				break;
			default:
				baseScore = 3000000;
				break;
		}

		// Bonus for capturing promotions
		if (flags & MOVE_FLAG_CAPTURE) {
			Piece victim = board.pieceAtSquare(to);
			baseScore += getPieceValue(victim) * 10;
		}

		return baseScore;
	}

	// 2. CAPTURES (MVV-LVA)
	if (flags & MOVE_FLAG_CAPTURE) {
		Piece victim = board.pieceAtSquare(to);
		Piece attacker = board.pieceAtSquare(from);
		int mvvLva = getPieceValue(victim) * 10 - getPieceValue(attacker);
		return 1000000 + mvvLva;
	}

	// 3. KILLER MOVES (non-captures that caused cutoffs at this ply)
	if (ply < MAX_PLY) {
		if (m == killers[ply][0]) return 90000;
		if (m == killers[ply][1]) return 80000;
	}

	// 4. CASTLING
	if (flags & MOVE_FLAG_CASTLING) return 50000;

	// 5. HISTORY (statistical goodness - capped below killers)
	int color = (board.turn == 1) ? 0 : 1;
	int historyScore = history[color][from][to];
	return std::min((int) historyScore, 70000); // Cap to stay below killers
}


#endif //CHESSENGINE_SEARCH_H
