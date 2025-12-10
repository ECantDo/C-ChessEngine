//
// Created by ECanDo on 2025-12-09.
//

#include "quiescence_search.h"
#include "search.h"

BestMove quiescenceSearch(Board &board, int alpha, int beta) {
    // If we do nothing, what's the score???
    int standPat = evaluateBoard(board);

    if (standPat >= beta) {
        return {0, beta, 1, 0, true, {}}; // Beta cutoff
    }

    if (standPat > alpha) {
        alpha = standPat;
    }

    std::vector<Move> captures;
    generateLegalMoves(board, captures, true);

    if (captures.empty()) {
        return {0, standPat, 1, 0, true, {}};
    }

//    orderMoves(captures, board, 0);

    int bestScore = standPat;

    unsigned long long nodes = 1;

    for (Move move: captures) {
//        std::cout << "CAPTURE! " << moveToString(move) << std::endl << std::flush;
        UndoInfo ui = board.makeMove(move);

        BestMove result = quiescenceSearch(board, -beta, -alpha);
        int score = -result.score;
        nodes += result.nodes;

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
    return {0, bestScore, nodes, 0, true, {}};
}