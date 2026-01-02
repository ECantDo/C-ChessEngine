//
// Created by ECanDo on 2025-12-06.
//

#include "search.h"
#include "Moves/move_list.h"

bool useOpeningBook = true;
std::atomic<bool> stopSearch{false};

static long g_timeLimitMS = 0;
static std::chrono::steady_clock::time_point g_searchStart;

void orderMoves(MoveList &moves, const Board &board, Move previousBest, int ply,
				Move killers[MAX_PLY][2], unsigned long long history[2][64][64]) {

	// Score all moves once
	std::vector<std::pair<Move, int>> scoredMoves;
	scoredMoves.reserve(moves.length());

	for (int i = 0; i < moves.length(); i++) {
	    Move m = moves.get(i);
		if (m == previousBest) {
			scoredMoves.emplace_back(m, 10000000); // Guarantee first
		} else {
			int score = scoreMoveForOrdering(m, board, ply, killers, history);
			scoredMoves.emplace_back(m, score);
		}
	}

	// Use partial_sort - only sort the top moves fully
	// Most beta cutoffs happen in the first few moves
	int len = (int)moves.length();
	int numToSort = len;// std::min(len, std::min(len >> 1, 8)); // Only fully sort top 8
	std::sort(
			scoredMoves.begin(),
			scoredMoves.end(),
			[](const auto &a, const auto &b) { return a.second > b.second; }
	);

	// Extract sorted moves
	for (size_t i = 0; i < moves.length(); i++) {
		moves.set(i, scoredMoves[i].first);
	}
}

int calculateExtension(Board &board) {
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
				   SearchValues &searchValues, bool nullMoveAllowed = true) {

	bool inCheck = isKingInCheck(board, board.turn);


	// ============ Check for Draw ============
	// 50 move, and repetition
	// Add current board early to check for repetition
	searchPath.push_back(board.zobristHash);

	if (board.halfMoveClock >= 100) {
		searchPath.pop_back();
		searchValues.nodes++;
		return {0, 0, plys, true, {}};
	}
	if (board.isRepetitionInSearch(searchPath)) {
		searchPath.pop_back();
		searchValues.nodes++;
		return {0, 0, plys, true, {}};  /* Draw score = 0 */
	}

	// Draw on insufficient material (one of the)
	if ((board.whitePawns | board.blackPawns | board.whiteRooks | board.blackRooks |
		 board.whiteBishops | board.blackBishops | board.whiteKnights | board.blackKnights |
		 board.whiteQueens | board.blackQueens) == 0) {
		searchPath.pop_back();
		searchValues.nodes++;
		return {0, 0, plys, true, {}};  /* Draw score = 0 */

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
		searchValues.tbHits++;
		return {ttEntry.bestMove, score, plys, true, {ttEntry.bestMove}};
	}

	// ============ Generate Moves ============
	MoveList moveList;
	generateLegalMoves(board, moveList, false);

	// ============ Legal moves is empty; check/draw ============
	if (moveList.empty()) {
		searchPath.pop_back();

		// King in check -> Mate
		if (inCheck) {
			int mateScore = -MATE_SCORE + plys;
			// Only seeing this move, or a from-here plys of 1

			globalTT.store(board.zobristHash, 0, depth, -MATE_SCORE, TT_EXACT);
			searchValues.nodes++;
			return {0, mateScore, plys, true, {}};
		}
		// King not in check -> Draw
		// Only seeing this move, or plys of 1
		globalTT.store(board.zobristHash, 0, depth, 0, TT_EXACT);
		searchValues.nodes++;
		return {0, 0, plys, true, {}};
	}

	// ============ Order Moves ============
	// Depth also happens to be the ply
	orderMoves(moveList, board, ttEntry.bestMove, plys, killerMoves, historyTable);
	Move bestMove = moveList.get(0);

	// ============ Exceeded parameters ============
	if (depth <= 0) {
		searchPath.pop_back();

		return quiescenceSearch(board, alpha, beta);
	}

	// ============ Reverse Futility Pruning ============
	// How good is my static eval? Is it so far above beta that even if I make a bad move, I will still beat beta
	if (depth <= 2 &&
		!inCheck &&
		abs(beta) < MATE_SCORE - 100) {

		int staticEval = evaluateBoard(board);
		int margin;
		// TODO: Replace with function
		switch (depth) {
			case 1:
				margin = 100;
				break;
			case 2:
				margin = 300;
				break;
			case 3:
				margin = 600;
				break;
			default:
				margin = 1000;
		}

		if (staticEval - margin >= beta) {
			searchPath.pop_back();
			searchValues.nodes++;
			return {0, staticEval - margin, plys, true, {}};
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

		int R = (depth >= 6 ? 3 : 2); // Reduction -- Basically skipping my move
		BestMove nullResult = alphaBeta(board, depth - 1 - R, plys + 1, -beta, -beta + 1,
										0, searchPath, killerMoves, historyTable,
										searchValues, false);
		// Restore values
		board.turn = oldTurn;
		board.halfMoveClock--;
		board.enPassantSquare = oldEnPass;
		board.zobristHash = oldHash;

		if (-nullResult.score >= beta) {
			BestMove verify = alphaBeta(board, depth - 1, plys + 1, beta - 1, beta,
										0, searchPath, killerMoves, historyTable,
										searchValues, false);

			if (verify.score >= beta) {
				searchPath.pop_back();
				return {0, beta, plys, true, {}};
			}
		}
	}

	int extension = calculateExtension(board);


//    if (stopSearch) {
//        return {0, 0, 1, plys, false, {bestMove}};
//    }

	int bestScore = -INF_SCORE;
	std::vector<Move> pv;

    searchValues.nodes++;
	bool completed = true;
	int movesSearched = 0;

	for (int i = 0; i < moveList.length(); i++) {
	    Move m = moveList.get(i);
		UndoInfo undo = board.makeMove(m);
		/*
		if (movesSearched > 0 &&
			depth <= 2 &&
			!inCheck &&
			!(m & MOVE_FLAG_CAPTURE) &&
			!isKingInCheck(board, board.turn) &&  // Not in check after move
			alpha < MATE_SCORE - 100) {

			int staticEval = evaluateBoard(board);  // From opponent's perspective
			int futilityMargin = (depth == 1) ? 150 : 300;

			// If opponent's position + margin is still worse than our alpha
			if (-staticEval + futilityMargin <= alpha) {
				board.unmakeMove(m, undo);
				movesSearched++;
				continue; // Skip searching this move
			}
		}
		 */


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
							   searchValues, nullMoveAllowed);

		} else {
			// Later moves: try null window search first
			if (movesSearched >= 4 && plys >= 1 &&
				!(m & MOVE_FLAG_CAPTURE) &&
				!isKingInCheck(board, -board.turn) &&
				!isKingInCheck(board, board.turn)) {
				// LMR with null window
				//int halfSize = moveList.length() >> 1;
				int reduction = 1 ;//+ (movesSearched > halfSize) /*+ (movesSearched > (halfSize >> 1) + halfSize)*/;

				// Try reduced null window search
				result = alphaBeta(board, depth - 1 - reduction, plys + 1,
								   -alpha - 1, -alpha,  // NULL WINDOW
								   0, searchPath, killerMoves, historyTable,
								   searchValues);

				// If it beat alpha, re-search at full depth
				if (-result.score > alpha) {
					result = alphaBeta(board, depth - 1, plys + 1,
									   -beta, -alpha,  // Still null window <<< FULL WINDOW, null might be slowing
									   0, searchPath, killerMoves, historyTable,
									   searchValues);

				}

				// If STILL beat alpha, do full window search
//				if (-result.score > alpha) {
//					result = alphaBeta(board, depth - 1, plys + 1,
//									   -beta, -alpha,  // FULL WINDOW
//									   0, searchPath, killerMoves, historyTable,
//									   extensionsUsed + extension);
//				}
			} else {
				// Non-LMR moves: null window then full if needed
				result = alphaBeta(board, depth - 1, plys + 1,
								   -alpha - 1, -alpha,  // NULL WINDOW
								   0, searchPath, killerMoves, historyTable,
								   searchValues);

				// Beat alpha? Re-search with full window
				if (-result.score > alpha /*&& -result.score < beta*/) {
					result = alphaBeta(board, depth - 1, plys + 1,
									   -beta, -alpha,  // FULL WINDOW
									   0, searchPath, killerMoves, historyTable,
									   searchValues);
				}
			}
		}

		int score = -result.score;

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
		if (ttScore >= MATE_SCORE - 100) {
			ttScore += plys;
		} else if (ttScore <= -MATE_SCORE + 100) {
			ttScore -= plys;
		}

		globalTT.store(board.zobristHash, bestMove, depth, ttScore, flag);

		return {bestMove, bestScore, plys, true, pv};
	} else {
		return {bestMove, bestScore, plys, false, {}};
	}
}

BestMove selectMove(Board &board, int maxDepth, long timeLimitMS, SearchValues &searchValues, int numThreads) {
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

	for (auto &thread: threads) {
		thread.join();
	}

	// All threads done - get the best result
	ThreadResult best = results[0];
	uint64_t totalNodes = results[0].nodes;
	uint64_t totalTbHits = results[0].tbHits;
	for (int i = 1; i < numThreads; i++) {
		// Prefer deeper search, or more nodes at same plys
		if (results[i].depth > best.depth ||
			(results[i].depth == best.depth && results[i].nodes > best.nodes)) {
			best = results[i];
		}
		totalNodes += results[i].nodes;
		totalTbHits += results[i].tbHits;
	}
    searchValues.nodes = totalNodes;
	searchValues.tbHits = totalTbHits;
	return {best.bestMove, best.bestScore, best.depth, true, best.pv};
}

std::mutex g_outputMutex;  // Global

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads) {
	Move bestMove = 0;
	int bestScore = 0;
	std::vector<Move> pv;
	unsigned long long totalNodes = 0, totalTbHits = 0;
	int completedDepth = 0;

	int earlyExits = 0;

	int startDepth = 1;// 1 + (threadId % std::min(8, totalThreads));
	auto startTime = std::chrono::steady_clock::now();

	// TODO: Make history global
	Move killerMoves[MAX_PLY][2] = {0};
	unsigned long long historyTable[2][64][64] = {0};

	// Early exit tracking
	Move lastBestMove = 0;
	int lastScore = 0;
	int stableMoveCount = 0;

	std::vector<uint64_t> searchPath;
	searchPath.reserve(64);

	for (int depth = startDepth; depth <= maxDepth; depth++) {
		if (stopSearch) break;
		if (g_timeLimitMS > 0) {
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_searchStart).count();

			// If we have used 60% of our time, don't start a new search
			if (elapsed > (long long) round(g_timeLimitMS * 0.6)) {
				earlyExits++;
				break;
			}
		}

		searchPath.clear();
		BestMove result;
		SearchValues searchValues{0, 0};

		// ==== Aspiration Windows ====
		if (depth >= 5 && abs(bestScore) < MATE_SCORE - 100) {
			int delta = 100; // Window size; typical is 50, but I am going with 100 for now, to make sure it works
			int alpha = bestScore - delta;
			int beta = bestScore + delta;

			while (true) {
				result = alphaBeta(board, depth, 0, alpha, beta, bestMove, searchPath,
								   killerMoves, historyTable, searchValues);
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
			result = alphaBeta(board, depth, 0, -INF_SCORE, INF_SCORE,
							   bestMove, searchPath, killerMoves, historyTable,
							   searchValues);
		}

		if (!result.completed || stopSearch) break;

		bestMove = result.bestMove;
		bestScore = result.score;
		pv = result.pv;
		totalNodes += searchValues.nodes;
		completedDepth = depth;
		totalTbHits += searchValues.tbHits;

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
					  << " nodes " << searchValues.nodes
					  << " tbhits " << searchValues.tbHits
					  << " time " << elapsed
					  << " nps " << (elapsed > 0 ? (searchValues.nodes * 1000 / elapsed) : 0)
					  << " pv ";

			for (Move &m: pv) {
				std::cout << moveToString(m) << ' ';
			}
			std::cout << std::endl << std::flush;
		}

		if (g_timeLimitMS > 0 && depth >= 5) {
			if (bestMove == lastBestMove
				&& abs(bestScore - lastScore) < 80
				&& abs(bestScore) < MATE_SCORE - 100) {

				stableMoveCount++;  // Increment here

				// Exit if stable for 3 iterations and used >30% time
				if (stableMoveCount >= 3 && elapsed > (long long) round(g_timeLimitMS * 0.3)) {
					earlyExits++;
					break;
				}
			} else {
				stableMoveCount = 0;  // Reset only when NOT stable
			}
		}
		lastScore = bestScore;
		lastBestMove = bestMove;
	}

//	if (earlyExits > 0) {
//	std::cerr << "Early exits: " << earlyExits << " at depth " << completedDepth << std::endl << std::flush;
//	}
	stopSearch = true;

	return {bestMove, bestScore, completedDepth, pv, totalNodes, totalTbHits};
}


bool isKingInCheck(const Board &board, int color) {
	uint64_t king = color == 1 ? board.whiteKing : board.blackKing;
	return isSquareAttacked(board, std::countr_zero(king), -color);
}
