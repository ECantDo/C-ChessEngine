//
// Created by ECanDo on 2025-12-09.
//

#include "quiescence_search.h"

#include <cassert>

#include "search.h"

constexpr int MAX_Q_DEPTH = 32;


BestMove quiescenceSearch(Board &board, int alpha, const int beta, SearchValues &searchValues, const int qDepth) {
	searchValues.nodes++;
	assert(alpha >= -INF_SCORE && alpha < beta && beta <= INF_SCORE);

	// Check for a draw
	if (board.insufficientMaterial()) {
		return {0, 0, 1, qDepth, true, {}};
	}

	const bool inCheck = board.isKingInCheck();

	// TT Probe
	// TTEntry ttEntry;
	// if (globalTT.probe(board.zobristHash, 0, alpha, beta, ttEntry)) {
	// 	return {ttEntry.bestMove, ttEntry.score, 1, qDepth, true, {}};
	// }

	// int alphaOrig = alpha;

	// If we do nothing, what's the score???
	Score standPat;
	if (inCheck) {
		standPat = -MATE_SCORE + qDepth;
	} else {
		standPat = evaluateBoardNNUE(board);

		if (standPat >= beta) {
			// globalTT.store(board.zobristHash, 0, 0, beta, TT_BETA);
			return {0, standPat, 1, qDepth, true, {}}; // Beta cutoff
		}

		if (standPat > alpha) {
			alpha = standPat;
		}
	}


	// Stop quiescence if too deep
	// if (qDepth >= MAX_Q_DEPTH) {
	// 	return {0, standPat, qDepth, qDepth, true, {}};
	// }

	MoveList moveList;
	generateMoves(board, moveList, true, !inCheck);

	if (moveList.empty()) {
		// Don't store TT, since there are still quite moves - need to implement draw/checkmates
		return {0, standPat, 1, qDepth, true, {}};
	}

	std::array<int, MoveLimit> moveScores{};
	for (int i = 0; i < moveList.length(); i++) {
		moveScores[i] = scoreMoveForOrdering(moveList.get(i), board, 0, nullptr, nullptr);
	}

	Score bestScore = standPat;
	// Move bestMove = 0;

	for (int i = 0; i < moveList.length(); i++) {
		selectNextBestMove(moveList, moveScores, i, static_cast<int>(moveList.length()));
		const Move move = moveList.get(i);

		if (!inCheck) {
			// // 1. SEE pruning - skip losing captures
			if (see(move, board) < 0) continue;

			// 2. Delta pruning - skip if even a winning capture can't raise alpha
			const int captured = abs(getPieceValue(board.pieceAtSquare(getMoveTo(move))));
			if (standPat + captured + 200 < alpha) continue;
		}

		//
		UndoInfo ui = board.makeMove(move);
		const BestMove result = quiescenceSearch(board, -beta, -alpha, searchValues, qDepth + 1);
		const Score score = -result.score;
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
