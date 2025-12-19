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
typedef uint32_t Move;

/* Flag constants */
#define MOVE_FLAG_CAPTURE 0x10
#define MOVE_FLAG_PROMOTION  0x20
#define MOVE_FLAG_EN_PASSANT 0x40
#define MOVE_FLAG_CASTLING   0x80

/* Promotion pieces */
#define PROMOTE_TO_KNIGHT 0
#define PROMOTE_TO_BISHOP 1
#define PROMOTE_TO_ROOK   2
#define PROMOTE_TO_QUEEN  3

struct UndoInfo {
    Piece capturedPiece;
    int enPassantSquare;
    uint8_t castlingRights;
    int halfMoveClock;
    uint64_t zobristHash;
};

inline Move encodeMove(int from, int to, int flags){
    return (Move)((flags << 12) | (to << 6) | from);
}

inline int getMoveFrom(Move m){
    return m & 0x3F; // Lowest 5 bits (0 to 63 for location)
}

inline int getMoveTo(Move m) {
    return (m >> 6) & 0x3F; // Middle 5 bits (0 to 63 for location)
}

inline int getMoveFlags(Move m) {
    return (m >> 12);
}

std::string moveToString(Move m);

Move stringToMove(std::string &str, const Board &board);

#endif //CHESSENGINE_MOVE_H
