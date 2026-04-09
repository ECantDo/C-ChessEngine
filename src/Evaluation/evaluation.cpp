//
// Created by ECanDo on 2025-12-09.
//

#include "evaluation.h"
#include "NNUE/nnue_eval.h"

Score evaluateBoardNNUE(Board &board) {
	return evaluateNNUE(board, g_nnueAccumulator);
}