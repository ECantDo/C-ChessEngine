//
// Created by ECanDo on 2026-01-06.
// Output format: "fen | score | result"
//   score  : centipawns, WHITE-relative (positive = white better)
//   result : WHITE-relative: 1.0 = white wins, 0.5 = draw, 0.0 = black wins
// This is exactly what bulletformat's DirectSequentialDataLoader expects.
//

#include <fstream>
#include "self_play_training.h"
#include "Board/board.h"
#include "Moves/move_list.h"
#include "Search/search.h"
#include "nnue_eval.h"
#include <random>

// =====================================================================================================================
// Helpers
// =====================================================================================================================

bool hasInsufficientMaterial(const Board &board) {
	// Only kings left
	if ((board.whitePawns | board.blackPawns | board.whiteRooks | board.blackRooks |
		 board.whiteBishops | board.blackBishops | board.whiteKnights | board.blackKnights |
		 board.whiteQueens | board.blackQueens) == 0) {
		return true;
	}

	uint64_t whitePieces = board.getWhiteBitboard();
	uint64_t blackPieces = board.getBlackBitboard();
	int whiteCount = std::popcount(whitePieces);
	int blackCount = std::popcount(blackPieces);

	if (whiteCount == 1 && blackCount == 1) return true;

	if ((whiteCount == 1 && blackCount == 2 && (board.blackKnights || board.blackBishops)) ||
		(blackCount == 1 && whiteCount == 2 && (board.whiteKnights || board.whiteBishops))) {
		return true;
	}

	return false;
}

// Score is from white's perspective (as your evaluator returns).
// Returns true if the game should be ended early due to a lopsided position.
static bool shouldAdjudicate(int whiteRelativeScore, int &adjudicationCount) {
	// Require 4 consecutive plies with |eval| > 1000cp before adjudicating.
	// This avoids cutting off positions that just had a big capture.
	if (abs(whiteRelativeScore) > 1000) {
		adjudicationCount++;
	} else {
		adjudicationCount = 0;
	}
	return adjudicationCount >= 4;
}

// =====================================================================================================================
// Position record
// =====================================================================================================================
struct PositionRecord {
	std::string fen;
	int scoreWhiteRelative; // centipawns, white-relative (raw from evaluator)
};

// =====================================================================================================================
// generateTrainingData
// =====================================================================================================================
void generateTrainingData(const char *outputFile, int numGames, int searchNodes) {
	g_printInfo = false;

	std::ofstream file(outputFile, std::ios::app);
	if (!file) {
		std::cerr << "Failed to open output file: " << outputFile << std::endl;
		g_printInfo = true;
		return;
	}

	std::cout << "info string Generating " << numGames
			<< " self-play games at " << searchNodes << " nodes" << std::endl;

	std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

	long long totalPositions = 0;

	for (int game = 0; game < numGames; game++) {
		Board board;
		std::vector<PositionRecord> records;
		records.reserve(120);

		std::vector<uint64_t> gamePath;
		gamePath.reserve(200);

		// ---- Random opening (4-8 random half-moves) ----
		std::uniform_int_distribution<int> openingDist(8, 10);
		int openingMoves = openingDist(rng);

		bool validStart = false;
		while (!validStart) {
			board.loadStartPosition();
			gamePath.clear();
			validStart = true;

			for (int i = 0; i < openingMoves; i++) {
				MoveList moveList;
				generateLegalMoves(board, moveList);
				if (moveList.empty()) {
					validStart = false;
					break;
				}

				gamePath.push_back(board.zobristHash);
				std::uniform_int_distribution<int> moveDist(0, (int) moveList.length() - 1);
				board.makeMove(moveList.get(moveDist(rng)));
			}

			if (validStart && abs(evaluateBoard(board)) > 1000) {
				validStart = false;
			}
		}

		// ---- Play game ----
		int moveCount = 0;
		int adjudicationCount = 0;
		bool adjudicated = false;
		int adjudicatedScore = 0;

		while (moveCount < 200) {
			MoveList moves;
			generateLegalMoves(board, moves);

			if (moves.empty()) break;
			if (hasInsufficientMaterial(board)) break;
			if (board.halfMoveClock >= 100) break;
			if (board.isRepetitionInSearch(gamePath)) break;

			SearchValues sv{0, 0};
			BestMove bm = selectMove(board, 32, 50, sv, 1, searchNodes);
			if (bm.bestMove == 0) break;

			// bm.score is from side-to-move's perspective (negamax).
			// Convert to white-relative for storage.
			int whiteRelativeScore = (board.turn == 1) ? bm.score : -bm.score;

			/* Skip positions in check — noisy, not representative of quiet positions */
			bool inCheck = isKingInCheck(board, board.turn);

			/* Skip positions where the next move is a capture — eval will change drastically */
			bool nextMoveIsCapture = (getMoveFlags(bm.bestMove) & MOVE_FLAG_CAPTURE) != 0;

			/* Skip positions with extreme evals — likely tactical noise */
			bool extremeEval = (abs(whiteRelativeScore) > 1200);

			bool isMateScore = (abs(bm.score) >= MATE_SCORE - 100);

			if (!isMateScore && !inCheck && !nextMoveIsCapture && !extremeEval) {
				records.push_back({board.generateFen(), whiteRelativeScore});
			}

			gamePath.push_back(board.zobristHash);
			board.makeMove(bm.bestMove);
			moveCount++;

			if (!isMateScore && shouldAdjudicate(whiteRelativeScore, adjudicationCount)) {
				adjudicated = true;
				adjudicatedScore = whiteRelativeScore;
				break;
			}
		}

		// ---- Determine game result (WHITE-relative) ----
		float whiteResult = 0.5f;

		if (adjudicated) {
			whiteResult = (adjudicatedScore > 0) ? 1.0f : 0.0f;
		} else {
			MoveList finalMoves;
			generateLegalMoves(board, finalMoves);

			if (finalMoves.empty() && isKingInCheck(board, board.turn)) {
				// board.turn is the side that was checkmated
				whiteResult = (board.turn == 1) ? 0.0f : 1.0f;
			}
			// stalemate / 50-move / repetition / insufficient material → 0.5
		}

		// ---- Write ----
		// bulletformat text: "fen | score | result"
		//   score  = white-relative centipawns
		//   result = white-relative (1.0/0.5/0.0)
		for (const auto &rec: records) {
			file << rec.fen
					<< " | " << rec.scoreWhiteRelative
					<< " | " << whiteResult
					<< "\n";
		}

		totalPositions += (long long) records.size();

		std::cout << "info string Game " << (game + 1) << "/" << numGames
				<< "  positions=" << records.size()
				<< "  result=" << whiteResult
				<< (adjudicated ? "  [adjudicated]" : "")
				<< "  total=" << totalPositions
				<< std::endl << std::flush;
	}

	file.close();
	std::cout << "info string Done. " << totalPositions
			<< " positions written to " << outputFile << std::endl << std::flush;
	g_printInfo = true;
}
