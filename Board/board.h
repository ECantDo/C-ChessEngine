//
// Created by ECanDo on 2025-08-22.
//

#include <cstdint>
#include <string>

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H

class Board {
public:
    // Bitboards
    uint64_t whitePawns;
    uint64_t whiteKnights;
    uint64_t whiteBishops;
    uint64_t whiteRooks;
    uint64_t whiteQueens;
    uint64_t whiteKing;

    uint64_t blackPawns;
    uint64_t blackKnights;
    uint64_t blackBishops;
    uint64_t blackRooks;
    uint64_t blackQueens;
    uint64_t blackKing;

    // Constructor
    Board();

    // Load starting position
    void loadStartPosition();

    // Print board
    void printBoard() const;

private:
    char pieceAtSquare(int square) const;

};

#endif //CHESSENGINE_BOARD_H
