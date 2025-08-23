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

    /**
     * Get the square index from file and rank
     *
     * @param file File 0-7 (a - h)
     * @param rank Rank 0-7 (1 - 8, with 0 = rank 1)
     * @return Index 0..63, or -1 if out of bounds.
     */
    int getBoardIndex(int file, int rank) const;

    /**
     * Get the square index from file and rank
     *
     * @param file File 'a'-'h'
     * @param rank Rank '1'-'8'
     * @return Index 0..63, or -1 if out of bounds.
     */
    int getBoardIndex(char file, char rank) const;

    /**
     * Get the string notation of a given index. i.e. convert `0` into "a1" or `28` into "e4"
     *
     * @param index Index of the board to convert.
     * @return String of the index
     */
    std::string getBoardPosition(int index) const;

private:
    char pieceAtSquare(int square) const;

};

#endif //CHESSENGINE_BOARD_H
