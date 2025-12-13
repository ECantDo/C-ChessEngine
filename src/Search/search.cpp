//
// Created by ECanDo on 2025-12-06.
//

#include "search.h"

bool useOpeningBook = true;
std::atomic<bool> stopSearch{false};

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
    // Add current board early to check for repetition
    searchPath.push_back(board.zobristHash);

    if (board.halfMoveClock >= 100) {
        return {0, 0, 1, 0, depth, true, {}};
    }
    if (board.isRepetitionInSearch(searchPath)) {
        return {0, 0, 1, 0, depth, true, {}};  /* Draw score = 0 */
    }

    // Draw on insufficient material (one of the)
    if ((board.whitePawns | board.blackPawns | board.whiteRooks | board.blackRooks |
         board.whiteBishops | board.blackBishops | board.whiteKnights | board.blackKnights |
         board.whiteQueens | board.blackQueens) == 0) {
        return {0, 0, 1, 0, depth, true, {}};  /* Draw score = 0 */

    }



    // ============ TT Storage consts ============

    int alphaOrig = alpha; // For the TT
    int betaOrig = beta;

    // ============ TT Probe ============
    TTEntry ttEntry;
    // The depth is how many nodes from here it has been searched
    if (globalTT.probe(board.zobristHash, maxDepth - depth, alpha, beta, ttEntry)) {
        int score = ttEntry.score;

//        // Adjust mate scores relative to current position
        if (score >= MATE_SCORE - 100) {
            // We're delivering mate - subtract depth to make it closer
            score -= depth;
        } else if (score <= -MATE_SCORE + 100) {
            // We're being mated - add depth to make it further away
            score += depth;
        }

        return {ttEntry.bestMove, score, 0, 1, maxDepth, true, {ttEntry.bestMove}};
    }

    // ============ Generate Moves ============
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList, false);

    // ============ Legal moves is empty; check/draw ============
    if (moveList.empty()) {
        // King in check -> Mate
        if (isKingInCheck(board, board.turn)) {
            int mateScore = -MATE_SCORE + depth;
            /* Only seeing this move, or a from-here depth of 1
            */
            globalTT.store(board.zobristHash, 0, 1, -MATE_SCORE, TT_EXACT);
            return {0, mateScore, 1, 0, depth, true, {}};
        }
        // King not in check -> Draw
        // Only seeing this move, or depth of 1
        globalTT.store(board.zobristHash, 0, 1, 0, TT_EXACT);
        return {0, 0, 1, 0, depth, true, {}};
    }

    // ============ Order Moves ============
    orderMoves(moveList, board, ttEntry.bestMove);
    Move bestMove = moveList[0];

    // ============ Time Check ============
    // Don't need it since it's in the main thread now
//    if (!stopSearch && g_timeLimitMS > 0) {
//        auto now = std::chrono::steady_clock::now();
//        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_searchStart).count();
//        if (elapsed >= g_timeLimitMS) {
//            stopSearch = true;
//        }
//    }

    // ============ Exceeded parameters ============
    if (depth >= maxDepth) {
        searchPath.pop_back();

        return quiescenceSearch(board, alpha, beta);
    }
//    if (stopSearch) {
//        return {0, 0, 1, depth, false, {bestMove}};
//    }

    int bestScore = -INF_SCORE;
    std::vector<Move> pv;

    unsigned long long nodes = 1;
    unsigned long long tbHits = 0;
    bool completed = true;

    for (Move m: moveList) {
        UndoInfo undo = board.makeMove(m);

        BestMove result = alphaBeta(board, depth + 1, maxDepth, -beta, -alpha,
                                    0, searchPath);
        int score = -result.score;
        nodes += result.nodes;
        tbHits += result.tbHits;

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

        if (stopSearch) {
            completed = false;
            break;
        }
    }

    searchPath.pop_back();
    if (completed) {
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

        return {bestMove, bestScore, nodes, tbHits, depth, true, pv};
    } else {
        return {bestMove, bestScore, nodes, tbHits, depth, false, {}};
    }
}

BestMove iterativeDeepening(Board &board, int maxDepth) {
    Move bestMove = 0;
    int bestScore = 0;
    unsigned long long totalNodes = 0;
    unsigned long long totalTbHits = 0;
    int depth;

    auto startTime = std::chrono::steady_clock::now();
    std::vector<Move> pv;


    for (depth = 1; depth <= maxDepth; depth++) {
        int alpha, beta;

        // TUNING FOR SINGLE TRY
        // 50 -> -30 ELO; 100 -> +7 ELO; 75 -> -31 ELO; 125 -> +5 ELO; 250 -> +9 ELO

        // TUNING FOR 4 ATTEMPTS
        // 250 -> +12.2 ELO; 100 -> -83.2 ELO; 150 -> -31 ELO; 225 -> -43.7 ELO; 275 -> -28 ELO
        int window = 250;

        // Asperation window: It is better, but only barely
//        if (depth <= 3) {
//            alpha = -INF_SCORE;
//            beta = INF_SCORE;
//        } else {
//            alpha = bestScore - window;
//            beta = bestScore + window;
//        }
        alpha = -INF_SCORE;
        beta = INF_SCORE;

        std::vector<uint64_t> searchPath;
        searchPath.reserve(32); // Reserve a depth of 32 moves
        BestMove result;
        bool validResult = false;

//        for (int attempt = 0; attempt < 4; attempt++) {
        result = alphaBeta(board, 0, depth, alpha, beta,
                           bestMove, searchPath);


//            if (result.score > alpha && result.score < beta) {
//                validResult = true;
//                break; // Score is within the window
//            }
//
//            // Failed - widen window
//            if (result.score <= alpha) {
//                alpha -= window * (1 << attempt);  // Widen by 50, 100, 200, 400...
//            }
//            if (result.score >= beta) {
//                beta += window * (1 << attempt);
//            }
//
//            // Last attempt - use full window
//            if (attempt == 3) {
//                alpha = -INF_SCORE;
//                beta = INF_SCORE;
//            }
//        }


        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        if (!result.completed) {
            // Throw out partial-computations ; (really not good), need to look into more
            break;
        }

        bestMove = result.bestMove;
        bestScore = result.score;
        totalNodes += result.nodes;
        totalTbHits += result.tbHits;

        pv = result.pv;


        std::string score;
        bool isMate = false; // Check for early exit, if mate is found at some depth, it is the first mate; take it
        if (abs(bestScore) >= MATE_SCORE - 100) { // I doubt it can find a forced mate in 50
            //TODO: Re-enable when quiescence search is implemented -- Horizon effect (I think)
            // Overall, likely needs more debugging...
//            isMate = true;
            int mateDistance = MATE_SCORE - abs(bestScore);
            int mateMoves = (mateDistance + 1) / 2;

//            std::cerr << "DEBUG: depth=" << depth
//                      << " bestScore=" << bestScore
//                      << " mateDistance=" << mateDistance
//                      << " mateMoves=" << mateMoves << std::endl << std::flush;

            if (bestScore > 0)
                score = std::format(" score mate {}", mateMoves);
            else
                score = std::format(" score mate -{}", mateMoves);

        } else {
            score = std::format(" score cp {}", bestScore);
        }

        /* UCI info output */
        std::cout << "info "
                  << score
                  << " depth " << depth
                  << " tbhits " << totalTbHits
                  << " nodes " << result.nodes
                  << " time " << elapsed
                  << " nps " << (elapsed > 0 ? (result.nodes * 1000 / elapsed) : 0)
                  << " pv ";

        for (Move &m: result.pv) {
            std::cout << moveToString(m) << ' ';
        }
        std::cout << std::endl << std::flush;

        if (stopSearch || isMate) {
            break;
        }
    }

    // Subtract 1 from depth because it's off by one
    return {bestMove, bestScore, totalNodes, totalTbHits, depth - 1, true, pv};
}

BestMove selectMove(Board &board, int maxDepth, long timeLimitMS, int numThreads) {
    stopSearch = false;
    g_timeLimitMS = timeLimitMS;
    g_searchStart = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    std::vector<ThreadResult> results(numThreads);

    // Launch all threads
    for (int i = 0; i < numThreads; i++) {
//        std::cerr << "Launching thread " << i << std::endl;

        threads.emplace_back([&results, board, maxDepth, i, numThreads]() {
            results[i] = searchThread(board, maxDepth, i, numThreads);
        });
    }
    // All threads are running
//    std::cerr << "All threads launched, monitoring time... (" << g_timeLimitMS << " ms)" << std::endl;

    // Main thread monitors time ONLY if there's a time limit
    if (g_timeLimitMS > 0) {
        while (!stopSearch) {
            auto now = std::chrono::steady_clock::now();
            unsigned long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - g_searchStart).count();

            if (elapsed >= g_timeLimitMS) {
                stopSearch = true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    // If no time limit, threads will search to maxDepth and stop naturally

//    std::cerr << "Setting stopSearch and joining threads..." << std::endl;
//    stopSearch = true;
    for (auto &thread: threads) {
        thread.join();
    }
//    std::cerr << "All threads joined, collecting results..." << std::endl;

    // All threads done - get the best result
    ThreadResult best = results[0];
    unsigned long long totalNodes = results[0].nodes;
    for (int i = 1; i < numThreads; i++) {
        // Prefer deeper search, or more nodes at same depth
        if (results[i].depth > best.depth ||
            (results[i].depth == best.depth && results[i].nodes > best.nodes)) {
            best = results[i];
        }
        totalNodes += results[i].nodes;
    }

    // TODO: Add a TB hit counter
    return {best.bestMove, best.bestScore, totalNodes, 0, best.depth, true, best.pv};
}

std::mutex g_outputMutex;  // Global

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads) {
    Move bestMove = 0;
    int bestScore = 0;
    std::vector<Move> pv;
    unsigned long long totalNodes = 0;
    int completedDepth = 0;

    int startDepth = 1 + (threadId % std::min(8, totalThreads));
    auto startTime = std::chrono::steady_clock::now();

    for (int depth = startDepth; depth <= maxDepth; depth++) {
        if (stopSearch) break;

        std::vector<uint64_t> searchPath;
        searchPath.reserve(30);

        BestMove result = alphaBeta(board, 0, depth, -INF_SCORE, INF_SCORE,
                                    bestMove, searchPath);

        if (!result.completed || stopSearch) break;

        bestMove = result.bestMove;
        bestScore = result.score;
        pv = result.pv;
        totalNodes += result.nodes;
        completedDepth = depth;

        // Print UCI info
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        {
            std::lock_guard<std::mutex> lock(g_outputMutex);

            std::string score;
            if (abs(bestScore) >= MATE_SCORE - 100) {
                int mateDistance = MATE_SCORE - abs(bestScore);
                int mateMoves = (mateDistance + 1) / 2;
                score = (bestScore > 0)
                        ? std::format(" score mate {}", mateMoves)
                        : std::format(" score mate -{}", mateMoves);
            } else {
                score = std::format(" score cp {}", bestScore);
            }

            std::cout << "info "
                      << score
                      << " depth " << depth
                      << " nodes " << result.nodes
                      << " time " << elapsed
                      << " nps " << (elapsed > 0 ? (result.nodes * 1000 / elapsed) : 0)
                      << " pv ";

            for (Move &m : pv) {
                std::cout << moveToString(m) << ' ';
            }
            std::cout << std::endl << std::flush;
        }
    }

    return {bestMove, bestScore, completedDepth, pv, totalNodes};
}


bool isKingInCheck(const Board &board, int color) {
    uint64_t king = color == 1 ? board.whiteKing : board.blackKing;
    return isSquareAttacked(board, std::countr_zero(king), -color);
}

/* Helper to get piece-square table value */

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