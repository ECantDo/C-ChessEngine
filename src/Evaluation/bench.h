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
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
	"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
	"4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
	"rq3rk1/ppp2ppp/1bnpN3/3N2B1/4P3/7P/PPPQ1PP1/2KR3R b - - 0 14",
	"r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4PpP1/1BNP4/PPP2P1P/3R1RK1 b - g3 0 14",
	"r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
	"r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
	"r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
	"4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
	"2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
	"r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
	"3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
	"r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
	"4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
	"3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
	"6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
	"3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
	"2K5/p7/7P/5pR1/8/5k2/r7/8 w - - 4 3",
	"8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
	"7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
	"8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
	"8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
	"8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
	"8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1",
	nullptr
};

inline void runBench() {
	// std::cout << "running bench" << std::endl << std::flush;
	uint64_t totalNodes = 0;

	/* Suppress per-depth UCI info lines so output is clean for OpenBench */
	const bool savedPrintInfo = g_printInfo;
	// g_printInfo = false;

	const auto benchStart = std::chrono::steady_clock::now();
	Board board;

	for (int i = 0; BENCH_POSITIONS[i] != nullptr; i++) {
		std::string fen(BENCH_POSITIONS[i]);
		std::cout << "Pos " << (i + 1) << "/25 (" << fen << ")" << std::endl << std::flush;
		board.loadFenPosition(fen);
		globalTT.clear();

		if (g_nnueLoaded) {
			initAccumulator(board, g_nnueAccumulator);
		}

		SearchValues sv{0, 0};
		selectMove(board, 12, 10000, sv, 1, -1);
		totalNodes += sv.nodes;
		std::cout << std::endl;
	}

	const auto benchEnd = std::chrono::steady_clock::now();
	const long long elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		benchEnd - benchStart).count();

	const uint64_t nps = (elapsedMs > 0)
							 ? (totalNodes * 1000ULL / static_cast<uint64_t>(elapsedMs))
							 : 0;

	g_printInfo = savedPrintInfo;

	/* OpenBench requires exactly: "<nodes> nodes <nps> nps" */
	std::cout
			<< "=========================================" << std::endl
			<< "Total Time (ms): " << elapsedMs << std::endl
			<< "Nodes Searched : " << totalNodes << std::endl
			<< "Nodes/Second   : " << nps << std::endl
			<< totalNodes << " nodes" << std::endl
			<< std::flush;
}

#endif /* CHESSENGINE_BENCH_H */
