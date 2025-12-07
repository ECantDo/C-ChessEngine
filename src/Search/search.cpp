//
// Created by ECanDo on 2025-12-06.
//


#include <chrono>
#include "search.h"

void orderMoves(std::vector<Move> &moves, const Board &board, Move previousBest) {
    std::sort(moves.begin(), moves.end(), [&board, previousBest](Move a, Move b) {
        /* Previous best move searched first */
        if (a == previousBest) return true;
        if (b == previousBest) return false;

        /* Then order by capture/promotion value */
        return scoreMoveForOrdering(a, board) > scoreMoveForOrdering(b, board);
    });
}

int scoreMoveForOrdering(Move m, const Board &board) {
    int flags = getMoveFlags(m);
    int score = 0;

    if (flags & MOVE_FLAG_CAPTURE) {
        int to = getMoveTo(m);
        int from = getMoveFrom(m);

        char victim = board.pieceAtSquare(to);
        char attacker = board.pieceAtSquare(from);

        score = 10000 + getPieceValue(victim) * 10 - getPieceValue(attacker);
    }

    if (flags & MOVE_FLAG_PROMOTION) {
        int promoType = flags & 0x3;
        switch (promoType) {
            case PROMOTE_TO_QUEEN:
                score += 9500;
                break;
            case PROMOTE_TO_ROOK:
                score += 5000;
                break;
            case PROMOTE_TO_BISHOP:
                score += 3300;
                break;
            case PROMOTE_TO_KNIGHT:
                score += 3000;
                break;
            default:
                break;
        }
    }

    // Castling is good
    if (flags & MOVE_FLAG_CASTLING) {
        score += 1000;
    }

    return score;
}

BestMove alphaBeta(Board &board, int depth, int maxDepth, int alpha, int beta, Move previousBest) {
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList);

    if (moveList.empty()) {
        // King in check -> Mate
        if (isKingInCheck(board, board.turn)) {
            return {0, -100000 + depth, 1};
        }
        // King not in check -> Draw
        return {0, 0, 1};
    }

    if (depth >= maxDepth) {
        return {0, evaluate(board), 1};
    }

    // Order moves for better pruning
    orderMoves(moveList, board, previousBest);

    Move bestMove = moveList[0];
    int bestScore = INT32_MIN;
    std::vector<Move> pv;

    unsigned long long nodes = 1;

    for (Move m: moveList) {
        UndoInfo undo = board.makeMove(m);

        BestMove result = alphaBeta(board, depth + 1, maxDepth, -beta, -alpha, 0);
        int score = -result.score;
        nodes += result.nodes;

        board.unmakeMove(m, undo);

        if (score > bestScore) {
            bestMove = m;
            bestScore = score;

            pv.clear();
            pv.push_back(m);
            pv.insert(pv.end(), result.pv.begin(), result.pv.end());
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            break;
        }
    }

    return {bestMove, bestScore, nodes, pv};

}

BestMove iterativeDeepening(Board &board, int maxDepth) {
    Move bestMove = 0;
    int bestScore = 0;
    unsigned long long totalNodes = 0;

    for (int depth = 1; depth <= maxDepth; depth++) {
        auto startTime = std::chrono::steady_clock::now();

        BestMove result = alphaBeta(board, 0, depth,
                                    INT32_MIN + 10000, INT32_MAX - 10000, bestMove);

        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        bestMove = result.bestMove;
        bestScore = -result.score;
        totalNodes += result.nodes;

        /* UCI info output */
        std::cout << "info depth " << depth
                  << " score cp " << bestScore
                  << " nodes " << result.nodes
                  << " time " << elapsed
                  << " nps " << (elapsed > 0 ? (result.nodes * 1000 / elapsed) : 0)
                  << " pv ";

        for (Move &m: result.pv) {
            std::cout << moveToString(m) << ' ';
        }
        std::cout << std::endl << std::flush;

        /* Check if we should stop (time management later) */
        // if (elapsed > timeLimitMS) break;

    }

    return {bestMove, bestScore, totalNodes};
}

BestMove selectMove(Board &board, int maxDepth) {
    return iterativeDeepening(board, maxDepth);
}

int evaluate(Board &board) {

    int score = 0;

    // Sum piece values
    score += std::popcount(board.whitePawns) * getPieceValue('P');
    score += std::popcount(board.whiteKnights) * getPieceValue('N');
    score += std::popcount(board.whiteBishops) * getPieceValue('B');
    score += std::popcount(board.whiteRooks) * getPieceValue('R');
    score += std::popcount(board.whiteQueens) * getPieceValue('Q');

    score += std::popcount(board.blackPawns) * getPieceValue('p');
    score += std::popcount(board.blackKnights) * getPieceValue('n');
    score += std::popcount(board.blackBishops) * getPieceValue('b');
    score += std::popcount(board.blackRooks) * getPieceValue('r');
    score += std::popcount(board.blackQueens) * getPieceValue('q');

    // Return from current player's perspective
    return board.turn == 1 ? score : -score;
}

bool isKingInCheck(const Board &board, int color) {
    uint64_t king = color == 1 ? board.whiteKing : board.blackKing;
    return isSquareAttacked(board, std::countr_zero(king), -color);
}