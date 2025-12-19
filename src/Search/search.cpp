//
// Created by ECanDo on 2025-12-06.
//

#include "search.h"

bool useOpeningBook = true;
std::atomic<bool> stopSearch{false};

static long g_timeLimitMS = 0;
static std::chrono::steady_clock::time_point g_searchStart;

int scoreMoveForOrdering(Move m, const Board &board, int ply,
                         Move killers[MAX_PLY][2], unsigned long long history[2][64][64]) {
    int flags = getMoveFlags(m);

    int to = getMoveTo(m);
    int from = getMoveFrom(m);

    // 1. CAPTURES (highest priority)
    if (flags & MOVE_FLAG_CAPTURE) {

        Piece victim = board.pieceAtSquare(to);
        Piece attacker = board.pieceAtSquare(from);
        return 1000000 + getPieceValue(victim) * 10 - getPieceValue(attacker);
    }

    // 2. PROMOTIONS
    if (flags & MOVE_FLAG_PROMOTION) {
        int promoType = flags & 0x3;
        switch (promoType) {
            case PROMOTE_TO_QUEEN:
                return 900000;
            case PROMOTE_TO_ROOK:
                return 500000;
            case PROMOTE_TO_BISHOP:
                return 330000;
            case PROMOTE_TO_KNIGHT:
                return 300000;
            default:
                break;
        }
    }

    // 3. KILLER MOVES (non-captures that caused cutoffs at this plys)
    if (ply < MAX_PLY) {
        if (m == killers[ply][0]) return 90000;
        if (m == killers[ply][1]) return 80000;
    }

    // 4. CASTLING
    if (flags & MOVE_FLAG_CASTLING) return 10000;

    // 5. HISTORY (statistical goodness)
    int color = (board.turn == 1) ? 0 : 1;
    return history[color][from][to];
}

void orderMoves(std::vector<Move> &moves, const Board &board, Move previousBest, int ply,
                Move killers[MAX_PLY][2], unsigned long long history[2][64][64]) {
    std::sort(moves.begin(), moves.end(), [&board, previousBest, ply, killers, history](Move a, Move b) {
        /* Previous best move searched first */
        if (a == previousBest) return true;
        if (b == previousBest) return false;

        /* Then order by capture/promotion value */
        return scoreMoveForOrdering(a, board, ply, killers, history) >
               scoreMoveForOrdering(b, board, ply, killers, history);
    });
}

int calculateExtension(Board &board, int extensionsUsed) {
    if (extensionsUsed >= MAX_EXTENSIONS) {
        return 0;
    }
    int extension = 0;
    bool inCheck = isKingInCheck(board, board.turn);
    if (inCheck) {
        extension += 1;
    }

    return extension;
}

bool hasNonPawnMaterial(Board &board) {
    // Check if there is something other than pawns on the board
    if (board.turn == 1) {
        return 0 !=
               (board.whiteQueens |
                board.whiteRooks |
                board.whiteBishops |
                board.whiteKnights);
    } else {
        return 0 !=
               (board.blackQueens |
                board.blackRooks |
                board.blackBishops |
                board.blackKnights);
    }
}

BestMove alphaBeta(Board &board, int depth, int plys, int alpha, int beta, Move previousBest,
                   std::vector<uint64_t> &searchPath, Move killerMoves[MAX_PLY][2],
                   unsigned long long historyTable[2][64][64],
                   int extensionsUsed = 0, bool nullMoveAllowed = true) {



    // ============ Check for Draw ============
    // 50 move, and repetition
    // Add current board early to check for repetition
    searchPath.push_back(board.zobristHash);

    if (board.halfMoveClock >= 100) {
        searchPath.pop_back();
        return {0, 0, 1, 0, plys, true, {}};
    }
    if (board.isRepetitionInSearch(searchPath)) {
        searchPath.pop_back();
        return {0, 0, 1, 0, plys, true, {}};  /* Draw score = 0 */
    }

    // Draw on insufficient material (one of the)
    if ((board.whitePawns | board.blackPawns | board.whiteRooks | board.blackRooks |
         board.whiteBishops | board.blackBishops | board.whiteKnights | board.blackKnights |
         board.whiteQueens | board.blackQueens) == 0) {
        searchPath.pop_back();
        return {0, 0, 1, 0, plys, true, {}};  /* Draw score = 0 */

    }



    // ============ TT Storage consts ============

    int alphaOrig = alpha; // For the TT
    int betaOrig = beta;

    // ============ TT Probe ============
    TTEntry ttEntry;
    // The plys is how many nodes from here it has been searched
    if (globalTT.probe(board.zobristHash, depth, alpha, beta, ttEntry)) {
        int score = ttEntry.score;

//        // Adjust mate scores relative to current position
        if (score >= MATE_SCORE - 100) {
            // We're delivering mate - subtract plys to make it closer
            score -= plys;
        } else if (score <= -MATE_SCORE + 100) {
            // We're being mated - add plys to make it further away
            score += plys;
        }

        searchPath.pop_back();
        return {ttEntry.bestMove, score, 0, 1, plys, true, {ttEntry.bestMove}};
    }

    // ============ Generate Moves ============
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList, false);
    bool inCheck = isKingInCheck(board, board.turn);

    // ============ Legal moves is empty; check/draw ============
    if (moveList.empty()) {
        searchPath.pop_back();

        // King in check -> Mate
        if (inCheck) {
            int mateScore = -MATE_SCORE + plys;
            // Only seeing this move, or a from-here plys of 1

            globalTT.store(board.zobristHash, 0, depth, -MATE_SCORE, TT_EXACT);
            return {0, mateScore, 1, 0, plys, true, {}};
        }
        // King not in check -> Draw
        // Only seeing this move, or plys of 1
        globalTT.store(board.zobristHash, 0, depth, 0, TT_EXACT);
        return {0, 0, 1, 0, plys, true, {}};
    }

    // ============ Order Moves ============
    // Depth also happens to be the ply
    orderMoves(moveList, board, ttEntry.bestMove, plys, killerMoves, historyTable);
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
    if (depth <= 0) {
        searchPath.pop_back();

        return quiescenceSearch(board, alpha, beta);
    }

    // ============ Reverse Futility Pruning ============
    // How good is my static eval? Is it so far above beta that even if I make a bad move, I will still beat beta
    if (depth <= 3 &&
        !inCheck &&
        abs(beta) < MATE_SCORE - 100) {

        int staticEval = evaluateBoard(board);
        int margin;
        switch (depth) {
            case 1:
                margin = 200;
                break;
            case 2:
                margin = 400;
                break;
            case 3:
                margin = 600;
                break;
            default:
                margin = 1000;
        }

        if (staticEval - margin >= beta) {
            searchPath.pop_back();
            return {0, staticEval - margin, 1, 0, plys, true, {}};
        }
    }

    // ============ Null Move Pruning ============
    if (nullMoveAllowed && !inCheck && depth >= 3 && hasNonPawnMaterial(board)) {
        // Save values
        int8_t oldTurn = board.turn;
        int oldEnPass = board.enPassantSquare;
        uint64_t oldHash = board.zobristHash;

        // Set up to pretend to make a move
        board.enPassantSquare = -1;
        board.halfMoveClock++;
        board.turn = (int8_t) (-board.turn);
        board.zobristHash ^= Zobrist::sideToMove;
        if (oldEnPass != -1) {
            board.zobristHash ^= Zobrist::enPassantFile[oldEnPass];
        }

        int R = (depth >= 8 ? 3 : 2); // Reduction -- Basically skipping my move
        BestMove nullResult = alphaBeta(board, depth - 1 - R, plys + 1, -beta, -beta + 1,
                                        0, searchPath, killerMoves, historyTable,
                                        extensionsUsed, false);
        // Restore values
        board.turn = oldTurn;
        board.halfMoveClock--;
        board.enPassantSquare = oldEnPass;
        board.zobristHash = oldHash;

        if (-nullResult.score >= beta) {
            BestMove verify = alphaBeta(board, depth - 1, plys + 1, beta - 1, beta,
                                        0, searchPath, killerMoves, historyTable,
                                        extensionsUsed, false);

            if (verify.score >= beta) {
                searchPath.pop_back();
                return {0, beta, verify.nodes, verify.tbHits, plys, true, {}};
            }
        }
    }

    int extension = calculateExtension(board, extensionsUsed);


//    if (stopSearch) {
//        return {0, 0, 1, plys, false, {bestMove}};
//    }

    int bestScore = -INF_SCORE;
    std::vector<Move> pv;

    unsigned long long nodes = 1;
    unsigned long long tbHits = 0;
    bool completed = true;
    int movesSearched = 0;

    for (Move m: moveList) {
        UndoInfo undo = board.makeMove(m);

        // Since using genPseudoLegal(), only actually checking when it's for a move I have made
//        if (isKingInCheck(board, -board.turn)) {
//            board.unmakeMove(m, undo);
//            continue;
//        }

        BestMove result;

        // Get PV node
        if (movesSearched == 0) {
            result = alphaBeta(board, depth - 1, plys + 1, -beta, -alpha,
                               0, searchPath, killerMoves, historyTable,
                               extensionsUsed + extension, nullMoveAllowed);
        } else {
            // Later moves: try null window search first
            if (movesSearched >= 5 && plys >= 3 &&
                !(m & MOVE_FLAG_CAPTURE) &&
                !isKingInCheck(board, -board.turn) &&
                !isKingInCheck(board, board.turn)) {
                // LMR with null window
                int reduction = 1;
                if (movesSearched > 6 && plys > 6) {
                    reduction = 2;
                    if (movesSearched > moveList.size() >> 1) {
                        reduction += 2;
                    }
                }

                // Try reduced null window search
                result = alphaBeta(board, depth - 1 - reduction, plys + 1,
                                   -alpha - 1, -alpha,  // NULL WINDOW
                                   0, searchPath, killerMoves, historyTable,
                                   extensionsUsed + extension, nullMoveAllowed);

                // If it beat alpha, re-search at full depth
                if (-result.score > alpha && reduction > 0) {
                    result = alphaBeta(board, depth - 1, plys + 1,
                                       -alpha - 1, -alpha,  // Still null window
                                       0, searchPath, killerMoves, historyTable,
                                       extensionsUsed + extension, nullMoveAllowed);
                }

                // If STILL beat alpha, do full window search
                if (-result.score > alpha) {
                    result = alphaBeta(board, depth - 1, plys + 1,
                                       -beta, -alpha,  // FULL WINDOW
                                       0, searchPath, killerMoves, historyTable,
                                       extensionsUsed + extension, nullMoveAllowed);
                }
            } else {
                // Non-LMR moves: null window then full if needed
                result = alphaBeta(board, depth - 1, plys + 1,
                                   -alpha - 1, -alpha,  // NULL WINDOW
                                   0, searchPath, killerMoves, historyTable,
                                   extensionsUsed + extension, nullMoveAllowed);

                // Beat alpha? Re-search with full window
                if (-result.score > alpha /*&& -result.score < beta*/) {
                    result = alphaBeta(board, depth - 1, plys + 1,
                                       -beta, -alpha,  // FULL WINDOW
                                       0, searchPath, killerMoves, historyTable,
                                       extensionsUsed + extension, nullMoveAllowed);
                }
            }
        }

        int score = -result.score;
        nodes += result.nodes;
        tbHits += result.tbHits;

        board.unmakeMove(m, undo);
        movesSearched++;

        if (score > bestScore) {
            bestMove = m;
            bestScore = score;

            pv.clear();
            pv.push_back(m);
            if (!result.pv.empty() && result.pv[0] != 0) {
                pv.insert(pv.end(), result.pv.begin(), result.pv.end());

            }
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            // If quiet move (i.e. not a capture)
            if (!(m & MOVE_FLAG_CAPTURE)) {
                // Shift old killer to slot 1, new to slot 0
                killerMoves[plys][1] = killerMoves[plys][0];
                killerMoves[plys][0] = m;

                int color = board.turn == 1 ? 0 : 1;
                int from = getMoveFrom(m);
                int to = getMoveTo(m);
                historyTable[color][from][to] += depth * depth;
                if (historyTable[color][from][to] > 100000) {
                    // Age all history values
                    for (int c = 0; c < 2; c++) {
                        for (int f = 0; f < 64; f++) {
                            for (int t = 0; t < 64; t++) {
                                historyTable[c][f][t] >>= 1;  // Divide by 2
                            }
                        }
                    }
                }
            }
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

        int ttScore = bestScore;
        if (ttScore >= MATE_SCORE - 100){
            ttScore += plys;
        } else if (ttScore <= -MATE_SCORE + 100){
            ttScore -= plys;
        }

        globalTT.store(board.zobristHash, bestMove, depth, ttScore, flag);

        return {bestMove, bestScore, nodes, tbHits, plys, true, pv};
    } else {
        return {bestMove, bestScore, nodes, tbHits, plys, false, {}};
    }
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
        // Prefer deeper search, or more nodes at same plys
        if (results[i].depth > best.depth ||
            (results[i].depth == best.depth && results[i].nodes > best.nodes)) {
            best = results[i];
        }
        totalNodes += results[i].nodes;
    }

    return {best.bestMove, best.bestScore, totalNodes, best.tbHits, best.depth, true, best.pv};
}

std::mutex g_outputMutex;  // Global

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads) {
    Move bestMove = 0;
    int bestScore = 0;
    std::vector<Move> pv;
    unsigned long long totalNodes = 0, totalTbHits = 0;
    int completedDepth = 0;

    int startDepth = 1;// 1 + (threadId % std::min(8, totalThreads));
    auto startTime = std::chrono::steady_clock::now();

    // TODO: Make history global
    Move killerMoves[MAX_PLY][2] = {0};
    unsigned long long historyTable[2][64][64] = {0};

    for (int depth = startDepth; depth <= maxDepth; depth++) {
        if (stopSearch) break;

        std::vector<uint64_t> searchPath;
        searchPath.reserve(64);

        BestMove result;

        // ==== Aspiration Windows ====
        if (depth >= 5 && abs(bestScore) < MATE_SCORE - 100) {
            int delta = 100; // Window size; typical is 50, but I am going with 100 for now, to make sure it works
            int alpha = bestScore - delta;
            int beta = bestScore + delta;

            while (true) {
                result = alphaBeta(board, depth, 1, alpha, beta, bestMove, searchPath,
                                   killerMoves, historyTable, 0);
                if (stopSearch || !result.completed) break;

                // Is score within the window?
                if (result.score > alpha && result.score < beta) {
                    break; // yay! It worked!
                }

                // Failed low; widen lower bound
                if (result.score <= alpha) {
                    alpha = std::max(alpha - delta, -INF_SCORE);
                    // Exponentially widen the window
                    delta <<= 1; // Same as *= 2
                }
                    // Failed high; widen upper bound
                else if (result.score >= beta) {
                    beta = std::min(beta + delta, INF_SCORE);
                    delta <<= 1; // Same as *= 2
                }

                searchPath.clear();

                // Prevent inf widening
                if (delta > 1000) {
                    alpha = -INF_SCORE;
                    beta = INF_SCORE;
                }

            }
        } else {
            // Depth < 4, or mate score, use full window.
            result = alphaBeta(board, depth, 1, -INF_SCORE, INF_SCORE,
                               bestMove, searchPath, killerMoves, historyTable, 0);
        }

        if (!result.completed || stopSearch) break;

        bestMove = result.bestMove;
        bestScore = result.score;
        pv = result.pv;
        totalNodes += result.nodes;
        completedDepth = depth;
        totalTbHits += result.tbHits;

        // Print UCI info
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        {
            std::lock_guard<std::mutex> lock(g_outputMutex);

            std::string score;
            if (abs(bestScore) >= MATE_SCORE - 100) {
                int mateDistance = MATE_SCORE - std::abs(bestScore);
                int mateMoves = (mateDistance + 1) >> 1;

                score = (bestScore > 0)
                        ? std::format(" score mate {}", mateMoves)
                        : std::format(" score mate -{}", mateMoves);
            } else {
                score = std::format(" score cp {}", bestScore);
            }
            std::cout << "info "
                      << score
                      << " depth " << completedDepth
                      << " nodes " << result.nodes
                      << " tbhits " << result.tbHits
                      << " time " << elapsed
                      << " nps " << (elapsed > 0 ? (result.nodes * 1000 / elapsed) : 0)
                      << " pv ";

            for (Move &m: pv) {
                std::cout << moveToString(m) << ' ';
            }
            std::cout << std::endl << std::flush;
        }
    }

    return {bestMove, bestScore, completedDepth, pv, totalNodes, totalTbHits};
}


bool isKingInCheck(const Board &board, int color) {
    uint64_t king = color == 1 ? board.whiteKing : board.blackKing;
    return isSquareAttacked(board, std::countr_zero(king), -color);
}
