//
// Created by ECanDo on 2025-12-04.
//

#ifndef CHESSENGINE_GENERATE_MOVES_H
#define CHESSENGINE_GENERATE_MOVES_H

#include "string"
#include <cstdio>
#include <vector>

#include "Board/move.h"
#include "Board/board.h"
#include "magicBitboards.h"

constexpr size_t MAX_MOVES = 256;

/**
 * No return; pass in the array by reference to avoid copying the array
 * @param board The board position to generate moves for
 * @param moveList The array to output the moves into
 */
void generateLegalMoves(Board &board, std::vector<Move> &moveList, bool capturesOnly = false);

// HELPER
bool isEnemyPiece(const Board &board, int square, int myColor);

bool isEmpty(const Board &board, int square);

bool isValidSquare(int square);

bool isSquareAttacked(const Board &board, int square, int attackingColor);

// OFFSETS
const int kingOffsets[8] = {-9, -8, -7, -1, 1, 7, 8, 9};

const int rookOffsets[4] = {8, -8, 1, -1};

const int bishopOffsets[4] = {9, 7, -9, -7};


void generateRookMoves(const Board &board, std::vector<Move> &moveList);


#endif //CHESSENGINE_GENERATE_MOVES_H
