//
// Created by ECanDo on 2026-01-06.
//

#include <fstream>
#include <iostream>
#include "nnue_eval.h"

NNUEParameters g_nnueParams;
NNUEAccumulator g_nnueAccumulator; // TODO: Make per thread
bool g_nnueLoaded = false;

// Load network from binary file
bool initNNUE(const char *filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open NNUE file: " << filename << std::endl;
		return false;
	}

	// Read magic header (for validation)
	uint32_t magic;
	file.read(reinterpret_cast<char *>(&magic), sizeof(magic));
	if (magic != 0x4E4E5545) {  // "NNUE" in hex
		std::cerr << "Invalid NNUE file format" << std::endl;
		return false;
	}

	// Read network dimensions
	uint32_t inputSize, hiddenSize;
	file.read(reinterpret_cast<char *>(&inputSize), sizeof(inputSize));
	file.read(reinterpret_cast<char *>(&hiddenSize), sizeof(hiddenSize));

	if (inputSize != NNUE_INPUT_SIZE || hiddenSize != NNUE_HIDDEN_SIZE) {
		std::cerr << "Network size mismatch" << std::endl;
		return false;
	}

	// Read all weights and biases
	file.read(reinterpret_cast<char *>(g_nnueParams.inputWeights),
			  sizeof(g_nnueParams.inputWeights));
	file.read(reinterpret_cast<char *>(g_nnueParams.inputBiases),
			  sizeof(g_nnueParams.inputBiases));
	file.read(reinterpret_cast<char *>(g_nnueParams.outputWeights),
			  sizeof(g_nnueParams.outputWeights));
	file.read(reinterpret_cast<char *>(&g_nnueParams.outputBias),
			  sizeof(g_nnueParams.outputBias));

	if (!file.good()) {
		std::cerr << "Failed to read NNUE weights" << std::endl;
		return false;
	}

	g_nnueLoaded = true;
	std::cout << "NNUE network loaded successfully" << std::endl;
	return true;
}

// Init acc from scratch
void initAccumulator(const Board &board, NNUEAccumulator &accumulator) {
	// Copy biases
	memcpy(accumulator.white, g_nnueParams.inputBiases, sizeof(accumulator.white));
	memcpy(accumulator.black, g_nnueParams.inputBiases, sizeof(accumulator.black));

	// Add pieces to the board
	for (int sq = 0; sq < 64; sq++) {
		Piece piece = board.pieceAtSquare(sq);
		if (piece != NONE) {
			updateAccumulatorAdd(piece, sq, accumulator);
		}
	}
}

// TODO: Use SIMD
void updateAccumulatorAdd(Piece piece, int square, NNUEAccumulator &accumulator) {
	int featureIdx = getInputFeatureIndex(piece, square);
	if (featureIdx < 0) return;

	// Update white's perspective
	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		accumulator.white[i] += g_nnueParams.inputWeights[featureIdx][i];
	}

	// Update black's perspective (mirror the square)
	int mirroredSquare = square ^ 56;  // Flip rank
	int mirroredFeatureIdx = getInputFeatureIndex(piece, mirroredSquare);

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		accumulator.black[i] += g_nnueParams.inputWeights[mirroredFeatureIdx][i];
	}
}

// Remove a piece from the accumulator
void updateAccumulatorRemove(Piece piece, int square, NNUEAccumulator &acc) {
	int featureIdx = getInputFeatureIndex(piece, square);
	if (featureIdx < 0) return;

	// Update white's perspective
	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		acc.white[i] -= g_nnueParams.inputWeights[featureIdx][i];
	}

	// Update black's perspective
	int mirroredSquare = square ^ 56;
	int mirroredFeatureIdx = getInputFeatureIndex(piece, mirroredSquare);

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		acc.black[i] -= g_nnueParams.inputWeights[mirroredFeatureIdx][i];
	}
}

// Evaluate the position using the accumulator
int evaluateNNUE(const Board &board, const NNUEAccumulator &acc) {
	// Choose perspective based on side to move
	const int16_t *us = (board.turn == 1) ? acc.white : acc.black;
	const int16_t *them = (board.turn == 1) ? acc.black : acc.white;

	// Output layer computation
	int32_t output = g_nnueParams.outputBias;

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		// Apply ClippedReLU activation
		int16_t usActivated = crelu(us[i]);
		int16_t themActivated = crelu(them[i]);

		// Weighted sum
		output += usActivated * g_nnueParams.outputWeights[i];
		output += themActivated * g_nnueParams.outputWeights[NNUE_HIDDEN_SIZE + i];
	}

	// Scale to centipawns (adjust this divisor based on your training)
	// This depends on how you scaled weights during training
	return output / 64;
}
