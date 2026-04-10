//
// Created by ECanDo on 2025-12-08.
//

#include <format>
#include "zobrist_hash.h"

namespace Zobrist {
    uint64_t pieceSquare[12][64];
    uint64_t sideToMove;
    uint64_t castlingRights[16];
    uint64_t enPassantFile[8];

    void init() {
        std::mt19937_64 rng(0x4789616ABCAF8461ULL);
//        std::mt19937_64 rng(0x9778946512784651ULL); // ~6500
        std::uniform_int_distribution<uint64_t> dist;

        for (int piece = 0; piece < 12; piece++) {
            for (int square = 0; square < 64; square++) {
                pieceSquare[piece][square] = dist(rng);
            }
        }

        sideToMove = dist(rng);

        for (int i = 0; i < 16; i++) {
            castlingRights[i] = dist(rng);
        }

        for (int i = 0; i < 8; i++) {
            enPassantFile[i] = dist(rng);
        }
    }

    int getZobristIndex(const Piece piece) {
        assert(piece != NONE);
        return PIECE_SQUARE_INDEXES[piece];
    }

}