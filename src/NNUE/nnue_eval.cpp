//
// Created by ECanDo on 2026-01-06.
//

#include <fstream>
#include <iostream>
#include "nnue_eval.h"
#include <cstring>

NNUEParameters g_nnueParams;
NNUEAccumulator g_nnueAccumulator; // TODO: Make per thread
bool g_nnueLoaded = false;

// Load network from binary file
bool initNNUE(const char *filename) {
	static_assert(NNUE_HIDDEN_SIZE % 16 == 0);

	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		std::cerr << "Failed to open NNUE file: " << filename << std::endl;
		return false;
	}

	// Bullet outputs a raw struct dump — no magic header, no size fields.
	// Order matches the Network struct in simple.rs:
	//   feature_weights  [768][HIDDEN_SIZE]  i16  quantised × QA
	//   feature_bias     [HIDDEN_SIZE]       i16  quantised × QA
	//   output_weights   [2*HIDDEN_SIZE]     i16  quantised × QB
	//   output_bias      [1]                 i16  quantised × QA*QB
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
void updateAccumulatorAdd(const Piece piece, int square, NNUEAccumulator &accumulator) {
	int featureIdx = getInputFeatureIndex(piece, square);
	if (featureIdx < 0) return;

	const int16_t *weights = g_nnueParams.inputWeights[featureIdx];
	addWeightsSIMD(accumulator.white, weights);

	Piece mirroredPiece = flipColor(piece);
	int mirroredSquare = square ^ 56; // Flip rank
	int mirroredFeatureIdx = getInputFeatureIndex(mirroredPiece, mirroredSquare);

	weights = g_nnueParams.inputWeights[mirroredFeatureIdx];
	addWeightsSIMD(accumulator.black, weights);
}

// Remove a piece from the accumulator
void updateAccumulatorRemove(Piece piece, int square, NNUEAccumulator &accumulator) {
	int featureIdx = getInputFeatureIndex(piece, square);
	if (featureIdx < 0) return;

	const int16_t *weights = g_nnueParams.inputWeights[featureIdx];
	subWeightsSIMD(accumulator.white, weights);

	Piece mirroredPiece = flipColor(piece);
	int mirroredSquare = square ^ 56; // Flip rank
	int mirroredFeatureIdx = getInputFeatureIndex(mirroredPiece, mirroredSquare);

	weights = g_nnueParams.inputWeights[mirroredFeatureIdx];
	subWeightsSIMD(accumulator.black, weights);
}

// Evaluate the position using the accumulator
int evaluateNNUE(const Board &board, const NNUEAccumulator &acc) {
	const int16_t *us = (board.turn == 1) ? acc.white : acc.black;
	const int16_t *them = (board.turn == 1) ? acc.black : acc.white;

	int32_t output = 0;

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		int32_t u = screlu(us[i]);
		int32_t t = screlu(them[i]);
		output += u * (int32_t) g_nnueParams.outputWeights[i];
		output += t * (int32_t) g_nnueParams.outputWeights[NNUE_HIDDEN_SIZE + i];
	}

	output /= QA; // QA²·QB → QA·QB
	output += static_cast<int32_t>(g_nnueParams.outputBias);
	output *= SCALE;
	output /= QA * QB; // remove quantisation entirely

	return output;
}
