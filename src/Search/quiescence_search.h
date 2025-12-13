//
// Created by ECanDo on 2025-12-09.
//

#ifndef CHESSENGINE_QUIESCENCE_SEARCH_H
#define CHESSENGINE_QUIESCENCE_SEARCH_H

#include "generate_moves.h"

class BestMove;

BestMove quiescenceSearch(Board &board, int alpha, int beta, int qDepth = 0);

#endif //CHESSENGINE_QUIESCENCE_SEARCH_H
