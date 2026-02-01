//
// Created by ECanDo on 2025-12-09.
//

#include "quiescence_search.h"
#include "search.h"

BestMove quiescenceSearch(Board &board, int alpha, int beta, SearchValues &searchValues, int qDepth) {
	searchValues.nodes++;
	const int MAX_Q_DEPTH = 32;

	// If we do nothing, what's the score???
	int standPat = evaluateBoardNNUE(board);

	if (standPat >= beta) {
		return {0, beta, 1, qDepth, true, {}}; // Beta cutoff
	}

	if (standPat > alpha) {
		alpha = standPat;
	}
	// Stop quiescence if too deep
	if (qDepth >= MAX_Q_DEPTH) {
		return {0, standPat, qDepth, qDepth, true, {}};
	}


	MoveList captures;
	generateLegalMoves(board, captures, true);

	if (captures.empty()) {
		return {0, standPat, 1, qDepth, true, {}};
	}

//    orderMoves(captures, board, 0);

	int bestScore = standPat;

	for (int i = 0; i < captures.length(); i++) {
		Move move = captures.get(i);
//        std::cout << "CAPTURE! " << moveToString(move) << std::endl << std::flush;
		int captured = abs(getPieceValue(board.pieceAtSquare(getMoveTo(move))));
		if (standPat + captured + 200 < alpha) {
			continue;
		}

		UndoInfo ui = board.makeMove(move);

		BestMove result = quiescenceSearch(board, -beta, -alpha, searchValues, qDepth + 1);
		int score = -result.score;

		board.unmakeMove(move, ui);

		if (score > bestScore) {
			bestScore = score;
		}

		if (score > alpha) {
			alpha = score;
		}

		if (alpha >= beta) {
			break; // Beta cutoff
		}
	}
	return {0, bestScore, 1, qDepth, true, {}};
}