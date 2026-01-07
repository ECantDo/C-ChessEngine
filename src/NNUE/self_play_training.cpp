//
// Created by ECanDo on 2026-01-06.
//

#include <fstream>
#include "self_play_training.h"
#include "Board/board.h"
#include "Moves/move_list.h"
#include "Search/search.h"
#include <random>


void generateTrainingData(const char *outputFile, int numGames, int depth) {
	g_printInfo = false;
	std::ofstream file(outputFile);
	if (!file) {
		std::cerr << "Failed to open output file: " << outputFile << std::endl;
		return;
	}

	std::cout << "info string Generating " << numGames << " self-play games at depth " << depth << std::endl;
	std::mt19937 engine(std::chrono::steady_clock::now().time_since_epoch().count());


	for (int game = 0; game < numGames; game++) {
		Board board;
		std::vector<std::string> positions;

		std::vector<uint64_t> gamePath;
		gamePath.reserve(200);

		// Play game
		std::uniform_int_distribution<int> dist(1, 12);
		int moveCount = dist(engine);
		// Get a random position to start from

		// Repeat until there are legal moves remaining in position
		bool rep;
		MoveList moveList;
		do {
			board.loadStartPosition();
			gamePath.clear();
			rep = false;

			for (int i = 0; i < moveCount; i++) {
				gamePath.push_back(board.zobristHash);
				moveList.clear();
				generateLegalMoves(board, moveList);
				if (moveList.empty()) { // Don't have to check for only kings here
					rep = true; // illegal or checkmate/stalemate reached
					break;
				}

				std::uniform_int_distribution<int> moveDist(0, (int) moveList.length() - 1);
				board.makeMove(moveList.get(moveDist(engine)));
			}
		} while (rep);

		while (moveCount < 200) {
			// Save current position
			positions.push_back(board.generateFen());
			gamePath.push_back(board.zobristHash);

			// Get legal moves
			MoveList moves;
			generateLegalMoves(board, moves);

			if (moves.empty() || insufficientMaterial(board)) {
				break;
			}

			// Search for best move
			SearchValues sv{0, 0};
			BestMove bm = selectMove(board, depth, -1, sv, 1);  // No time limit

			if (bm.bestMove == 0) {
				break;
			}

			if (board.halfMoveClock >= 100) {
				break;
			}
			if (board.isRepetitionInSearch(gamePath)) {
				break;
			}

			board.makeMove(bm.bestMove);
			moveCount++;
		}

		// Determine game result
		float result = 0.5;  // Default: draw

		MoveList finalMoves;
		generateLegalMoves(board, finalMoves);

		if (finalMoves.empty()) {
			// Checkmate or stalemate
			if (isKingInCheck(board, board.turn)) {
				// Checkmate - previous player won
				result = (board.turn == 1) ? 0.0 : 1.0;
			}
		}

		// Write positions with result
		for (const auto &fen: positions) {
			file << fen << " | " << result << "\n";
		}
		std::cout << "info string Generated " << (game + 1) << "/" << numGames << " games" << std::endl
				  << std::flush;

	}

	file.close();
	std::cout << "info string Self-play data saved to " << outputFile << std::endl << std::flush;
	g_printInfo = true;
}