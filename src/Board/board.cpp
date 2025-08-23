//
// Created by ECanDo on 2025-08-22.
//

#include "board.h"
#include "piece.h"
#include <iostream>

Board::Board()
        : whitePawns(0), whiteBishops(0), whiteKing(0), whiteKnights(0), whiteQueens(0), whiteRooks(0),
          blackPawns(0), blackBishops(0), blackKing(0), blackKnights(0), blackQueens(0), blackRooks(0) {}

void Board::loadStartPosition() {
    // Pawns
    whitePawns = 0x000000000000FF00ULL;
    blackPawns = 0x00FF000000000000ULL;

    // Rooks
    whiteRooks = 0x0000000000000081ULL;
    blackRooks = 0x8100000000000000ULL;

    // Knights
    whiteKnights = 0x0000000000000042ULL;
    blackKnights = 0x4200000000000000ULL;

    // Bishops
    whiteBishops = 0x0000000000000024ULL;
    blackBishops = 0x2400000000000000ULL;

    // Queens
    whiteQueens = 0x0000000000000008ULL;
    blackQueens = 0x0800000000000000ULL;

    // Kings
    whiteKing = 0x0000000000000010ULL;
    blackKing = 0x1000000000000000ULL;
}

// Helper: which piece is at a square
char Board::pieceAtSquare(int square) const {
    uint64_t mask = 1ULL << square;

    if (whitePawns & mask) return WHITE_PAWN;
    if (whiteKnights & mask) return WHITE_KNIGHT;
    if (whiteBishops & mask) return WHITE_BISHOP;
    if (whiteRooks & mask) return WHITE_ROOK;
    if (whiteQueens & mask) return WHITE_QUEEN;
    if (whiteKing & mask) return WHITE_KING;

    if (blackPawns & mask) return BLACK_PAWN;
    if (blackKnights & mask) return BLACK_KNIGHT;
    if (blackBishops & mask) return BLACK_BISHOP;
    if (blackRooks & mask) return BLACK_ROOK;
    if (blackQueens & mask) return BLACK_QUEEN;
    if (blackKing & mask) return BLACK_KING;

    return NONE;
}

// Print Board
void Board::printBoard() const {
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            std::cout << pieceAtSquare(square) << " ";
        }
        std::cout << "\n";
    }
}

uint64_t Board::getBitboard(char piece) {
    switch (piece) {
        case WHITE_PAWN:
            return whitePawns;
        case WHITE_KNIGHT:
            return whiteKnights;
        case WHITE_ROOK:
            return whiteRooks;
        case WHITE_BISHOP:
            return whiteBishops;
        case WHITE_QUEEN:
            return whiteQueens;
        case WHITE_KING:
            return whiteKing;
        case BLACK_PAWN:
            return blackPawns;
        case BLACK_KNIGHT:
            return blackKnights;
        case BLACK_ROOK:
            return blackRooks;
        case BLACK_BISHOP:
            return blackBishops;
        case BLACK_QUEEN:
            return blackQueens;
        case BLACK_KING:
            return blackKing;
        default:
            return 0;
    }
}
//======================================================================================================================
// Non-class
//======================================================================================================================

int getBoardIndex(int file, int rank) {
    if (file < 0 || file > 7 || rank < 0 || rank > 7)
        return -1;
    return file * 8 + rank;
}

int getBoardIndex(char file, char rank) {
    return getBoardIndex(file - 'a', rank - '1');
}

std::string getBoardPosition(int index) {
    if (index < 0 || index >= 64) return "";

    int file = index >> 3; // index / 8;
    int rank = index & 7; // index % 8;

    return std::string()
           + static_cast<char>(file + 'a')
           + static_cast<char>(rank + '1');
}