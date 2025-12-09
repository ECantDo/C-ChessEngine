//
// Created by ECanDo on 2025-12-06.
//

#include "search.h"

static bool stopSearch = false;

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

BestMove alphaBeta(Board &board, int depth, int maxDepth, int alpha, int beta, Move previousBest,
                   std::vector<uint64_t> &searchPath) {

    // ============ Check for Draw ============
    // 50 move, and repetition
    if (depth > 0) {
        if (board.isDraw() || board.isRepetitionInSearch(searchPath)) {
            return {0, 0, 1, {}};  /* Draw score = 0 */
        }
    }

    // ============ TT Storage consts ============

    int alphaOrig = alpha; // For the TT
    int betaOrig = beta;

    // ============ TT Probe ============
    TTEntry ttEntry;
    // The depth is how many nodes from here it has been searched
    if (globalTT.probe(board.zobristHash, maxDepth - depth, alpha, beta, ttEntry)) {
        return {ttEntry.bestMove, ttEntry.score, 1, maxDepth, {ttEntry.bestMove}};
    }

    // ============ Generate Moves ============
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList);

    // ============ Legal moves is empty; check/draw ============
    if (moveList.empty()) {
        // King in check -> Mate
        if (isKingInCheck(board, board.turn)) {
            int mateScore = -MATE_SCORE + depth;
            globalTT.store(board.zobristHash, 0, maxDepth - depth, mateScore, TT_EXACT);
            return {0, mateScore, 1, depth, {0}};
        }
        // King not in check -> Draw
        globalTT.store(board.zobristHash, 0, maxDepth - depth, 0, TT_EXACT);
        return {0, 0, 1, depth, {0}};
    }

    // ============ Order Moves ============
    orderMoves(moveList, board, ttEntry.bestMove);
    Move bestMove = moveList[0];

    // ============ Time Check ============
    if (!stopSearch && g_timeLimitMS > 0) {
        auto now = std::chrono::steady_clock::now();
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_searchStart).count();
        if (elapsed >= g_timeLimitMS) {
            stopSearch = true;
        }
    }

    // ============ Exceeded parameters ============
    if (depth >= maxDepth || stopSearch) {
        return {bestMove, evaluate(board), 1, depth, {bestMove}};
    }

    // Now add to search path
    searchPath.push_back(board.zobristHash);

    int bestScore = -INF_SCORE;
    std::vector<Move> pv;

    unsigned long long nodes = 1;

    for (Move m: moveList) {
        UndoInfo undo = board.makeMove(m);

        BestMove result = alphaBeta(board, depth + 1, maxDepth, -beta, -alpha, 0, searchPath);
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

    // ==== STORE TT MOVE ====
    TTFlag flag;
    if (bestScore <= alphaOrig) {
        flag = TT_ALPHA;
    } else if (bestScore >= betaOrig) {
        flag = TT_BETA;
    } else {
        flag = TT_EXACT;
    }

    globalTT.store(board.zobristHash, bestMove, maxDepth - depth, bestScore, flag);

    searchPath.pop_back();
    return {bestMove, bestScore, nodes, depth, pv};
}

BestMove iterativeDeepening(Board &board, int maxDepth) {
    Move bestMove = 0;
    int bestScore = 0;
    unsigned long long totalNodes = 0;
    int depth;

    auto startTime = std::chrono::steady_clock::now();


    for (depth = 1; depth <= maxDepth; depth++) {
        std::vector<uint64_t> searchPath;

        BestMove result = alphaBeta(board, 0, depth, -INF_SCORE, INF_SCORE,
                                    bestMove, searchPath);

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
            int mateMoves = (depth + 1) >> 1;

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
    }

    return {bestMove, bestScore, totalNodes, depth};
}

BestMove selectMove(Board &board, int maxDepth, long timeLimitMS) {
    stopSearch = false;
    g_timeLimitMS = timeLimitMS;
    g_searchStart = std::chrono::steady_clock::now();

    return iterativeDeepening(board, maxDepth);
}

int evaluate(Board &board) {
    // TODO: Pawn structure

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
    int sq = isWhite ? flipIndex(square) : square; // Seems backwards, but is fine

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

// =====================================================================================================================
// DEBUGGING FUNCTIONS
// =====================================================================================================================
void rootDebugAlphaBeta(const Board &board, int maxDepth) {
    std::vector<Move> moveList;
    generateLegalMoves(const_cast<Board &>(board), moveList);
    if (moveList.empty()) {
        std::cout << "No legal moves at root\n";
        return;
    }

    struct Row {
        Move m;
        int score;
        std::vector<Move> pv;
    };
    std::vector<Row> rows;

    int alpha = -INF_SCORE;
    int beta = INF_SCORE;

    for (Move m: moveList) {
        std::vector<uint64_t> searchPath;
        Board b = board;                    // make local copy to be safe
        UndoInfo ui = b.makeMove(m);
        BestMove res = alphaBeta(b, 1, maxDepth, -beta, -alpha, 0, searchPath);
        int score = -res.score;
        rows.push_back({m, score, res.pv});
    }

    // sort by score descending for readability
    std::sort(rows.begin(), rows.end(), [](auto &a, auto &b) { return a.score > b.score; });

    std::cout << "=== root debug depth " << maxDepth << " ===\n";
    for (auto &r: rows) {
        std::cout << std::left << std::setw(8) << moveToString(r.m)
                  << " | score = " << std::setw(8) << r.score
                  << " | pv: ";
        for (auto &mm: r.pv) std::cout << moveToString(mm) << ' ';
        std::cout << '\n' << std::flush;
    }
    std::cout << "=== end root debug ===\n";
}