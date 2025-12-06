//
// Created by ECanDo on 2025-12-04.
//

#ifndef CHESSENGINE_GENERATE_MOVES_H
#define CHESSENGINE_GENERATE_MOVES_H

#include "string"
#include <cstdio>
#include <vector>

#include "move.h"
#include "board.h"

constexpr size_t MAX_MOVES = 256;

/**
 * No return; pass in the array by reference to avoid copying the array
 * @param board The board position to generate moves for
 * @param moveList The array to output the moves into
 * @param moveCount The number of moves found.
 */
void generatePseudoLegalMoves(Board &board, std::vector<Move> &moveList);

void generateLegalMoves(Board &board, std::vector<Move> &moveList, size_t &moveCount);

/**
 * Generate pseudo legal moves
 * @param board
 * @param moves
 */
void generateKingMoves(const Board &board, std::vector<Move> &moves);

void generateRookMoves(const Board &board, std::vector<Move> &moves);

// HELPER
bool isEnemyPiece(const Board &board, int square, int myColor);

bool isEmpty(const Board &board, int square);

bool isValidSquare(int square);

// OFFSETS
const int kingOffsets[8] = {-9, -8, -7, -1, 1, 7, 8, 9};

const int rookOffsets[4] = {8, -8, 1, -1};

#endif //CHESSENGINE_GENERATE_MOVES_H
