//
// Created by ECanDo on 2025-12-06.
//


#include <chrono>
#include <atomic>
#include "search.h"

extern std::atomic<bool> stopSearch;

static long g_timeLimitMS = 0;
static std::chrono::steady_clock::time_point g_searchStart;

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
            return {0, -MATE_SCORE + depth, 1, depth};
        }
        // King not in check -> Draw
        return {0, 0, 1, depth};
    }


    // --- TIME CHECK ----------------------------------------------------
    if (!stopSearch && g_timeLimitMS > 0) {
        auto now = std::chrono::steady_clock::now();
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_searchStart).count();
        if (elapsed >= g_timeLimitMS) {
            stopSearch = true;
        }
    }
    if (depth >= maxDepth || stopSearch) {
        return {0, evaluate(board), 1, depth - 1};
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

    return {bestMove, bestScore, nodes, depth, pv};

}

BestMove iterativeDeepening(Board &board, int maxDepth) {
    Move bestMove = 0;
    int bestScore = 0;
    unsigned long long totalNodes = 0;
    int depth;

    for (depth = 1; depth <= maxDepth; depth++) {
        auto startTime = std::chrono::steady_clock::now();

        BestMove result = alphaBeta(board, 0, depth,
                                    INT32_MIN + 10000, INT32_MAX - 10000, bestMove);

        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        bestMove = result.bestMove;
        bestScore = result.score;
        totalNodes += result.nodes;


        std::string score;
        bool isMate = false; // Check for early exit, if mate is found at some depth, it is the first mate; take it
        if (abs(bestScore) >= MATE_SCORE - 1000) {
            // it's a mate score
            isMate = true;
            int matePly = MATE_SCORE - abs(bestScore);
            int mateMoves = (matePly + 1) / 2;

            // negative means you're being mated
            if (bestScore > 0)
                score = std::format(" score mate {}", mateMoves);
            else
                score = std::format(" score mate -{}", mateMoves);

        } else {
            score = std::format(" score cp {}", bestScore);
        }

        /* UCI info output */
        std::cout << "info depth " << depth
                  << score
                  << " nodes " << result.nodes
                  << " time " << elapsed
                  << " nps " << (elapsed > 0 ? (result.nodes * 1000 / elapsed) : 0)
                  << " pv ";

        for (Move &m: result.pv) {
            std::cout << moveToString(m) << ' ';
        }
        std::cout << std::endl << std::flush;

        /* Check if we should stop (time management later) */
        if (isMate || stopSearch) {
            break;
        }
        // if (elapsed > timeLimitMS) break;

    }

    // TODO: add PV
    return {bestMove, bestScore, totalNodes, depth};
}

BestMove selectMove(Board &board, int maxDepth, long timeLimitMS) {
    std::cout << "Found limit to be " << timeLimitMS << '\n';

    stopSearch = false;
    g_timeLimitMS = timeLimitMS;
    g_searchStart = std::chrono::steady_clock::now();

    return iterativeDeepening(board, maxDepth);
}

int evaluate(Board &board) {
    int score = 0;

    for (char piece: ALL_PIECES) {
        uint64_t bitboard = board.getBitboard(piece);
        // Sum piece values
        score += std::popcount(bitboard) * getPieceValue(piece);

        // Piece square table values
        bool isWhite = isupper(piece);
        while (bitboard) {
            int sq = std::countr_zero(bitboard);
            bitboard &= bitboard - 1;

            if (isWhite) { // Add white score
                score += getPieceSquareValue(piece, sq);
            } else { // Subtract black score
                score -= getPieceSquareValue(piece, sq);
            }
        }
    }

    // Return from current player's perspective
    return board.turn == 1 ? score : -score;
}

bool isKingInCheck(const Board &board, int color) {
    uint64_t king = color == 1 ? board.whiteKing : board.blackKing;
    return isSquareAttacked(board, std::countr_zero(king), -color);
}

/* Helper to get piece-square table value */
int getPieceSquareValue(char piece, int square) {
    /* For black pieces, flip the square vertically */
    bool isWhite = isupper(piece);
    int sq = isWhite ? square : (63 - square);

    switch (tolower(piece)) {
        case 'p':
            return pawnTable[sq];
        case 'n':
            return knightTable[sq];
        case 'b':
            return bishopTable[sq];
        case 'r':
            return rookTable[sq];
        case 'q':
            return queenTable[sq];
        case 'k':
            return kingMiddleGameTable[sq];
        default:
            return 0;
    }
}