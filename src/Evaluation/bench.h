/*
* bench.h - Fixed-depth search benchmark for OpenBench compatibility
 */

#ifndef CHESSENGINE_BENCH_H
#define CHESSENGINE_BENCH_H

#include <cstdint>
#include <chrono>
#include <iostream>
#include <string>

#include "Board/board.h"
#include "NNUE/nnue_eval.h"
#include "Search/search.h"

static const char *BENCH_POSITIONS[] = {
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	// "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
	"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
	"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
	"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
	nullptr
};

#define BENCH_DEPTH 6

inline void runBench() {
	// std::cout << "running bench" << std::endl << std::flush;
	uint64_t totalNodes = 0;

	/* Suppress per-depth UCI info lines so output is clean for OpenBench */
	bool savedPrintInfo = g_printInfo;
	g_printInfo = false;

	auto benchStart = std::chrono::steady_clock::now();
	Board board;

	for (int i = 0; BENCH_POSITIONS[i] != nullptr; i++) {
		std::string fen(BENCH_POSITIONS[i]);
		board.loadFenPosition(fen);
		globalTT.clear();

		if (g_nnueLoaded) {
			initAccumulator(board, g_nnueAccumulator);
		}

		SearchValues sv{0, 0};
		selectMove(board, BENCH_DEPTH, 0, sv, 1, UINT64_MAX);
		totalNodes += sv.nodes;
	}

	auto benchEnd = std::chrono::steady_clock::now();
	long long elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		benchEnd - benchStart).count();

	uint64_t nps = (elapsedMs > 0)
					   ? (totalNodes * 1000ULL / static_cast<uint64_t>(elapsedMs))
					   : 0;

	g_printInfo = savedPrintInfo;

	/* OpenBench requires exactly: "<nodes> nodes <nps> nps" */
	std::cout << totalNodes << " nodes " << nps << " nps" << std::endl << std::flush;
}

#endif /* CHESSENGINE_BENCH_H */
