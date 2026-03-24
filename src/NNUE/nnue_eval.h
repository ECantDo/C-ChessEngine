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

constexpr int NNUE_INPUT_SIZE = 768;
constexpr int NNUE_HIDDEN_SIZE = 128;
constexpr int32_t QA = 255;
constexpr int32_t QB = 64;
constexpr int32_t SCALE = 400;

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

void try_init_nnue(const std::string &filename);

inline void try_init_nnue() {
	try_init_nnue("quantised.bin");
}

bool initNNUE(const char *filename);

int evaluateNNUE(const Board &board, const NNUEAccumulator &accumulator);

void initAccumulator(const Board &board, NNUEAccumulator &accumulator);

void updateAccumulatorAdd(Piece piece, int square, NNUEAccumulator &accumulator);

void updateAccumulatorRemove(Piece piece, int square, NNUEAccumulator &accumulator);

static inline void addWeightsSIMD(
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

inline int getInputFeatureIndex(Piece piece, int square) {
	// Simple encoding: piece type (0-11) × 64 squares
	int pieceIndex;

	switch (piece) {
		case WHITE_PAWN:
			pieceIndex = 0;
			break;
		case WHITE_KNIGHT:
			pieceIndex = 1;
			break;
		case WHITE_BISHOP:
			pieceIndex = 2;
			break;
		case WHITE_ROOK:
			pieceIndex = 3;
			break;
		case WHITE_QUEEN:
			pieceIndex = 4;
			break;
		case WHITE_KING:
			pieceIndex = 5;
			break;
		case BLACK_PAWN:
			pieceIndex = 6;
			break;
		case BLACK_KNIGHT:
			pieceIndex = 7;
			break;
		case BLACK_BISHOP:
			pieceIndex = 8;
			break;
		case BLACK_ROOK:
			pieceIndex = 9;
			break;
		case BLACK_QUEEN:
			pieceIndex = 10;
			break;
		case BLACK_KING:
			pieceIndex = 11;
			break;
		default:
			return -1;
	}

	return pieceIndex * 64 + square;
}

inline int32_t screlu(int16_t x) {
	int32_t y = std::clamp(static_cast<int32_t>(x), 0, QA);
	return y * y;
}

#endif //CHESSENGINE_NNUE_EVAL_H
