//
// Created by ECanDo on 2025-12-06.
//

#include "search.h"

#include <cassert>

#include "Moves/move_list.h"
#include "NNUE/nnue_eval.h"

bool g_printInfo = true;
bool useOpeningBook = true;
std::atomic<bool> stopSearch{false};

static long g_timeLimitMS = 0;
static std::chrono::steady_clock::time_point g_searchStart;

bool hasNonPawnMaterial(const Board &board) {
	// Check if there is something other than pawns on the board
	if (board.turn == 1) {
		return 0 !=
			   (board.whiteQueens |
				board.whiteRooks |
				board.whiteBishops |
				board.whiteKnights);
	}
	return 0 !=
		   (board.blackQueens |
			board.blackRooks |
			board.blackBishops |
			board.blackKnights);
}

BestMove alphaBeta(Board &board, Depth depth, Depth plys, Score alpha, Score beta, Move previousBest,
				   std::vector<uint64_t> &searchPath, Move killerMoves[MAX_PLY][2],
				   unsigned long long historyTable[2][64][64],
				   SearchValues &searchValues, bool nullMoveAllowed = true) {
	searchValues.nodes++;

	bool inCheck = board.isKingInCheck(); //isKingInCheck(board, board.turn);


	// ============ Check for Draw ============
	// 50 move, and repetition
	// Add current board early to check for repetition
	searchPath.push_back(board.zobristHash);

	if (board.halfMoveClock >= 100) {
		searchPath.pop_back();
		return {0, 0, plys, plys, true, {}};
	}
	if (board.isRepetitionInSearch(searchPath)) {
		searchPath.pop_back();
		return {0, 0, plys, plys, true, {}}; /* Draw score = 0 */
	}

	// Draw on insufficient material (one of the)
	if (board.insufficientMaterial()) {
		searchPath.pop_back();
		return {0, 0, plys, plys, true, {}}; /* Draw score = 0 */
	}

	// ============ TT Storage consts ============

	Score alphaOrig = alpha; // For the TT
	Score betaOrig = beta;

	// ============ TT Probe ============
	TTEntry ttEntry;
	// The plys is how many nodes from here it has been searched
	if (globalTT.probe(board.zobristHash, depth, alpha, beta, ttEntry)) {
		Score score = ttEntry.score;

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
		return {ttEntry.bestMove, score, plys, plys, true, {ttEntry.bestMove}};
	}

	// ============ Exceeded parameters ============
	if (depth <= 0) {
		searchPath.pop_back();
		BestMove qSearchRes = quiescenceSearch(board, alpha, beta, searchValues);
		if (qSearchRes.score > MATE_SCORE - 100) {
			qSearchRes.score -= plys;
		} else if (qSearchRes.score <= -MATE_SCORE + 100) {
			qSearchRes.score += plys;
		}
		assert(std::abs(qSearchRes.score) <= MATE_SCORE);
		qSearchRes.selDepth += plys;
		return qSearchRes;
	}

	Score staticEval = evaluateBoardNNUE(board);

	// Razoring
	if (!inCheck && depth <= 3) {
		if (staticEval < alpha - 400 - 250 * depth * depth) {
			searchPath.pop_back();
			BestMove qResult = quiescenceSearch(board, alpha, beta, searchValues);
			qResult.selDepth += plys;
			return qResult;
		}
	}

	// ============ Reverse Futility Pruning ============
	// How good is my static eval? Is it so far above beta that even if I make a bad move, I will still beat beta
	if (depth <= 3 &&
		!inCheck &&
		abs(beta) < MATE_SCORE - 100) {
		// int staticEval = evaluateBoardNNUE(board);
		const int margin = depth * 150;

		if (staticEval - margin >= beta) {
			searchPath.pop_back();
			searchValues.nodes++;
			return {0, static_cast<Score>(staticEval - margin), plys, plys, true, {}};
		}
	}

	// ============ Null Move Pruning ============
	if (nullMoveAllowed && !inCheck && depth >= 3 && hasNonPawnMaterial(board)) {
		// Save values
		int8_t oldTurn = board.turn;
		int oldEnPass = board.enPassantSquare;
		uint64_t oldHash = board.zobristHash;
		uint8_t oldCastling = board.castling;

		// Set up to pretend to make a move
		board.enPassantSquare = -1;
		board.halfMoveClock++;
		board.turn = static_cast<int8_t>(-board.turn);
		board.zobristHash ^= Zobrist::sideToMove;
		if (oldEnPass != -1) {
			board.zobristHash ^= Zobrist::enPassantFile[oldEnPass];
		}

		int R = 2; //(depth >= 6 ? 3 : 2); // Reduction -- Basically skipping my move
		BestMove nullResult = alphaBeta(board, depth - 1 - R, plys + 1, -beta, -beta + 1,
										0, searchPath, killerMoves, historyTable,
										searchValues, false);
		// Restore values
		board.turn = oldTurn;
		board.halfMoveClock--;
		board.enPassantSquare = oldEnPass;
		board.zobristHash = oldHash;
		board.castling = oldCastling;

		if (-nullResult.score >= beta) {
			BestMove verify = alphaBeta(board, depth - 1, plys + 1, beta - 1, beta,
										0, searchPath, killerMoves, historyTable,
										searchValues, false);

			if (verify.score >= beta) {
				searchPath.pop_back();
				return {0, beta, plys, plys, true, {}};
			}
		}
	}

	// ============ Generate Moves ============
	MoveList moveList;
	generateMoves(board, moveList, false);

	// ============ Order Moves ============
	// Only setting it up for in the list
	//	orderMoves(moveList, board, ttEntry.bestMove, plys, killerMoves, historyTable);
	std::array<int, MoveLimit> moveScores{};
	scoreAllMoves(moveList, moveScores,
				  board, ttEntry.bestMove, plys, killerMoves, historyTable);
	Move bestMove; // = moveList.get(0);


	Score bestScore = -MATE_SCORE;
	int maxSelDepth = plys;
	std::vector<Move> pv;

	bool completed = true;
	int movesSearched = 0;

	for (int i = 0; i < moveList.length(); i++) {
		selectNextBestMove(moveList, moveScores, i, static_cast<int>(moveList.length()));
		Move move = moveList.get(i);

		// Futility Pruning
		if (movesSearched > 0 && !inCheck && depth <= 8 &&
			!(move & MOVE_FLAG_CAPTURE) &&
			!(move & MOVE_FLAG_PROMOTION) &&
			abs(alpha) < MATE_SCORE - 100) {
			const int lmrDepth = std::max(0, depth - lmrTable[depth][movesSearched]);
			// int staticEval = evaluateBoardNNUE(board);
			if (staticEval + 100 + 120 * lmrDepth <= alpha) {
				continue;
			}
		}


		UndoInfo undo = board.makeMove(move);

		// Check legality - is our king now in check?
		if (board.isKingInCheck(-board.turn)) {
			board.unmakeMove(move, undo);
			continue;
		}
		// const uint64_t ourKing = (board.turn == -1) ? board.whiteKing : board.blackKing;
		// if (isSquareAttacked(board, std::countr_zero(ourKing), board.turn)) {
		// 	board.unmakeMove(move, undo);
		// 	continue; // illegal move, skip
		// }


		BestMove result;

		// ====== Extensions ======
		int extension = 0; // calculateExtension(board, move);
		const bool otherInCheck = board.isKingInCheck();
		if (otherInCheck) {
			++extension;
		}
		const int toSquare = getMoveTo(move);
		const int pieceMoved = getPieceType(board.pieceAtSquare(getMoveFrom(move)));
		if (pieceMoved == TYPE_PAWN && (toSquare == 1 || toSquare == 6)) {
			extension += 1;
		}

		// ====== Get PV node ======
		if (movesSearched == 0) {
			result = alphaBeta(board, depth - 1 + extension, plys + 1, -beta, -alpha,
							   0, searchPath, killerMoves, historyTable,
							   searchValues, nullMoveAllowed);
		} else {
			int reduction = 0;
			// LMR/Late Move Reductions
			if (depth >= 2 && movesSearched >= 2) {
				reduction = lmrTable[depth][movesSearched];

				// Reduce less for captures (SEE already ordered them well)
				if (move & MOVE_FLAG_CAPTURE) {
					reduction -= 1;
				}

				// Reduce less if in check or giving check
				if (otherInCheck) {
					reduction -= 1;
				}
			}

			// Try reduced null window search
			result = alphaBeta(board, depth - 1 - reduction, plys + 1,
							   -alpha - 1, -alpha, // NULL WINDOW
							   0, searchPath, killerMoves, historyTable,
							   searchValues, nullMoveAllowed);

			// If it beat alpha, re-search at full depth
			if (-result.score > alpha && reduction > 0) {
				result = alphaBeta(board, depth - 1 + extension, plys + 1,
								   -alpha - 1, -alpha, // Still null window <<< FULL WINDOW, null might be slowing
								   0, searchPath, killerMoves, historyTable,
								   searchValues, nullMoveAllowed);
			}

			if (-result.score > alpha) {
				result = alphaBeta(board, depth - 1 + extension, plys + 1,
								   -beta, -alpha, // Full window
								   0, searchPath, killerMoves, historyTable,
								   searchValues, nullMoveAllowed);
			}
		}

		Score score = -result.score;
		board.unmakeMove(move, undo);
		assert(std::abs(score) <= MATE_SCORE);

		movesSearched++;

		if (result.selDepth > maxSelDepth) {
			maxSelDepth = result.selDepth;
		}

		if (score > bestScore) {
			bestMove = move;
			bestScore = score;

			pv.clear();
			pv.push_back(move);
			if (!result.pv.empty() && result.pv[0] != 0) {
				pv.insert(pv.end(), result.pv.begin(), result.pv.end());
			}
		}

		if (score > alpha) {
			alpha = score;
		}

		if (alpha >= beta) {
			// If quiet move (i.e. not a capture)
			if (!(move & MOVE_FLAG_CAPTURE)) {
				// Shift old killer to slot 1, new to slot 0
				killerMoves[plys][1] = killerMoves[plys][0];
				killerMoves[plys][0] = move;

				int color = board.turn == 1 ? 0 : 1;
				int from = getMoveFrom(move);
				int to = getMoveTo(move);
				historyTable[color][from][to] += depth * depth;
				if (historyTable[color][from][to] > 100000) {
					// Age all history values
					for (int c = 0; c < 2; c++) {
						for (int f = 0; f < 64; f++) {
							for (int t = 0; t < 64; t++) {
								historyTable[c][f][t] >>= 1; // Divide by 2
							}
						}
					}
				}
			}
			break;
		}

		if (stopSearch || !result.completed) {
			completed = false;
			break;
		}
	}

	searchPath.pop_back();

	// ============ Legal moves is empty; check/draw ============
	if (movesSearched == 0 && completed) {
		// King in check -> Mate
		if (inCheck) {
			Score mateScore = plys - MATE_SCORE;
			globalTT.store(board.zobristHash, 0, depth, -MATE_SCORE, TT_EXACT);
			return {0, mateScore, plys, plys, true, {}};
		}
		// King not in check -> Draw
		// Only seeing this move, or plys of 1
		globalTT.store(board.zobristHash, 0, depth, 0, TT_EXACT);
		return {0, 0, plys, plys, true, {}};
	}

	if (completed) {
		assert(std::abs(bestScore) <= MATE_SCORE);
		// ==== STORE TT MOVE ====
		TTFlag flag;
		if (bestScore <= alphaOrig) {
			flag = TT_ALPHA;
		} else if (bestScore >= betaOrig) {
			flag = TT_BETA;
		} else {
			flag = TT_EXACT;
		}

		Score ttScore = bestScore;
		if (ttScore >= MATE_SCORE - 100) {
			ttScore += plys;
		} else if (ttScore <= -MATE_SCORE + 100) {
			ttScore -= plys;
		}

		globalTT.store(board.zobristHash, bestMove, depth, ttScore, flag);
		return {bestMove, bestScore, plys, maxSelDepth, true, pv};
	}
	return {bestMove, bestScore, plys, maxSelDepth, false, {}};
}

BestMove selectMove(Board &board, int maxDepth, const long timeLimitMS, SearchValues &searchValues, int numThreads,
					uint64_t maxNodes) {
	stopSearch = false;
	g_timeLimitMS = timeLimitMS;
	g_searchStart = std::chrono::steady_clock::now();

	std::vector<std::thread> threads;
	std::vector<ThreadResult> results(numThreads);

	// Launch all threads
	for (int i = 0; i < numThreads; i++) {
		//        std::cerr << "Launching thread " << i << std::endl;

		threads.emplace_back([&results, board, maxDepth, i, numThreads, maxNodes]() {
			results[i] = searchThread(board, maxDepth, i, numThreads, maxNodes);
		});
	}
	// All threads are running

	// Main thread monitors time ONLY if there's a time limit
	if (g_timeLimitMS > 0) {
		while (!stopSearch) {
			auto now = std::chrono::steady_clock::now();
			unsigned long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				now - g_searchStart).count();

			if (elapsed >= g_timeLimitMS) {
				stopSearch = true;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
	return {best.bestMove, best.bestScore, best.depth, best.selDepth, true, best.pv};
}

std::mutex g_outputMutex; // Global

ThreadResult searchThread(Board board, int maxDepth, int threadId, int totalThreads, uint64_t maxNodes) {
	Move bestMove = 0;
	Score bestScore = 0;
	std::vector<Move> pv;
	unsigned long long totalNodes = 0, totalTbHits = 0;
	int completedDepth = 0;

	// TODO: Make nnueAcc per thread rather than global (mandatory 1 thread)
	if (g_nnueLoaded) {
		initAccumulator(board, g_nnueAccumulator);
	}

	int earlyExits = 0;

	int startDepth = 1; // 1 + (threadId % std::min(8, totalThreads));
	auto startTime = std::chrono::steady_clock::now();

	// TODO: Make history global
	Move killerMoves[MAX_PLY][2] = {0};
	unsigned long long historyTable[2][64][64] = {0};

	// Early exit tracking
	Move lastBestMove = 0;
	Score lastScore = 0;
	int stableMoveCount = 0;
	int selDepth = 0;

	std::vector<uint64_t> searchPath;
	searchPath.reserve(maxDepth);

	int searchAgainCounter = 0;

	for (int rootDepth = startDepth; rootDepth <= maxDepth; rootDepth++) {
		if (stopSearch) break;
		if (g_timeLimitMS > 0) {
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_searchStart).count();

			// If we have used 60% of our time, don't start a new search
			if (elapsed > static_cast<long long>(round(g_timeLimitMS * 0.6))) {
				break;
			}
		}

		if (totalNodes > maxNodes) {
			break;
		}

		BestMove result;
		SearchValues searchValues{0, 0};

		// ==== Aspiration Windows ====
		// Search is at least 200 ms faster without it -- get a better eval?


		if (rootDepth >= 6 && abs(bestScore) < 2000) {
			int delta = 15; // Window size; typical is 50, but I am going with 100 for now, to make sure it works
			Score alpha = bestScore - delta;
			Score beta = bestScore + delta;

			int failedHighCnt = 0;
			int fails = 0;
			while (true) {
				searchPath.clear();
				int adjustedDepth = std::max(1, rootDepth - failedHighCnt - 3 * (searchAgainCounter + 1) / 4);


				result = alphaBeta(board, adjustedDepth, 0, alpha, beta, bestMove, searchPath,
								   killerMoves, historyTable, searchValues);
				if (stopSearch || !result.completed) break;

				// Failed high; widen upper bound
				if (result.score <= alpha) {
					beta = alpha;
					alpha = std::max(result.score - delta, -INF_SCORE);

					failedHighCnt = 0;
				} else if (result.score >= beta) {
					alpha = std::max(static_cast<Score>(beta - delta), alpha);
					beta = std::min(result.score + delta, INF_SCORE);
					++failedHighCnt;
				} else
					break;

				if (++fails >= 5) {
					searchPath.clear();
					result = alphaBeta(board, rootDepth, 0, -INF_SCORE, INF_SCORE,
									   bestMove, searchPath, killerMoves, historyTable,
									   searchValues);
					// std::cout << "Fails 4" << std::endl << std::flush;
					break;
				}
				delta += delta / 3; // widen by 33% instead of doubling
			}
		} else {
			// Depth < 6, or mate score, use full window.
			result = alphaBeta(board, rootDepth, 0, -INF_SCORE, INF_SCORE,
							   bestMove, searchPath, killerMoves, historyTable,
							   searchValues);
		}

		if (!result.completed || stopSearch) break;

		bestMove = result.bestMove;
		bestScore = result.score;
		pv = result.pv;
		totalNodes += searchValues.nodes;
		completedDepth = rootDepth;
		totalTbHits += searchValues.tbHits;
		selDepth = result.selDepth;

		Score absBestScore = abs(bestScore);
		// Print UCI info
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

		// Stable
		// if (rootDepth >= 5 && g_timeLimitMS > 0) {
		// 	if (bestMove == lastBestMove
		// 		&& abs(bestScore - lastScore) < 80
		// 		&& absBestScore < MATE_SCORE - 100
		// 	) {
		// 		stableMoveCount++; // Increment here
		//
		// 		// Exit if stable for 3 iterations and used >30% time
		// 		if (stableMoveCount >= 3 && elapsed > static_cast<long long>(round(g_timeLimitMS * 0.3))) {
		// 			earlyExits++;
		// 			stopSearch = true;
		// 		}
		// 	} else {
		// 		stableMoveCount = 0; // Reset only when NOT stable
		// 	}
		// }

		// Mate distance calculation; Mate distance will be -1 if there is not a forced mate
		// greater than 1 otherwise
		int mateDistance = -1;
		int mateMoves = -1;
		if (abs(bestScore) >= MATE_SCORE - 100) {
			mateDistance = MATE_SCORE - std::abs(bestScore);
			mateMoves = (mateDistance + 1) >> 1;
		}


		// Early exit on Mate; but do a search first to depth 6
		if (mateDistance >= 0 && rootDepth >= 6) {
			// mateDistance is in plies, mateMoves is in moves (for UCI output)

			// Exit if we've searched 2+ plies deeper than the mate distance
			// OR if it's a short mate (≤3 moves) and we've reached depth 6
			if (rootDepth >= mateDistance + 2 || mateDistance <= 6) {
				earlyExits++;
				stopSearch = true;
			}
		}

		if (g_printInfo) {
			std::lock_guard<std::mutex> lock(g_outputMutex);

			std::string score;
			if (mateDistance >= 0) {
				score = (bestScore > 0)
							? std::format(" score mate {}", mateMoves)
							: std::format(" score mate -{}", mateMoves);
			} else {
				score = std::format(" score cp {}", bestScore);
			}
			std::cout << "info "
					<< score
					<< " depth " << completedDepth
					<< " seldepth " << result.selDepth
					<< " nodes " << totalNodes
					//<< " tbhits " << searchValues.tbHits
					<< " time " << elapsed
					<< " hashfull " << (globalTT.stored * 1000) / (globalTT.getSize() * CLUSTER_SIZE)
					<< " nps " << (elapsed > 0 ? (totalNodes * 1000 / elapsed) : 0)
					<< " pv ";

			for (Move &m: pv) {
				std::cout << moveToString(m) << ' ';
			}
			std::cout << std::endl << std::flush;
		}


		lastScore = bestScore;
		lastBestMove = bestMove;
	}

	//	if (earlyExits > 0) {
	//	std::cerr << "Early exits: " << earlyExits << " at depth " << completedDepth << std::endl << std::flush;
	//	}
	stopSearch = true;

	return {bestMove, bestScore, completedDepth, selDepth, pv, totalNodes, totalTbHits};
}
