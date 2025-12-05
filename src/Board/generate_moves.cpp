//
// Created by ECanDo on 2025-12-04.
//

#include "generate_moves.h"

void generatePseudoLegalMoves(Board &board, std::vector<Move> &moveList) {
    moveList.clear();
    moveList.reserve(MAX_MOVES);

    // Generate King moves
    generateKingMoves(board, moveList);

    // Generate Rook moves

    // Generate Bishop moves

    // Generate Queen moves

    // Generate Knight moves

    // Generate Pawn moves
}

// =====================================================================================================================
// Single generator functions
// =====================================================================================================================
void generateKingMoves(const Board &board, std::vector<Move> &moveList) {
    // Will only have 1 bit enabled; there is only 1 king
    uint64_t kingBitBoard = board.turn == 1 ? board.whiteKing : board.blackKing;

    uint64_t myPieces, theirPieces;
    if (board.turn == 1) {
        myPieces = board.getWhiteBitboard();
        theirPieces = board.getBlackBitboard();
    } else {
        myPieces = board.getBlackBitboard();
        theirPieces = board.getWhiteBitboard();
    }

    // Same as log2(x), but we know x is a power of 2
    int kingSquare = std::countr_zero(kingBitBoard);

    // Generate general, normal moves
    for (int offset: kingOffsets) {
        int targetSquare = kingSquare + offset;

        // Stop if off the board
        if (!isValidSquare(targetSquare)) {
            continue;
        }

        uint64_t targetMask = 1ULL << targetSquare;

        // If it intersects with one of my pieces, stop
        if (targetMask & myPieces) {
            continue;
        }

        int flags = 0;
        // If it intersects with opposing pieces, it's a capture move
        if (targetMask & theirPieces) {
            flags = MOVE_FLAG_CAPTURE;
        }

        // Otherwise it is pseudo-legal, add to move list
        moveList.push_back(encodeMove(kingSquare, targetSquare, flags));
    }

    uint64_t allPieceBitboard = myPieces | theirPieces;
    // White Castling moves
    if (board.turn == 1 && kingSquare == 4) {
        if ((board.castling & 0b1000) && (0x60 & allPieceBitboard) == 0) { // White kingside
            moveList.push_back(encodeMove(4, 6, MOVE_FLAG_CASTLING));
        } else if ((board.castling & 0b0100) && (0x0E & allPieceBitboard) == 0) { // White queen side
            moveList.push_back(encodeMove(4, 2, MOVE_FLAG_CASTLING));
        }
    } else if (kingSquare == 60) { // Black Castling moves
        if ((board.castling & 0b0010) && (0x6000000000000000 & allPieceBitboard) == 0) { // Black kingside
            moveList.push_back(encodeMove(60, 62, MOVE_FLAG_CASTLING));
        } else if ((board.castling & 0b0001) && (0x0E00000000000000 & allPieceBitboard) == 0) { // Black queen side
            moveList.push_back(encodeMove(60, 58, MOVE_FLAG_CASTLING));
        }
    }
}
// =====================================================================================================================
// Helper functions
// =====================================================================================================================

bool isEnemyPiece(const Board &board, int square, int myColor) {
    uint64_t bitboard = myColor == -1 ? board.getWhiteBitboard() : board.getBlackBitboard();
    uint64_t mask = 1ULL << square;

    return (bitboard & mask) != 0;
}

bool isEmpty(const Board &board, int square) {
    uint64_t bitboard = board.getWhiteBitboard() | board.getBlackBitboard();
    uint64_t mask = 1ULL << square;

    return (bitboard & mask) == 0;
}

bool isValidSquare(int square) {
    return square < 64 && square >= 0;
}