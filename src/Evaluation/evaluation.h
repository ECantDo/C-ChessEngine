//
// Created by ECanDo on 2025-12-09.
//

#ifndef CHESSENGINE_EVALUATION_H
#define CHESSENGINE_EVALUATION_H

#include "Board/board.h"
#include "Moves/generate_moves.h"
#include "Search/transposition_table.h"

// =====================================================================================================================
// Main Evaluation Functions
// =====================================================================================================================
Score evaluateBoardNNUE(Board &board);


#endif //CHESSENGINE_EVALUATION_H
