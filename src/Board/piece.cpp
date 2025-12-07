//
// Created by ECanDo on 2025-08-23.
//

#include "piece.h"

int getPieceValue(char piece) {
    // NONE_PIECE, <BLACK/WHITE>_KING; all worth 0, caught by the default case
    switch (piece) {
        case WHITE_PAWN:
            return 100;
        case WHITE_KNIGHT:
            return 310;
        case WHITE_ROOK:
            return 500;
        case WHITE_BISHOP:
            return 330;
        case WHITE_QUEEN:
            return 950;

        case BLACK_PAWN:
            return -100;
        case BLACK_KNIGHT:
            return -310;
        case BLACK_ROOK:
            return -500;
        case BLACK_BISHOP:
            return -330;
        case BLACK_QUEEN:
            return -950;

        default:
            return 0;
    }
}