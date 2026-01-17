//
// Created by ECanDo on 2026-01-06.
//

#include <fstream>
#include "self_play_training.h"
#include "Board/board.h"
#include "Moves/move_list.h"
#include "Search/search.h"
#include "nnue_eval.h"
#include <random>

// Helper function to check if position is clearly decided
bool shouldAdjudicate(const Board &board, int score) {
	// Adjudicate if evaluation is overwhelming (> 10 pawns advantage)
	// This prevents playing out clearly won/lost positions
	return abs(score) > 1000;
}

// Helper function to check insufficient material
bool hasInsufficientMaterial(const Board &board) {
	// Only kings left
	if ((board.whitePawns | board.blackPawns | board.whiteRooks | board.blackRooks |
		 board.whiteBishops | board.blackBishops | board.whiteKnights | board.blackKnights |
		 board.whiteQueens | board.blackQueens) == 0) {
		return true;
	}

	// King vs King + minor piece (insufficient material for checkmate)
	uint64_t whitePieces = board.getWhiteBitboard();
	uint64_t blackPieces = board.getBlackBitboard();
	int whiteCount = std::popcount(whitePieces);
	int blackCount = std::popcount(blackPieces);

	// K vs K
	if (whiteCount == 1 && blackCount == 1) return true;

	// K vs K+N or K vs K+B
	if ((whiteCount == 1 && blackCount == 2 && (board.blackKnights || board.blackBishops)) ||
		(blackCount == 1 && whiteCount == 2 && (board.whiteKnights || board.whiteBishops))) {
		return true;
	}

	return false;
}

void generateTrainingData(const char *outputFile, int numGames, int depth) {
	g_printInfo = false;
	std::ofstream file(outputFile);
	if (!file) {
		std::cerr << "Failed to open output file: " << outputFile << std::endl;
		return;
	}

	std::cout << "info string Generating " << numGames << " self-play games at tc " << depth << "ms" << std::endl;
	std::mt19937 engine(std::chrono::steady_clock::now().time_since_epoch().count());

	for (int game = 0; game < numGames; game++) {
		Board board;
		std::vector<std::string> positions;
		std::vector<int> scores;  // Track evaluation for each position

		std::vector<uint64_t> gamePath;
		gamePath.reserve(200);

		// Get a random opening position (3-8 random moves for more diversity)
		std::uniform_int_distribution<int> dist(3, 8);
		int moveCount = dist(engine);

		bool validStart = false;
		int attempts = 0;

		// Try to get a valid random starting position
		while (!validStart && attempts < 10) {
			board.loadStartPosition();
			gamePath.clear();
			validStart = true;
			attempts++;

			for (int i = 0; i < moveCount; i++) {
				MoveList moveList;
				generateLegalMoves(board, moveList);

				if (moveList.empty()) {
					validStart = false;
					break;
				}

				gamePath.push_back(board.zobristHash);
				std::uniform_int_distribution<int> moveDist(0, (int) moveList.length() - 1);
				board.makeMove(moveList.get(moveDist(engine)));
			}

			// Check if position is already decided
			int eval = evaluateBoard(board);
			if (abs(eval) > 500) {  // Too one-sided
				validStart = false;
			}
		}

		if (!validStart) {
			// Fallback to starting position if we can't find good random start
			board.loadStartPosition();
			gamePath.clear();
			moveCount = 0;
		}

		int lastScore = 0;

		// Play game until termination
		while (moveCount < 200) {
			// Get legal moves
			MoveList moves;
			generateLegalMoves(board, moves);

			if (moves.empty()) {
				// Game over by checkmate or stalemate
				break;
			}

			if (hasInsufficientMaterial(board)) {
				// Draw by insufficient material
				break;
			}

			if (board.halfMoveClock >= 100) {
				// Draw by 50-move rule
				break;
			}

			if (board.isRepetitionInSearch(gamePath)) {
				// Draw by repetition
				break;
			}

			// Search for best move
			SearchValues sv{0, 0};
			BestMove bm = selectMove(board, 32, depth, sv, 1);

			if (bm.bestMove == 0) {
				break;
			}

			// Save position BEFORE making move (so perspective is correct)
			positions.push_back(board.generateFen());
			scores.push_back(bm.score);
			lastScore = bm.score;


			gamePath.push_back(board.zobristHash);
			board.makeMove(bm.bestMove);
			moveCount++;
		}

		// Determine game result (from WHITE's perspective)
		float result = 0.5;  // Default: draw

		MoveList finalMoves;
		generateLegalMoves(board, finalMoves);

		if (finalMoves.empty()) {
			// Checkmate or stalemate
			if (isKingInCheck(board, board.turn)) {
				// Checkmate - PREVIOUS player (who just moved) won
				// board.turn is the player who is checkmated
				result = (board.turn == 1) ? 0.0f : 1.0f;  // Black won : White won
			}
			// else: stalemate, result stays 0.5
		}

		// else: draw conditions (50-move, repetition, insufficient material) - stays 0.5

		// Write positions with result (result is always from WHITE's perspective)
		for (size_t i = 0; i < positions.size(); i++) {
			const auto &fen = positions[i];

			// Parse FEN to get side to move
			std::string fenCopy = fen;
			size_t spacePos = fenCopy.find(' ');
			if (spacePos != std::string::npos) {
				spacePos = fenCopy.find(' ', spacePos + 1);
				if (spacePos != std::string::npos && fenCopy[spacePos - 1] == 'b') {
					// Position is from BLACK's perspective, flip result
					file << fen << " | " << (1.0f - result) << "\n";
					continue;
				}
			}

			// Position is from WHITE's perspective
			file << fen << " | " << result << "\n";
		}

		std::cout << "info string Generated " << (game + 1) << "/" << numGames
				  << " games (" << positions.size() << " positions, result=" << result << ")"
				  << std::endl << std::flush;
	}

	file.close();
	std::cout << "info string Self-play data saved to " << outputFile << std::endl << std::flush;
	g_printInfo = true;
}

void generateSupervisedData(const std::string &cmd) {
	bool nnueLoadedValue = g_nnueLoaded;
	g_printInfo = false;
	g_nnueLoaded = false;

	std::stringstream ss(cmd);
	std::string tok;

	ss >> tok; // "supervised"
	ss >> tok; // "positions"

	int numPositions = 100;
	std::string filename = "supervised_data.txt";
	int searchDepth = 8;
	int searchTime = 50;

	while (ss >> tok) {
		if (tok == "file") ss >> filename;
		else if (tok == "depth") ss >> searchDepth;
		else if (tok == "time") ss >> searchTime;
		else numPositions = std::stoi(tok);
	}

	std::ofstream outFile(filename);
	if (!outFile) {
		std::cerr << "Failed to open " << filename << std::endl;
		g_nnueLoaded = nnueLoadedValue;
		return;
	}

	std::random_device rd;
	std::mt19937 gen(rd());

	int validPositions = 0;

	// Play games until we have enough positions
	while (validPositions < numPositions) {
		Board board;
		globalTT.clear(); // Fresh TT per game

		// Play one full game
		while (validPositions < numPositions) {
			MoveList moves;
			generateLegalMoves(board, moves);

			if (moves.empty()) break; // Game over

			// Evaluate current position
			SearchValues sv{0, 0};
			BestMove bestMove = selectMove(board, searchDepth, searchTime, sv, 1);

			if (abs(bestMove.score) >= MATE_SCORE - 100) {
				break; // Don't save this position; game over, cant use further scores, all will be mates
			}

			// Save position + eval
			outFile << board.generateFen() << " | " << bestMove.score << "\n";
			validPositions++;

			if (validPositions % 20 == 0) {
				std::cout << "Generated " << validPositions << "/" << numPositions << std::endl << std::flush;
			}

			// Make a move (with some randomness)
			Move move;
			int rng = gen() % 100;

			if (rng < 50) {
				// 50% - play best move
				move = bestMove.bestMove;
			} else if (rng < 70) {
				// 20% - play a capture
				move = moves.get(gen() % moves.length());
				for (int j = 0; j < moves.length(); j++) {
					if (getMoveFlags(moves.get(j)) & MOVE_FLAG_CAPTURE) {
						move = moves.get(j);
						break;
					}
				}
			} else {
				// 30% - random move
				move = moves.get(gen() % moves.length());
			}

			board.makeMove(move);
		}

		// Game finished, start a new one
	}

	outFile.close();
	std::cout << "\nSupervised data saved to " << filename
			  << " (" << validPositions << " positions)"
			  << std::endl << std::flush;

	g_nnueLoaded = nnueLoadedValue;
	g_printInfo = true;
}