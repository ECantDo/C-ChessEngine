//
// Created by ECanDo on 2025-12-06.
//


#include "search.h"

BestMove selectMove(Board &board, int depth, int maxDepth) {
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList);

    if (moveList.empty()) {
        // In check or stalemate.

        // King in check -> Mate
        if (isKingInCheck(board, board.turn)) {
            return {0, -100000 + depth, 1};
        }
        // King not in check -> Draw
        return {0, 0, 1};
    }


    Move bestMove = moveList[0];
    int bestMoveScore = INT32_MIN;
    unsigned long long nodes = 1;

    for (Move m: moveList) {
        UndoInfo undo = board.makeMove(m);

        int score;
        if (depth >= maxDepth) {
            // Max depth; evaluate
            score = -evaluate(board);
        } else {
            BestMove result = selectMove(board, depth + 1, maxDepth);
            score = -result.score; // Negate for switching sides
            nodes += result.nodes;
        }

        board.unmakeMove(m, undo);

        if (score > bestMoveScore) {
            bestMoveScore = score;
            bestMove = m;
        }
    }

    return {bestMove, bestMoveScore, nodes};
}

int evaluate(Board &board) {

    int score = 0;

    // Sum piece values
    score += std::popcount(board.whitePawns) * 100;
    score += std::popcount(board.whiteKnights) * 300;
    score += std::popcount(board.whiteBishops) * 330;
    score += std::popcount(board.whiteRooks) * 500;
    score += std::popcount(board.whiteQueens) * 950;

    score -= std::popcount(board.blackPawns) * 100;
    score -= std::popcount(board.blackKnights) * 300;
    score -= std::popcount(board.blackBishops) * 330;
    score -= std::popcount(board.blackRooks) * 500;
    score -= std::popcount(board.blackQueens) * 950;

    // Return from current player's perspective
    return board.turn == 1 ? score : -score;
}

bool isKingInCheck(const Board &board, int color) {
    uint64_t king = color == 1 ? board.whiteKing : board.blackKing;
    return isSquareAttacked(board, std::countr_zero(king), -color);
}