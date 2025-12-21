//
// Created by ECanDo on 2025-12-08.
//

#ifndef CHESSENGINE_ZOBRIST_HASH_H
#define CHESSENGINE_ZOBRIST_HASH_H

#include <cstdint>
#include <random>
#include <stdexcept>
#include <format>
#include "board.h"

namespace Zobrist {
	extern uint64_t pieceSquare[12][64];
	extern uint64_t sideToMove;
	extern uint64_t castlingRights[16];
	extern uint64_t enPassantFile[8];

	void init();

	inline constexpr int getZobristIndex(Piece piece) {
		switch (piece) {
			case WHITE_PAWN:
				return 0;
			case WHITE_KNIGHT:
				return 1;
			case WHITE_BISHOP:
				return 2;
			case WHITE_ROOK:
				return 3;
			case WHITE_QUEEN:
				return 4;
			case WHITE_KING:
				return 5;
			case BLACK_PAWN:
				return 6;
			case BLACK_KNIGHT:
				return 7;
			case BLACK_BISHOP:
				return 8;
			case BLACK_ROOK:
				return 9;
			case BLACK_QUEEN:
				return 10;
			case BLACK_KING:
				return 11;
			default:
				throw std::invalid_argument(std::format("Trying to get Zobrist hash for an invalid piece: "
														"{}", pieceToChar(piece)));
		}
	}
}

#endif //CHESSENGINE_ZOBRIST_HASH_H
