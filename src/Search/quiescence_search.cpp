//
// Created by ECanDo on 2025-12-09.
//

#include "quiescence_search.h"

#include <cassert>

#include "search.h"

BestMove quiescenceSearch(Board &board, int alpha, const int beta, SearchValues &searchValues, const int qDepth) {
	searchValues.nodes++;
	constexpr int MAX_Q_DEPTH = 32;
	assert(alpha >= -INF_SCORE && alpha < beta && beta <= INF_SCORE);

	// TT Probe
	// TTEntry ttEntry;
	// if (globalTT.probe(board.zobristHash, 0, alpha, beta, ttEntry)) {
	// 	return {ttEntry.bestMove, ttEntry.score, 1, qDepth, true, {}};
	// }

	// int alphaOrig = alpha;

	// If we do nothing, what's the score???
	const int standPat = evaluateBoardNNUE(board);

	if (standPat >= beta) {
		// globalTT.store(board.zobristHash, 0, 0, beta, TT_BETA);
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
		// Don't store TT, since there are still quite moves - need to implement draw/checkmates
		return {0, standPat, 1, qDepth, true, {}};
	}

	std::array<int, MoveLimit> moveScores{};
	for (int i = 0; i < captures.length(); i++) {
		moveScores[i] = scoreMoveForOrdering(captures.get(i), board, 0, nullptr, nullptr);
	}

	int bestScore = standPat;
	// Move bestMove = 0;

	for (int i = 0; i < captures.length(); i++) {
		selectNextBestMove(captures, moveScores, i, static_cast<int>(captures.length()));
		const Move move = captures.get(i);

		// // 1. SEE pruning - skip losing captures
		if (see(move, board) < 0) continue;

		// 2. Delta pruning - skip if even a winning capture can't raise alpha
		const int captured = abs(getPieceValue(board.pieceAtSquare(getMoveTo(move))));
		if (standPat + captured + 200 < alpha) continue;

		//
		UndoInfo ui = board.makeMove(move);
		const BestMove result = quiescenceSearch(board, -beta, -alpha, searchValues, qDepth + 1);
		const int score = -result.score;
		board.unmakeMove(move, ui);

		if (score > bestScore) {
			bestScore = score;
			// bestMove = move;
		}
		if (score > alpha) alpha = score;
		if (alpha >= beta) break; // Beta cutoff
	}
	// TT Store
	// TTFlag flag;
	// if (bestScore <= alphaOrig) flag = TT_ALPHA;
	// else if (bestScore >= beta) flag = TT_BETA;
	// else flag = TT_EXACT;
	//
	// globalTT.store(board.zobristHash, bestMove, 0, bestScore, flag);
	return {0, bestScore, 1, qDepth, true, {}};
}
