//
// Created by ECanDo on 2026-01-06.
//

#include <fstream>
#include <iostream>
#include "nnue_eval.h"
#include <cstring>
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <iomanip>

#include "incbin.h"

NNUEParameters g_nnueParams;
NNUEAccumulator g_nnueAccumulator; // TODO: Make per thread
bool g_nnueLoaded = false;
INCBIN(nnue, "(768-1024)x2-1-8.bin");


void try_init_nnue(const std::string &filename) {
	if (!initNNUE(filename.c_str())) {
		std::cout << "info string No NNUE network found, using classical evaluation" << std::endl;
	} else {
		std::cout << "info string NNUE network found, NNUE" << std::endl;
	}
}

static void loadFromPtr(const char *ptr) {
	// Step 1; load in input weights
	memcpy(g_nnueParams.inputWeights, ptr, sizeof(g_nnueParams.inputWeights));
	ptr += sizeof(g_nnueParams.inputWeights);

	// Step 2; load in input biases
	memcpy(g_nnueParams.inputBiases, ptr, sizeof(g_nnueParams.inputBiases));
	ptr += sizeof(g_nnueParams.inputBiases);

	// Step 3: output weights (bucket-contiguous in file, matches struct layout)
	memcpy(g_nnueParams.outputWeights, ptr, sizeof(g_nnueParams.outputWeights));
	ptr += sizeof(g_nnueParams.outputWeights);

	// Step 4: output biases
	memcpy(g_nnueParams.outputBias, ptr, sizeof(g_nnueParams.outputBias));

	g_nnueLoaded = true;
}

bool initNNUEEmbedded() {
	static_assert(NNUE_HIDDEN_SIZE % 16 == 0);

	const char *ptr = reinterpret_cast<const char *>(g_nnue_data);

	loadFromPtr(ptr);

	std::cout << "Loaded embedded NNUE" << std::endl << std::flush;
	return true;
}

// Load network from binary file
bool initNNUE(const char *filename) {
	static_assert(NNUE_HIDDEN_SIZE % 16 == 0);

	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open NNUE file: " << filename << std::endl;
		return false;
	}

	const std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
	if (!file.good() && !file.eof()) {
		std::cerr << "Failed to read NNUE file" << std::endl;
		return false;
	}

	const char *ptr = buffer.data();

	loadFromPtr(ptr);

	g_nnueLoaded = true;
	return true;
}

// Init acc from scratch
void initAccumulator(const Board &board, NNUEAccumulator &accumulator) {
	// Copy biases
	memcpy(accumulator.white, g_nnueParams.inputBiases, sizeof(accumulator.white));
	memcpy(accumulator.black, g_nnueParams.inputBiases, sizeof(accumulator.black));

	// Add pieces to the board
	for (int sq = 0; sq < 64; sq++) {
		const Piece piece = board.pieceAtSquare(sq);
		if (piece != NONE) {
			updateAccumulatorAdd(piece, sq, accumulator);
		}
	}
}

void updateAccumulatorAdd(const Piece piece, const int square, NNUEAccumulator &accumulator) {
	const int featureIdx = getInputFeatureIndex(piece, square);
	if (featureIdx < 0) return;

	const int16_t *weights = g_nnueParams.inputWeights[featureIdx];
	addWeightsSIMD(accumulator.white, weights);

	const Piece mirroredPiece = flipColor(piece);
	const int mirroredSquare = square ^ 56; // Flip rank
	const int mirroredFeatureIdx = getInputFeatureIndex(mirroredPiece, mirroredSquare);

	weights = g_nnueParams.inputWeights[mirroredFeatureIdx];
	addWeightsSIMD(accumulator.black, weights);
}

// Remove a piece from the accumulator
void updateAccumulatorRemove(const Piece piece, const int square, NNUEAccumulator &accumulator) {
	const int featureIdx = getInputFeatureIndex(piece, square);
	if (featureIdx < 0) return;

	const int16_t *weights = g_nnueParams.inputWeights[featureIdx];
	subWeightsSIMD(accumulator.white, weights);

	const Piece mirroredPiece = flipColor(piece);
	const int mirroredSquare = square ^ 56; // Flip rank
	const int mirroredFeatureIdx = getInputFeatureIndex(mirroredPiece, mirroredSquare);

	weights = g_nnueParams.inputWeights[mirroredFeatureIdx];
	subWeightsSIMD(accumulator.black, weights);
}

void evaluateNNUE_Debug(const Board &board, const NNUEAccumulator &accumulator) {
	const int16_t *us = (board.turn == 1) ? accumulator.white : accumulator.black;
	const int16_t *them = (board.turn == 1) ? accumulator.black : accumulator.white;

	const int activeBucket = getOutputBucket(board);
	const int pieces = std::popcount(board.getWhiteBitboard() | board.getBlackBitboard());

	// Header
	std::cout << "buckets\n";
	std::cout << "+------------+------------+\n";
	std::cout << "|   Bucket   | Evaluation |\n";
	std::cout << "+------------+------------+\n";

	for (int b = 0; b < NUM_OUTPUT_BUCKETS; b++) {
		int32_t output = 0;

		for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
			const int32_t u = screlu(us[i]);
			const int32_t t = screlu(them[i]);
			output += u * static_cast<int32_t>(g_nnueParams.outputWeights[b][i]);
			output += t * static_cast<int32_t>(g_nnueParams.outputWeights[b][NNUE_HIDDEN_SIZE + i]);
		}

		output /= QA;
		output += static_cast<int32_t>(g_nnueParams.outputBias[b]);
		output *= SCALE;
		output /= QA * QB;

		// Format: * prefix for active bucket, padded bucket number, padded eval
		bool active = (b == activeBucket);
		double evalPawns = output / 100.0;

		// Bucket cell: "* 3" or "  3", left-padded to 10 chars
		std::string bucketLabel = (active ? "* " : "  ") + std::to_string(b);
		// Eval cell: "+ 1.23" or "- 1.23", right-padded to 10 chars
		std::string sign = (evalPawns >= 0) ? "+" : "-";
		char evalBuf[16];
		std::snprintf(evalBuf, sizeof(evalBuf), "%.2f", std::abs(evalPawns));
		std::string evalLabel = sign + " " + evalBuf;

		// Pad bucket to 10 chars, eval to 10 chars
		std::cout << "| " << std::left << std::setw(10) << bucketLabel
				<< "| " << std::setw(10) << evalLabel << "|\n";
	}

	std::cout << "+------------+------------+\n";
	std::cout << "* = active bucket (material: " << pieces << " pieces)\n";
}

// Evaluate the position using the accumulator
int evaluateNNUE(const Board &board, const NNUEAccumulator &accumulator) {
	const int16_t *us = (board.turn == 1) ? accumulator.white : accumulator.black;
	const int16_t *them = (board.turn == 1) ? accumulator.black : accumulator.white;

	const int bucket = getOutputBucket(board);

	int32_t output = 0;

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		const int32_t u = screlu(us[i]);
		const int32_t t = screlu(them[i]);
		output += u * static_cast<int32_t>(g_nnueParams.outputWeights[bucket][i]);
		output += t * static_cast<int32_t>(g_nnueParams.outputWeights[bucket][NNUE_HIDDEN_SIZE + i]);
	}

	output /= QA; // QA²·QB → QA·QB
	output += static_cast<int32_t>(g_nnueParams.outputBias[bucket]);
	output *= SCALE;
	output /= QA * QB; // remove quantisation entirely

	return output;
}
