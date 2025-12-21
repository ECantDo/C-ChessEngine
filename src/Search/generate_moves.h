//
// Created by ECanDo on 2025-12-04.
//

#ifndef CHESSENGINE_GENERATE_MOVES_H
#define CHESSENGINE_GENERATE_MOVES_H

#include "string"
#include <cstdio>
#include <vector>

#include "Board/move.h"
#include "Board/board.h"
#include "magicBitboards.h"

constexpr size_t MAX_MOVES = 256;

/**
 * No return; pass in the array by reference to avoid copying the array
 * @param board The board position to generate moves for
 * @param moveList The array to output the moves into
 */
void generateLegalMoves(Board &board, std::vector<Move> &moveList, bool capturesOnly = false);

// HELPER
bool isEnemyPiece(const Board &board, int square, int myColor);

bool isEmpty(const Board &board, int square);

bool isValidSquare(int square);

inline bool isSquareAttacked(const Board &board, int square, int attackingColor) {
	if (square < 0 || square > 63){
		std::cerr << "Trying to check if square " << square << " is attacked" << std::endl << std::flush;
		return false;
	}

	// Pre-calculate once
	uint64_t blockers = board.getBlackBitboard() | board.getWhiteBitboard();

	uint64_t enemyRooks, enemyBishops, enemyQueens, enemyKnights, enemyKing, enemyPawns;

	if (attackingColor == 1) {
		enemyRooks = board.whiteRooks;
		enemyBishops = board.whiteBishops;
		enemyQueens = board.whiteQueens;
		enemyKnights = board.whiteKnights;
		enemyKing = board.whiteKing;
		enemyPawns = board.whitePawns;
	} else {
		enemyRooks = board.blackRooks;
		enemyBishops = board.blackBishops;
		enemyQueens = board.blackQueens;
		enemyKnights = board.blackKnights;
		enemyKing = board.blackKing;
		enemyPawns = board.blackPawns;
	}

	// 1. Pawn attacks (using precomputed table)
	if (enemyPawns & PAWN_ATTACKS[attackingColor == 1 ? BLACK : WHITE][square]) {
		return true;
	}

	// 2. Knight attacks
	if (enemyKnights & KNIGHT_ATTACKS[square]) return true;

	// 3. King attacks
	if (enemyKing & KING_ATTACKS[square]) return true;

	// 4. Sliding pieces
	uint64_t rookAttacks = getRookAttacks(square, blockers);
	if (rookAttacks & (enemyRooks | enemyQueens)) return true;

	uint64_t bishopAttacks = getBishopAttacks(square, blockers);
	if (bishopAttacks & (enemyBishops | enemyQueens)) return true;

	return false;
}

// OFFSETS
const int kingOffsets[8] = {-9, -8, -7, -1, 1, 7, 8, 9};

const int rookOffsets[4] = {8, -8, 1, -1};

const int bishopOffsets[4] = {9, 7, -9, -7};


void generateRookMoves(const Board &board, std::vector<Move> &moveList);


#endif //CHESSENGINE_GENERATE_MOVES_H
