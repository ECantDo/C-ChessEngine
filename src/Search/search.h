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
inline int lmrTable[MAX_PLY][MAX_PLY];

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


BestMove selectMove(Board &board, int maxDepth, long timeLimitMS, SearchValues &searchValues, int numThreads = 1,
					uint64_t maxNodes = -1);

bool isKingInCheck(const Board &board, int color);

bool insufficientMaterial(Board &board);

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads, uint64_t maxNodes);

// SEE piece values – intentionally simpler than eval values
static constexpr int SEE_VALUE[7] = {
	0, // TYPE_NONE   = 0
	100, // TYPE_PAWN   = 1
	500, // TYPE_ROOK   = 2
	310, // TYPE_KNIGHT = 3
	330, // TYPE_BISHOP = 4
	950, // TYPE_QUEEN  = 5
	20000 // TYPE_KING   = 6
};

inline int seeValue(Piece p) {
	if (p == NONE) return 0;
	return SEE_VALUE[getPieceType(p)]; // getPieceType gives 0-6 directly
}

// Returns a bitboard of ALL pieces (both colours) attacking `sq`,
// given the current occupancy `occ`.  Used by SEE to find discovered attackers.
inline uint64_t allAttackersTo(const Board &board, int sq, uint64_t occ) {
	uint64_t result = 0;

	// Pawns
	result |= (PAWN_ATTACKS[WHITE][sq] & board.blackPawns);
	result |= (PAWN_ATTACKS[BLACK][sq] & board.whitePawns);

	// Knights
	result |= (KNIGHT_ATTACKS[sq] & (board.whiteKnights | board.blackKnights));

	// Kings
	result |= (KING_ATTACKS[sq] & (board.whiteKing | board.blackKing));

	// Sliding pieces (respect current occupancy so discovered attacks show up)
	uint64_t rookSliders = board.whiteRooks | board.blackRooks
						   | board.whiteQueens | board.blackQueens;
	uint64_t bishopSliders = board.whiteBishops | board.blackBishops
							 | board.whiteQueens | board.blackQueens;

	result |= (getRookAttacks(sq, occ) & rookSliders);
	result |= (getBishopAttacks(sq, occ) & bishopSliders);

	return result;
}

// Returns the least-valuable piece in `attackers` belonging to `color`,
// and sets `piece` to what it is.  Returns 0 if none found.
inline uint64_t leastValuableAttacker(const Board &board, uint64_t attackers,
									  int color, Piece &piece) {
	// Order: pawn, knight, bishop, rook, queen, king
	uint64_t mine;

	// Pawns
	mine = attackers & (color == 1 ? board.whitePawns : board.blackPawns);
	if (mine) {
		piece = (color == 1 ? WHITE_PAWN : BLACK_PAWN);
		return mine & -mine;
	}

	// Knights
	mine = attackers & (color == 1 ? board.whiteKnights : board.blackKnights);
	if (mine) {
		piece = (color == 1 ? WHITE_KNIGHT : BLACK_KNIGHT);
		return mine & -mine;
	}

	// Bishops
	mine = attackers & (color == 1 ? board.whiteBishops : board.blackBishops);
	if (mine) {
		piece = (color == 1 ? WHITE_BISHOP : BLACK_BISHOP);
		return mine & -mine;
	}

	// Rooks
	mine = attackers & (color == 1 ? board.whiteRooks : board.blackRooks);
	if (mine) {
		piece = (color == 1 ? WHITE_ROOK : BLACK_ROOK);
		return mine & -mine;
	}

	// Queens
	mine = attackers & (color == 1 ? board.whiteQueens : board.blackQueens);
	if (mine) {
		piece = (color == 1 ? WHITE_QUEEN : BLACK_QUEEN);
		return mine & -mine;
	}

	// King
	mine = attackers & (color == 1 ? board.whiteKing : board.blackKing);
	if (mine) {
		piece = (color == 1 ? WHITE_KING : BLACK_KING);
		return mine & -mine;
	}

	return 0; // no attacker of this color
}

// Main SEE function.
// `move`  – the capture move being evaluated
// `board` – position BEFORE the move is made
// Returns net material swing from the perspective of the side making the move.
inline int see(Move move, const Board &board) {
	int toSq = getMoveTo(move);
	int fromSq = getMoveFrom(move);

	Piece captured = board.pieceAtSquare(toSq);
	if (captured == NONE) return 0;

	int gain[32];
	int depth = 0;

	gain[0] = seeValue(captured);

	uint64_t occ = board.getWhiteBitboard() | board.getBlackBitboard();

	// Remove the moving piece from occupancy
	occ &= ~(1ULL << fromSq);

	// Get all attackers after the first capture
	uint64_t attackers = allAttackersTo(board, toSq, occ);

	Piece attacker = board.pieceAtSquare(fromSq);
	int sideToMove = -board.turn; // opponent recaptures first

	while (depth < 30) {
		depth++;
		gain[depth] = seeValue(attacker) - gain[depth - 1];

		// If even getting the piece for free doesn't help, stop
		if (std::max(-gain[depth - 1], gain[depth]) < 0) break;

		// Find least valuable attacker for current side
		Piece nextAttacker;
		uint64_t attackerBit = leastValuableAttacker(board, attackers, sideToMove, nextAttacker);
		if (!attackerBit) break;

		// Remove attacker and find newly revealed sliding attackers
		occ &= ~attackerBit;
		attackers = allAttackersTo(board, toSq, occ);
		attackers &= occ; // only pieces still on the board

		attacker = nextAttacker;
		sideToMove = -sideToMove;
	}

	// Minimax back
	while (--depth > 0) {
		gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
	}

	return gain[0];
}


inline int scoreMoveForOrdering(Move m, const Board &board, int ply,
								Move killers[MAX_PLY][2],
								unsigned long long history[2][64][64]) {
	const int flags = getMoveFlags(m);
	const int to = getMoveTo(m);
	const int from = getMoveFrom(m);

	int score = 0;

	// ── 1. Promotions ────────────────────────────────────────────────────────
	if (flags & MOVE_FLAG_PROMOTION) {
		switch (const int promoType = flags & 0x3) {
			case PROMOTE_TO_QUEEN: score += 9000000;
				break;
			case PROMOTE_TO_ROOK: score += 5000000;
				break;
			case PROMOTE_TO_BISHOP: score += 3300000;
				break;
			case PROMOTE_TO_KNIGHT: score += 3200000;
				break;
			default: score += 3000000;
				break;
		}
		// Fall through to also score the capture component if any
	}

	// ── 2. Captures (SEE-based)
	if (flags & MOVE_FLAG_CAPTURE) {
		const int seeScore = see(m, board);

		if (seeScore >= 0) {
			// Winning or equal capture — above killers
			score += 1000000 + seeScore;
		} else {
			// Losing capture — below history, searched last
			score += -500000 + seeScore;
		}

		// If also a promotion, the promotion bonus already puts it near the top;
		// add the capture value on top so promoting captures rank correctly
		// among themselves (already handled since score is additive).
		return score;
	}

	// ── 3. Quiet move bonuses

	// Killer moves
	if (ply < MAX_PLY) {
		if (m == killers[ply][0]) score += 90000;
		else if (m == killers[ply][1]) score += 80000;
	}

	// Castling
	if (flags & MOVE_FLAG_CASTLING) score += 50000;

	// History heuristic
	const int color = (board.turn == 1) ? 0 : 1;
	score += std::min(static_cast<int>(history[color][from][to]), 70000);

	return score;
}

inline void initLmrTable() {
	for (int depth = 0; depth < MAX_PLY; depth++) {
		for (int movesSearched = 0; movesSearched < MAX_PLY; movesSearched++) {
			lmrTable[depth][movesSearched] = std::max(0, static_cast<int>(
														  0.75 + std::log(depth) * std::log(movesSearched) / 3.0
													  ));
		}
	}
}


#endif //CHESSENGINE_SEARCH_H
