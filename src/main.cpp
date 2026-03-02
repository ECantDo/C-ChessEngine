#include "Board/board.h"
#include "Board/move.h"
#include "Search/search.h"
#include "Board/zobrist_hash.h"
#include "Evaluation/evaluation.h"
#include "../tests/MoveGeneration/test_move_generation.h"
#include "NNUE/nnue_eval.h"
#include "NNUE/self_play_training.h"

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#define VERSION "V22.0_NNUE_Attempt2"

bool debug = false;
Board currentBoard;
int g_numThreads = 1;

std::atomic<bool> searchRunning{false};
std::atomic<bool> isPondering{false};
std::thread mainSearchThread;
Move ponderMove = 0;

/*-------------------------------------------------------------
 * Function to run the search in a separate thread
 *-------------------------------------------------------------*/
void runSearchThread(Board board, long timeLimit, long depth, int numThreads, uint64_t maxNodes) {
	globalTT.overwrites = 0;
	globalTT.overwriteSameKey = 0;

	std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	SearchValues searchValues{0, 0};
	BestMove bm = selectMove(board, depth, timeLimit, searchValues, numThreads, maxNodes);

	if (bm.bestMove == 0) {
		MoveList moves;
		generateLegalMoves(board, moves);
		if (!moves.empty()) {
			bm.bestMove = moves.get(0);
			std::cerr << "WARNING: Search returned null move, using fallback: "
					<< moveToString(bm.bestMove) << std::endl;
		} else {
			std::cout << "bestmove (none)\n" << std::flush;
			searchRunning = false;
			isPondering = false;
			return;
		}
	}

	if (debug) {
		std::cout << "info string |"
				<< " TT Stored = " << globalTT.stored
				<< " TT Overwrite = " << globalTT.overwrites
				<< " TT Overwrite same key " << globalTT.overwriteSameKey
				<< " TT Size = " << globalTT.getSize()
				<< std::endl << std::flush;
	}

	long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - startTime).count();

	std::string score;
	if (abs(bm.score) >= MATE_SCORE - 100) {
		int mateDistance = MATE_SCORE - abs(bm.score);
		int mateMoves = (mateDistance + 1) / 2;

		if (bm.score > 0)
			score = std::format("score mate {}", mateMoves);
		else
			score = std::format("score mate -{}", mateMoves);
	} else {
		score = std::format("score cp {}", bm.score);
	}

	/* Only output info and bestmove if not pondering or if pondering was converted to regular search */
	if (!isPondering) {
		std::cout << "info "
				<< score
				<< " depth " << bm.plys
				<< " seldepth " << bm.selDepth
				//<< " tbhits " << searchValues.tbHits
				<< " nodes " << searchValues.nodes
				<< " time " << elapsed
				<< " hashfull " << (globalTT.stored * 1000) / (globalTT.getSize() * CLUSTER_SIZE)
				<< " nps " << (elapsed > 0 ? (searchValues.nodes * 1000 / elapsed) : 0)
				<< " pv";
		for (Move &m: bm.pv) {
			std::cout << ' ' << moveToString(m);
		}
		std::cout << std::endl << std::flush;

		/* Store ponder move if available (second move in PV) */
		Move ponderOutput = 0;
		if (bm.pv.size() >= 2) {
			ponderOutput = bm.pv[1];
		}

		std::cout << "bestmove " << moveToString(bm.bestMove);
		if (ponderOutput != 0) {
			std::cout << " ponder " << moveToString(ponderOutput);
		}
		std::cout << '\n' << std::flush;
	}

	searchRunning = false;
	isPondering = false;
}

/*-------------------------------------------------------------
 * Parse "position ..." command
 *-------------------------------------------------------------*/
void setPosition(const std::string &line) {
	std::stringstream ss(line);
	std::string tok;

	ss >> tok; /* "position" */
	ss >> tok;

	if (tok == "startpos") {
		currentBoard = Board(); /* Should initialize startpos */
		if (ss >> tok && tok == "moves") {
			while (ss >> tok) {
				currentBoard.makeMove(stringToMove(tok, currentBoard));
			}
		}
	} else if (tok == "fen") {
		std::string fen, part;
		fen.clear();

		for (int i = 0; i < 6 && ss >> part; i++) {
			if (!fen.empty()) fen += " ";
			fen += part;
		}

		currentBoard = Board(fen);

		currentBoard.gameHistory.clear();

		if (ss >> tok && tok == "moves") {
			while (ss >> tok) {
				currentBoard.gameHistory.push_back(currentBoard.zobristHash);
				currentBoard.makeMove(stringToMove(tok, currentBoard));
			}
		}
	}
}

void parseDebug(const std::string &cmd) {
	std::stringstream ss(cmd);
	std::string tok;

	ss >> tok; /* "debug" */

	ss >> tok; /* "on" or "off" */

	if (tok == "on") debug = true;
	else if (tok == "off") debug = false;
}

/*-------------------------------------------------------------
 * Start search (possibly in separate thread)
 *-------------------------------------------------------------*/
void startSearch(const std::string &goCmd) {
	/* If search is already running, stop it first */
	if (searchRunning) {
		stopSearch = true;
		if (mainSearchThread.joinable()) {
			mainSearchThread.join();
		}
	}

	stopSearch = false;
	bool isPeft = false;

	long movetime = -1; /* exact time to use (ms) */
	long depth = -1; /* plys limit */
	long nodes = -1; /* node limit */

	long wtime = -1, btime = -1; /* remaining time (ms) */
	long winc = 0, binc = 0; /* increments (ms) */

	std::stringstream ss(goCmd);
	std::string tok;
	ss >> tok; /* "go" */

	while (ss >> tok) {
		if (tok == "movetime") ss >> movetime;
		else if (tok == "depth") ss >> depth;
		else if (tok == "nodes") ss >> nodes;

		else if (tok == "wtime") ss >> wtime;
		else if (tok == "btime") ss >> btime;
		else if (tok == "winc") ss >> winc;
		else if (tok == "binc") ss >> binc;
		else if (tok == "perft") {
			ss >> depth;
			isPeft = true;
		}
	}

	/*---------------------------------------------------------
	 * If no limits were explicitly given, derive a time limit
	 *---------------------------------------------------------*/
	long timeLimit = 0;

	if (movetime > 0) {
		timeLimit = movetime;
		depth = 50; /* No need in going any higher than 50 */
	} else if (wtime >= 0 && btime >= 0) {
		/* Allocate time based on whose move it is */
		long remaining = (currentBoard.turn == 1 ? wtime : btime);
		long increment = (currentBoard.turn == 1 ? winc : binc);

		/* Basic time allocation: use 1/30 of remaining + 80% of increment (allow for some overhead) */
		timeLimit = remaining / 30 + (long) (increment * 0.8);

		/* Safety clamp: never more than 80% of remaining */
		if (timeLimit > remaining * 4 / 5)
			timeLimit = remaining * 4 / 5;

		depth = 50;
	} else {
		/* No time controls given — default to plys search */
		if (depth <= 0)
			depth = 10; /* fallback */
	}

	/* When pondering, use infinite time and depth */

	if (timeLimit > 100) {
		timeLimit -= 80; /* Allow for 80ms of outputting time */
	}


	if (!isPeft) {
		/* Launch search in separate thread */
		searchRunning = true;
		Board boardCopy = currentBoard; /* Copy board for thread safety */

		if (mainSearchThread.joinable()) {
			mainSearchThread.join();
		}

		mainSearchThread = std::thread(runSearchThread, boardCopy, timeLimit, depth, g_numThreads, nodes);
	} else {
		/* Perft runs in main thread (it's fast and synchronous) */
		auto start = std::chrono::high_resolution_clock::now();

		uint64_t totalNodes = perft(depth, currentBoard);
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		std::cout << totalNodes << std::endl << std::flush;
		//std::cout << "Took " << duration << " ms"
		//<< " nps " << (duration > 0 ? (totalNodes * 1000 / duration) : 0)
		//<< std::endl << std::flush;
	}
}

void try_init_nnue(const std::string &filename) {
	if (!initNNUE(filename.c_str())) {
		std::cout << "info string No NNUE network found, using classical evaluation" << std::endl;
	}
}

void try_init_nnue() {
	try_init_nnue("quantised.bin");
}

/*-------------------------------------------------------------
 * UCI main loop
 *-------------------------------------------------------------*/
int main() {
	Zobrist::init();
	globalTT.clear();
	initMagicBitboards();

	/* Try to load NNUE network */
	try_init_nnue();
	if (g_nnueLoaded) {
		initAccumulator(currentBoard, g_nnueAccumulator); // ← add this
	}

	//    std::string openingBookLocation = "./openingBook.bin";
	//    loadBookToHashMap(openingBookLocation);

	std::ios::sync_with_stdio(false);
	std::cin.tie(nullptr);

	std::string line;

	while (std::getline(std::cin, line)) {
		if (line.empty()) continue;

		if (line.rfind("go", 0) == 0) {
			/* Keep at the top, the most common input */
			startSearch(line);
		} else if (line == "uci") {
			std::cout << std::format("id name ECanBot-{}\n", VERSION) << std::flush;
			std::cout << "id author ECanDo\n" << std::flush;

			/* Advertise pondering support */
			std::cout << "option name Ponder type check default false\n" << std::flush;

			/* Future options: */
			// std::cout << "option name Hash type spin default 16 min 1 max 4096\n";

			std::cout << "uciok\n" << std::flush;
		} else if (line == "isready") {
			/* Wait for search to finish if running */
			if (mainSearchThread.joinable() && searchRunning) {
				mainSearchThread.join();
			}
			std::cout << "readyok\n" << std::flush;
		} else if (line.rfind("setoption", 0) == 0) {
			/* TODO: handle engine options */
		} else if (line == "ucinewgame") {
			/* Stop any running search */
			stopSearch = true;
			if (mainSearchThread.joinable()) {
				mainSearchThread.join();
			}
			currentBoard = Board();
			globalTT.clear();
			ponderMove = 0;
		} else if (line.rfind("position", 0) == 0) {
			/* Stop any running search before changing position */
			stopSearch = true;
			if (mainSearchThread.joinable()) {
				mainSearchThread.join();
			}
			try {
				setPosition(line);
			} catch (std::invalid_argument &e) {
				std::cerr << e.what() << std::endl;
			}
		} else if (line == "stop") {
			stopSearch = true;
			/* Don't join here - let search finish naturally and output bestmove */
		} else if (line == "quit") {
			stopSearch = true;
			if (mainSearchThread.joinable()) {
				mainSearchThread.join();
			}
			break;
		} else if (line == "d") {
			currentBoard.printBoard();
			std::cout << currentBoard.generateFen() << std::endl << std::flush;
		} else if (line == "eval") {
			std::cout << "Evaluation: " << evaluateBoardNNUE(currentBoard) << std::endl;
			std::cout << "FEN: " << currentBoard.generateFen() << std::endl << std::flush;
		} else if (line.rfind("debug", 0) == 0) {
			//            rootDebugAlphaBeta(currentBoard, 6);
			parseDebug(line);
		} else if (line.rfind("selfplay", 0) == 0) {
			std::stringstream ss(line);
			std::string cmd;
			int numGames = 10000;
			int search_nodes = 150;
			std::string filename = "selfplay_data.txt";

			ss >> cmd; /* "selfplay" */

			/* Parse optional parameters */
			std::string tok;
			while (ss >> tok) {
				if (tok == "games") ss >> numGames;
				else if (tok == "nodes") ss >> search_nodes;
				else if (tok == "file") ss >> filename;
			}

			generateTrainingData(filename.c_str(), numGames, search_nodes);
		} else if (line.rfind("reload", 0) == 0) {
			std::stringstream ss(line);
			std::string cmd;
			std::string filename = "quantised.bin";

			ss >> cmd; /* "selfplay" */

			/* Parse optional parameters */
			std::string tok;
			while (ss >> tok) {
				if (tok == "file") ss >> filename;
			}

			/* Stop any running search */
			stopSearch = true;
			if (mainSearchThread.joinable()) {
				mainSearchThread.join();
			}
			currentBoard = Board();
			globalTT.clear();
			ponderMove = 0;
			try_init_nnue(filename);
		}
	}

	return 0;
}
