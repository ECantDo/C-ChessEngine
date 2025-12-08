//
// Created by ECanDo on 2025-12-08.
//

#ifndef CHESSENGINE_ZOBRIST_HASH_H
#define CHESSENGINE_ZOBRIST_HASH_H

#include <cstdint>
#include <random>
#include "board.h"

namespace Zobrist {
    extern uint64_t pieceSquare[12][64];
    extern uint64_t sideToMove;
    extern uint64_t castlingRights[16];
    extern uint64_t enPassantFile[8];

    void init();

    int getZobristIndex(char piece);
}

#endif //CHESSENGINE_ZOBRIST_HASH_H
