//
// Created by ECanDo on 2025-12-09.
//

#include "evaluation.h"

int evaluatePawns(Board &board, int side) {
    int score = 0;
    // ==== Doubled Pawns ====
    // White pawns, subtract from total score (penalty)
    // Loop for each file
    uint64_t pawnBitboard = side == 1 ? board.whitePawns : board.blackPawns;
    for (int i = 0; i < 8; i++) {
        int extraPawnsInFile = __builtin_popcount(pawnBitboard & (FILE_MASK << i)) - 1;

        // Penalty = (x-1)^2 * 25 | x > 1 , where x = number of pawns in file
        // The idea is to have a smaller penalty for 2 pawns doubled, but a much larger one for 3+ pawns
        // with 2 pawns, penalty is -25; 3 pawns is -100, or a whole pawn, which is effectively what it is
        if (extraPawnsInFile > 0) {
            score -= extraPawnsInFile * extraPawnsInFile * 35;
        }

        // TODO:
        //  Check for passed pawn
        //  Check for isolated pawn
        //  Unsupported Pawns
    }
    // ==== Pawns around the king, push the pawns on the other side ====
    // also known as pawn shelter
    // TODO

    return score;
}

int evaluateBoard(Board &board) {
    int score = 0;

    for (char piece: ALL_PIECES) {
        uint64_t bitboard = board.getBitboard(piece);
        // Sum piece values
        score += std::popcount(bitboard) * getPieceValue(piece);

        // Piece square table values
        bool isWhite = isupper(piece);
        while (bitboard) {
            int sq = std::countr_zero(bitboard);
            bitboard &= bitboard - 1;

            if (isWhite) { // Add white score
                score += getPieceSquareValue(piece, sq);
            } else { // Subtract black score
                score -= getPieceSquareValue(piece, sq);
            }
        }
    }

    score += evaluatePawns(board, 1); // Add the score for white; when score is negative, bad for white
    score -= evaluatePawns(board, -1); // Subtract the score for black; when score is negative, good for white

    // ==== Mobility ====
    // TODO
    // Should just be [mobility_bonus * (#whitemoves - #blackmoves)] and it should be good enough (for now)


    // TODO:
    //  Open files near king
    //  Game phase
    //  Hanging pieces

    // Return from current player's perspective; black does need to be negative
    return board.turn == 1 ? score : -score;
}

int getPieceSquareValue(char piece, int square) {
    /* For black pieces, flip the square vertically */
    bool isWhite = isupper(piece);
    int sq = isWhite ? flipIndex(square) : square; // Seems backwards, but is fine

    switch (tolower(piece)) {
        case 'p':
            return pawnTable[sq];
        case 'n':
            return knightTable[sq];
        case 'b':
            return bishopTable[sq];
        case 'r':
            return rookTable[sq];
        case 'q':
            return queenTable[sq];
        case 'k':
            return kingMiddleGameTable[sq];
        default:
            return 0;
    }
}