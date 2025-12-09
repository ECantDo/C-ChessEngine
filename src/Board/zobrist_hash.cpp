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
        std::mt19937_64 rng(0x9778946512784651ULL);
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


    int getZobristIndex(char piece) {
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
                throw std::invalid_argument(std::format("Invalid piece: {}", piece));
        }
    }
}