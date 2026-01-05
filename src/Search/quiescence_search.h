//
// Created by ECanDo on 2025-12-09.
//

#ifndef CHESSENGINE_QUIESCENCE_SEARCH_H
#define CHESSENGINE_QUIESCENCE_SEARCH_H

#include "Moves/generate_moves.h"

class SearchValues;
class BestMove;

BestMove quiescenceSearch(Board &board, int alpha, int beta, SearchValues &searchValues, int qDepth = 0);

#endif //CHESSENGINE_QUIESCENCE_SEARCH_H
