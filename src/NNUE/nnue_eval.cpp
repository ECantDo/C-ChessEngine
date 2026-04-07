//
// Created by ECanDo on 2026-01-06.
//

#include <fstream>
#include <iostream>
#include "nnue_eval.h"
#include <cstring>
#define INCBIN_PREFIX g_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include <cassert>

#include "incbin.h"

NNUEParameters g_nnueParams;
NNUEAccumulator g_nnueAccumulator; // TODO: Make per thread
bool g_nnueLoaded = false;
INCBIN(nnue, "(768-128)x2-1.bin");


void try_init_nnue(const std::string &filename) {
	if (!initNNUE(filename.c_str())) {
		std::cout << "info string No NNUE network found, using classical evaluation" << std::endl;
	} else {
		std::cout << "info string NNUE network found, NNUE" << std::endl;
	}
}


static void loadFromPtr(const char *ptr) {
	memcpy(g_nnueParams.inputWeights, ptr, sizeof(g_nnueParams.inputWeights));
	ptr += sizeof(g_nnueParams.inputWeights);
	memcpy(g_nnueParams.inputBiases, ptr, sizeof(g_nnueParams.inputBiases));
	ptr += sizeof(g_nnueParams.inputBiases);
	memcpy(g_nnueParams.outputWeights, ptr, sizeof(g_nnueParams.outputWeights));
	ptr += sizeof(g_nnueParams.outputWeights);
	memcpy(&g_nnueParams.outputBias, ptr, sizeof(g_nnueParams.outputBias));

	g_nnueLoaded = true;
}

bool initNNUEEmbedded() {
	static_assert(NNUE_HIDDEN_SIZE % 16 == 0);

	const auto ptr = reinterpret_cast<const char *>(g_nnue_data);

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
		if (const Piece piece = board.pieceAtSquare(sq);
			piece != NONE) {
			updateAccumulatorAdd(piece, sq, accumulator);
		}
	}
}

void updateAccumulatorAdd(const Piece piece, const int square, NNUEAccumulator &accumulator) {
	assert(piece != NONE);
	assert(square >= 0 && square < 64);
	const int featureIdx = getInputFeatureIndex(piece, square);

	assert(featureIdx >= 0);

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
	assert(piece != NONE);
	assert(square >= 0 && square < 64);
	const int featureIdx = getInputFeatureIndex(piece, square);
	assert(featureIdx >= 0);

	const int16_t *weights = g_nnueParams.inputWeights[featureIdx];
	subWeightsSIMD(accumulator.white, weights);

	const Piece mirroredPiece = flipColor(piece);
	const int mirroredSquare = square ^ 56; // Flip rank
	const int mirroredFeatureIdx = getInputFeatureIndex(mirroredPiece, mirroredSquare);

	weights = g_nnueParams.inputWeights[mirroredFeatureIdx];
	subWeightsSIMD(accumulator.black, weights);
}

// Evaluate the position using the accumulator
int evaluateNNUE(const Board &board, const NNUEAccumulator &accumulator) {
	const int16_t *us = (board.turn == 1) ? accumulator.white : accumulator.black;
	const int16_t *them = (board.turn == 1) ? accumulator.black : accumulator.white;

	int32_t output = 0;

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i++) {
		const int32_t u = screlu(us[i]);
		const int32_t t = screlu(them[i]);
		output += u * static_cast<int32_t>(g_nnueParams.outputWeights[i]);
		output += t * static_cast<int32_t>(g_nnueParams.outputWeights[NNUE_HIDDEN_SIZE + i]);
	}

	output /= QA; // QA²·QB → QA·QB
	output += static_cast<int32_t>(g_nnueParams.outputBias);
	output *= SCALE;
	output /= QA * QB; // remove quantisation entirely

	return output;
}
