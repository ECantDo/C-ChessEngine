//
// Created by ECanDo on 2025-12-06.
//

#include "search.h"

bool useOpeningBook = true;
std::atomic<bool> stopSearch{false};

static long g_timeLimitMS = 0;
static std::chrono::steady_clock::time_point g_searchStart;

int scoreMoveForOrdering(Move m, const Board &board, int ply,
                         Move killers[MAX_PLY][2], int history[2][64][64]) {
    int flags = getMoveFlags(m);

    int to = getMoveTo(m);
    int from = getMoveFrom(m);

    // 1. CAPTURES (highest priority)
    if (flags & MOVE_FLAG_CAPTURE) {

        char victim = board.pieceAtSquare(to);
        char attacker = board.pieceAtSquare(from);
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

    // 3. KILLER MOVES (non-captures that caused cutoffs at this depth)
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
                Move killers[MAX_PLY][2], int history[2][64][64]) {
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

BestMove alphaBeta(Board &board, int depth, int maxDepth, int alpha, int beta, Move previousBest,
                   std::vector<uint64_t> &searchPath, Move killerMoves[MAX_PLY][2], int historyTable[2][64][64],
                   int extensionsUsed = 0, bool nullMoveAllowed = true) {



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
        return {0, 0, 1, 0, depth + extensionsUsed, true, {}};  /* Draw score = 0 */

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
    bool inCheck = isKingInCheck(board, board.turn);

    // ============ Legal moves is empty; check/draw ============
    if (moveList.empty()) {
        // King in check -> Mate
        if (inCheck) {
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
    // Depth also happens to be the ply
    orderMoves(moveList, board, ttEntry.bestMove, depth, killerMoves, historyTable);
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

    // ============ Null Move Pruning ============
//    if (nullMoveAllowed && !inCheck && depth >= 4 && hasNonPawnMaterial(board)) {
//        // Save values
//        int8_t oldTurn = board.turn;
//        int oldEnPass = board.enPassantSquare;
//        uint64_t oldHash = board.zobristHash;
//
//        // Set up to pretend to make a move
//        board.enPassantSquare = -1;
//        board.halfMoveClock++;
//        board.turn = (int8_t) (-board.turn);
//        board.zobristHash ^= Zobrist::sideToMove;
//        if (oldEnPass != -1){
//            board.zobristHash ^= Zobrist::enPassantFile[oldEnPass];
//        }
//
//        int R = 2; // Reduction -- Basically skipping my move
//        BestMove nullResult = alphaBeta(board, depth + 1, maxDepth - R, -beta, -beta + 50,
//                                        0, searchPath, killerMoves, historyTable,
//                                        extensionsUsed, false);
//        // Restore values
//        board.turn = oldTurn;
//        board.halfMoveClock--;
//        board.enPassantSquare = oldEnPass;
//        board.zobristHash = oldHash;
//
//        if (-nullResult.score >= beta) {
//            // Cutoff
//            return {0, beta, nullResult.nodes, nullResult.tbHits, depth, true, {}};
//        }
//    }

    int extension = calculateExtension(board, extensionsUsed);

    // ============ Exceeded parameters ============
    if (depth >= maxDepth + extension || depth >= MAX_PLY) {
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
    int movesSearched = 0;

    for (Move m: moveList) {
        UndoInfo undo = board.makeMove(m);

        BestMove result;

        // LATE MOVE REDUCTIONS
        // Reduce if:
        // - Not first few moves
        // - Not a capture
        // - Not in check
        // - Not giving check
        // - Depth is high enough
        if (movesSearched >= 5 &&
            depth >= 3 &&
            !(m & MOVE_FLAG_CAPTURE) &&
            !isKingInCheck(board, -board.turn) && // Am I in check?
            !isKingInCheck(board, board.turn) // Is opponent in check
                ) {

            int reduction = 1;
            // TODO: Tweak
            if (movesSearched > 6 && depth > 6) {
                reduction = 2; // More reduction for later moves
            }

            result = alphaBeta(board, depth + 1, maxDepth - 1 - reduction, -beta, -alpha,
                               0, searchPath, killerMoves, historyTable,
                               extensionsUsed + extension);

            int score = -result.score;

            // If better than expected, re-search at full depth
            if (score > alpha) {
                result = alphaBeta(board, depth + 1, maxDepth, -beta, -alpha,
                                   0, searchPath, killerMoves, historyTable,
                                   extensionsUsed + extension);
            }
        } else {
            // Normal full depth search
            // TODO: PVS (Principal Variation Search
            result = alphaBeta(board, depth + 1, maxDepth, -beta, -alpha,
                               0, searchPath, killerMoves, historyTable,
                               extensionsUsed + extension);
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
            pv.insert(pv.end(), result.pv.begin(), result.pv.end());
        }

        if (score > alpha) {
            alpha = score;
        }

        if (alpha >= beta) {
            // If quiet move (i.e. not a capture)
            if (!(m & MOVE_FLAG_CAPTURE)) {
                // Shift old killer to slot 1, new to slot 0
                killerMoves[depth][1] = killerMoves[depth][0];
                killerMoves[depth][0] = m;

                int color = board.turn == 1 ? 0 : 1;
                int from = getMoveFrom(m);
                int to = getMoveTo(m);
                historyTable[color][from][to] += depth * depth;
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

        globalTT.store(board.zobristHash, bestMove, maxDepth - depth, bestScore, flag);

        return {bestMove, bestScore, nodes, tbHits, depth, true, pv};
    } else {
        return {bestMove, bestScore, nodes, tbHits, depth, false, {}};
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
        // Prefer deeper search, or more nodes at same depth
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

    int startDepth = 1 + (threadId % std::min(8, totalThreads));
    auto startTime = std::chrono::steady_clock::now();

    // TODO: Make history global
    Move killerMoves[MAX_PLY][2] = {0};
    int historyTable[2][64][64] = {0};

    for (int depth = startDepth; depth <= maxDepth; depth++) {
        if (stopSearch) break;

        std::vector<uint64_t> searchPath;
        searchPath.reserve(30);

        BestMove result = alphaBeta(board, 0, depth, -INF_SCORE, INF_SCORE,
                                    bestMove, searchPath, killerMoves, historyTable, 0);

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
