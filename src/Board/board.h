//
// Created by ECanDo on 2025-08-22.
//

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H

#include "move.h"
#include "piece.h"
#include "zobrist_hash.h"

#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>

class Board {
public:
    // Constructor
    Board();

    explicit Board(std::string &fen);

    // Load starting position
    void loadStartPosition();

    bool loadFenPosition(std::string &fen);

    [[nodiscard]] std::string generateFen() const;

    // Print Board
    void printBoard() const;

    [[nodiscard]] uint64_t getBitboard(char piece) const;

    [[nodiscard]] uint64_t getWhiteBitboard() const;

    [[nodiscard]] uint64_t getBlackBitboard() const;

    // Move making
    /**
     * Make a move, assumes that the move is a legal move.
     * @param m The move to make
     * @return
     */
    UndoInfo makeMove(Move m);

    void unmakeMove(Move m, const UndoInfo &undoInfo);


    [[nodiscard]] char pieceAtSquare(int square) const;

    [[nodiscard]] uint64_t *getBitboardPointer(char piece);

    void setPieceAtSquare(int square, char piece);


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

    /**
     * -1 for none
     * 0 < n < 64 for the board index -> C or F rank
     */
    int enPassantSquare;

    /**
     * 1 for white
     * -1 for black
     */
    int8_t turn;

    /**
     * Castling rights:
     * 0b0000 -> no one has rights
     * 0b1000 -> White king-side
     * 0b0100 -> White queen-side
     * 0b0010 -> Black king-side
     * 0b0001 -> Black queen-side
     *
     * Example:
     * 0b1010 -> Both white and black of king-side rights
     */
    uint8_t castling;

    /**
     * For 50-move rule
     */
    int halfMoveClock;

    /**
     * Counts from 1, increments after Black's move
     */
    int fullMove;

    uint64_t zobristHash;

    [[nodiscard]] uint64_t computeZobristHash() const;

    std::vector<uint64_t> gameHistory; // Positions from actual game

    [[nodiscard]] bool isDraw() const {
        // Fifty move rule
        if (halfMoveClock >= 100) {
            return true;
        }

        // Count board repetitions
        int reps = 0;
        int startIdx = std::max(0, (int) gameHistory.size() - halfMoveClock);
        for (int i = startIdx; i < gameHistory.size(); i++) {
            if (gameHistory[i] == zobristHash) {
                reps++;
                if (reps >= 2) {
                    return true; // 3rd occurrence
                }
            }
        }
        return false;
    }

    [[nodiscard]] bool isRepetitionInSearch(const std::vector<uint64_t> &searchPath) const {
        /* Check if this exact position occurred earlier in the search */
        for (size_t i = 0; i < searchPath.size(); i += 2) {  /* Skip opponent moves */
            if (searchPath[i] == zobristHash) {
                return true;
            }
        }
        return false;
    }

};

/**
* Get the square index from file and rank
*
* @param file File 0-7 (a - h)
* @param rank Rank 0-7 (1 - 8, with 0 = rank 1)
* @return Index 0..63, or -1 if out of bounds.
*/
int getBoardIndex(int file, int rank);

/**
 * Get the square index from file and rank
 *
 * @param file File 'a'-'h'
 * @param rank Rank '1'-'8'
 * @return Index 0..63, or -1 if out of bounds.
 */
int getBoardIndex(char file, char rank);

/**
 * Get the string notation of a given index. i.e. convert `0` into "a1" or `28` into "e4"
 *
 * @param index Index of the Board to convert.
 * @return String of the index
 */
std::string getBoardPosition(int index);

#endif //CHESSENGINE_BOARD_H
