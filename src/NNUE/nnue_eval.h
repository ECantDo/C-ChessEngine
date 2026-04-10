//
// Created by ECanDo on 2026-01-06.
//

#ifndef CHESSENGINE_NNUE_EVAL_H
#define CHESSENGINE_NNUE_EVAL_H


#include <cstdint>
#include <string>
#include <algorithm>
#include <immintrin.h>
#include "Board/board.h"
#include "Search/transposition_table.h"

constexpr int NNUE_INPUT_SIZE = 768;
constexpr int NNUE_HIDDEN_SIZE = 128;
constexpr int16_t QA = 255; // don't go above 255
constexpr int16_t QB = 64;
constexpr int16_t SCALE = 400;


extern uint8_t PIECE_SQUARE_INDEXES[BLACK_KING + 1];

// int16 for speed :3
using NNUEWeight = int16_t;
using NNUEBias = int16_t;

struct alignas(64) NNUEParameters {
	NNUEWeight inputWeights[NNUE_INPUT_SIZE][NNUE_HIDDEN_SIZE]; // input x hidden
	NNUEBias inputBiases[NNUE_HIDDEN_SIZE]; // 1 bias per node

	// 2 perspectives
	NNUEWeight outputWeights[NNUE_HIDDEN_SIZE * 2];
	int16_t outputBias;
};

struct alignas(64) NNUEAccumulator {
	// Two perspectives
	int16_t white[NNUE_HIDDEN_SIZE];
	int16_t black[NNUE_HIDDEN_SIZE];
};

extern NNUEParameters g_nnueParams;
extern NNUEAccumulator g_nnueAccumulator;
extern bool g_nnueLoaded;

void initNNUE_LUTs();

void try_init_nnue(const std::string &filename);

bool initNNUEEmbedded();

inline void try_init_nnue() {
	if (!initNNUEEmbedded()) {
		std::cout << "initNNUEEmbedded() failed, please use setoption EvalFile ..." << std::endl << std::flush;
	}
}

bool initNNUE(const char *filename);

Score evaluateNNUE(const Board &board, const NNUEAccumulator &accumulator);

void initAccumulator(const Board &board, NNUEAccumulator &accumulator);

void updateAccumulatorAdd(Piece piece, int square, NNUEAccumulator &accumulator);

void updateAccumulatorRemove(Piece piece, int square, NNUEAccumulator &accumulator);

static void addWeightsSIMD(
	int16_t *accumulator,
	const int16_t *weights
) {
	// Process 16 int16 values at a time
	for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
		// Load 16 accumulator values
		__m256i acc = _mm256_load_si256(
			reinterpret_cast<const __m256i *>(accumulator + i)
		);

		// Load 16 weights
		__m256i w = _mm256_load_si256(
			reinterpret_cast<const __m256i *>(weights + i)
		);

		// acc += w
		acc = _mm256_add_epi16(acc, w);

		// Store back
		_mm256_store_si256(
			reinterpret_cast<__m256i *>(accumulator + i),
			acc
		);
	}
}

static inline void subWeightsSIMD(
	int16_t *accumulator,
	const int16_t *weights
) {
	for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
		__m256i acc = _mm256_load_si256(
			reinterpret_cast<const __m256i *>(accumulator + i)
		);

		__m256i w = _mm256_load_si256(
			reinterpret_cast<const __m256i *>(weights + i)
		);

		acc = _mm256_sub_epi16(acc, w);

		_mm256_store_si256(
			reinterpret_cast<__m256i *>(accumulator + i),
			acc
		);
	}
}

inline int getInputFeatureIndex(const Piece piece, const int square) {
	// Simple encoding: piece type (0-11) × 64 squares
	const int pieceIndex = PIECE_SQUARE_INDEXES[piece];
	assert(pieceIndex < 12);
	return pieceIndex * 64 + square;
}

#endif //CHESSENGINE_NNUE_EVAL_H
