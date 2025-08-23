//
// Created by ECanDo on 2025-08-22.
//

#include "board.h"
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

    if (whitePawns & mask) return 'P';
    if (whiteKnights & mask) return 'N';
    if (whiteBishops & mask) return 'B';
    if (whiteRooks & mask) return 'R';
    if (whiteQueens & mask) return 'Q';
    if (whiteKing & mask) return 'K';

    if (blackPawns & mask) return 'p';
    if (blackKnights & mask) return 'n';
    if (blackBishops & mask) return 'b';
    if (blackRooks & mask) return 'r';
    if (blackQueens & mask) return 'q';
    if (blackKing & mask) return 'k';

    return '.';
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

int Board::getBoardIndex(int file, int rank) {
    if (file < 0 || file > 7 || rank < 0 || rank > 7)
        return -1;
    return file + rank * 8;
}

int Board::getBoardIndex(char file, char rank) {
    return getBoardIndex(file - 'a', rank - '1');
}

std::string Board::getBoardPosition(int index) {
    if (index < 0 || index >= 64) return "";

    int rank = index >> 3; // index / 8;
    int file = index & 7; // index % 8;

    return std::string()
           + static_cast<char>(file + 'a')
           + static_cast<char>(rank + '1');
}