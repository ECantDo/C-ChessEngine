//
// Created by ECanDo on 2025-12-09.
//

#include "quiescence_search.h"
#include "search.h"

BestMove quiescenceSearch(Board &board, int alpha, const int beta, SearchValues &searchValues, const int qDepth) {
	searchValues.nodes++;
	constexpr int MAX_Q_DEPTH = 32;

	// If we do nothing, what's the score???
	const int standPat = evaluateBoardNNUE(board);

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
	generateMoves(board, captures, true, true);

	if (captures.empty()) {
		return {0, standPat, 1, qDepth, true, {}};
	}

	//    orderMoves(captures, board, 0);

	int bestScore = standPat;

	for (int i = 0; i < captures.length(); i++) {
		const Move move = captures.get(i);
		//        std::cout << "CAPTURE! " << moveToString(move) << std::endl << std::flush;
		const int captured = abs(getPieceValue(board.pieceAtSquare(getMoveTo(move))));
		if (standPat + captured + 200 < alpha) {
			continue;
		}

		UndoInfo ui = board.makeMove(move);

		const BestMove result = quiescenceSearch(board, -beta, -alpha, searchValues, qDepth + 1);
		const int score = -result.score;

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
