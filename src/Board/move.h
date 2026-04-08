//
// Created by ECanDo on 2025-12-03.
//

#ifndef CHESSENGINE_MOVE_H
#define CHESSENGINE_MOVE_H

#include <cstdint>
#include <string>
#include "piece.h"

class Board;

// TODO: Refactor EVERYTHING to use uint16
typedef int32_t Move;

/* Flag constants */
// #define MOVE_FLAG_CAPTURE 0x10
// #define MOVE_FLAG_PROMOTION  0x20
// #define MOVE_FLAG_EN_PASSANT 0x40
// #define MOVE_FLAG_CASTLING   0x80

/* Promotion pieces */
// #define PROMOTE_TO_KNIGHT 0
// #define PROMOTE_TO_BISHOP 1
// #define PROMOTE_TO_ROOK   2
// #define PROMOTE_TO_QUEEN  3

enum Flags {
	PROMOTE_TO_KNIGHT = 0, // 0x00 -> 0000 0000
	PROMOTE_TO_BISHOP = 1, // 0x01 -> 0000 0001
	PROMOTE_TO_ROOK = 2, // 0x02 -> 0000 0010
	PROMOTE_TO_QUEEN = 3, // 0x03 -> 0000 0011

	MOVE_FLAG_CAPTURE = 0x10,
	MOVE_FLAG_PROMOTION = 0x20,
	MOVE_FLAG_EN_PASSANT = 0x40,
	MOVE_FLAG_CASTLING = 0x80,
};

struct UndoInfo {
	Piece capturedPiece;
	int enPassantSquare;
	uint8_t castlingRights;
	uint8_t halfMoveClock;
	uint64_t zobristHash;
};

/**
 * Encoding: m-> move type, p -> promotion type, f -> from square, t -> to square
 * 0b mmmm _ pptt tttf ffff
 *
 * u-16 type?
 *
 * @param from The square moving the piece from
 * @param to The square moving the piece to
 * @param flags The flags of the move, like the promotion type
 * @return
 */
inline Move encodeMove(const int from, const int to, const int flags) {
	return flags << 12 | to << 6 | from;
}

inline int getMoveFrom(const Move m) {
	return m & 0x3F; // Lowest 5 bits (0 to 63 for location)
}

inline int getMoveTo(const Move m) {
	return (m >> 6) & 0x3F; // Middle 5 bits (0 to 63 for location)
}

inline int getMoveFlags(const Move m) {
	return (m >> 12);
}

std::string moveToString(Move m);

Move stringToMove(const std::string &str, const Board &board);

#endif //CHESSENGINE_MOVE_H
