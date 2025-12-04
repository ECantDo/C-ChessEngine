//
// Created by ECanDo on 2025-08-23.
//

#include "piece.h"

int getValue(char piece) {

    // NONE_PIECE, <BLACK/WHITE>_KING; all worth 0, caught by the default case
    switch (piece) {
        case WHITE_PAWN:
            return 100;
        case WHITE_KNIGHT:
            return 320;
        case WHITE_ROOK:
            return 500;
        case WHITE_BISHOP:
            return 330;
        case WHITE_QUEEN:
            return 900;

        case BLACK_PAWN:
            return -100;
        case BLACK_KNIGHT:
            return -320;
        case BLACK_ROOK:
            return -500;
        case BLACK_BISHOP:
            return -330;
        case BLACK_QUEEN:
            return -900;

        default:
            return 0;
    }
}