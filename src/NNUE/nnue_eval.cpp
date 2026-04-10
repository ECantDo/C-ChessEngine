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


uint8_t PIECE_SQUARE_INDEXES[BLACK_KING + 1] = {};

void initNNUE_LUTs() {
	memset(PIECE_SQUARE_INDEXES, 255, sizeof(PIECE_SQUARE_INDEXES));
	PIECE_SQUARE_INDEXES[WHITE_PAWN] = 0;
	PIECE_SQUARE_INDEXES[WHITE_KNIGHT] = 1;
	PIECE_SQUARE_INDEXES[WHITE_BISHOP] = 2;
	PIECE_SQUARE_INDEXES[WHITE_ROOK] = 3;
	PIECE_SQUARE_INDEXES[WHITE_QUEEN] = 4;
	PIECE_SQUARE_INDEXES[WHITE_KING] = 5;
	PIECE_SQUARE_INDEXES[BLACK_PAWN] = 6;
	PIECE_SQUARE_INDEXES[BLACK_KNIGHT] = 7;
	PIECE_SQUARE_INDEXES[BLACK_BISHOP] = 8;
	PIECE_SQUARE_INDEXES[BLACK_ROOK] = 9;
	PIECE_SQUARE_INDEXES[BLACK_QUEEN] = 10;
	PIECE_SQUARE_INDEXES[BLACK_KING] = 11;
}


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
	g_nnueLoaded = false;
	static_assert(NNUE_HIDDEN_SIZE % 16 == 0);

	const auto ptr = reinterpret_cast<const char *>(g_nnue_data);

	loadFromPtr(ptr);

	std::cout << "Loaded embedded NNUE" << std::endl << std::flush;
	return true;
}

// Load network from binary file
bool initNNUE(const char *filename) {
	g_nnueLoaded = false;
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
Score evaluateNNUE(const Board &board, const NNUEAccumulator &accumulator) {
	const int16_t *us = (board.turn == 1) ? accumulator.white : accumulator.black;
	const int16_t *them = (board.turn == 1) ? accumulator.black : accumulator.white;
	const int16_t *w0 = g_nnueParams.outputWeights;
	const int16_t *w1 = g_nnueParams.outputWeights + NNUE_HIDDEN_SIZE;

	const __m256i zero = _mm256_setzero_si256();
	const __m256i qa = _mm256_set1_epi16(QA);

	__m256i sum = _mm256_setzero_si256();

	for (int i = 0; i < NNUE_HIDDEN_SIZE; i += 16) {
		// ── "us" half ──────────────────────────────────────────────────
		__m256i u = _mm256_load_si256(reinterpret_cast<const __m256i *>(us + i));
		__m256i wu = _mm256_load_si256(reinterpret_cast<const __m256i *>(w0 + i));

		u = _mm256_max_epi16(u, zero);
		u = _mm256_min_epi16(u, qa); // clamp to [0, 255]

		__m256i u_sq = _mm256_mullo_epi16(u, u); // clamp², safe: 255²=65025 < 65535

		__m256i u_sq_lo = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(u_sq)); // unsigned extend
		__m256i u_sq_hi = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(u_sq, 1));
		__m256i wu_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(wu)); // weights stay signed
		__m256i wu_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(wu, 1));

		sum = _mm256_add_epi32(sum, _mm256_mullo_epi32(u_sq_lo, wu_lo));
		sum = _mm256_add_epi32(sum, _mm256_mullo_epi32(u_sq_hi, wu_hi));

		// ── "them" half ────────────────────────────────────────────────
		__m256i t = _mm256_load_si256(reinterpret_cast<const __m256i *>(them + i));
		__m256i wt = _mm256_load_si256(reinterpret_cast<const __m256i *>(w1 + i));

		t = _mm256_max_epi16(t, zero);
		t = _mm256_min_epi16(t, qa);

		__m256i t_sq = _mm256_mullo_epi16(t, t);

		__m256i t_sq_lo = _mm256_cvtepu16_epi32(_mm256_castsi256_si128(t_sq));
		__m256i t_sq_hi = _mm256_cvtepu16_epi32(_mm256_extracti128_si256(t_sq, 1));
		__m256i wt_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(wt));
		__m256i wt_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(wt, 1));

		sum = _mm256_add_epi32(sum, _mm256_mullo_epi32(t_sq_lo, wt_lo));
		sum = _mm256_add_epi32(sum, _mm256_mullo_epi32(t_sq_hi, wt_hi));
	}

	// Horizontal reduction: 8 int32 lanes → scalar
	__m128i lo = _mm256_castsi256_si128(sum);
	__m128i hi = _mm256_extracti128_si256(sum, 1);
	__m128i s = _mm_add_epi32(lo, hi);
	s = _mm_add_epi32(s, _mm_srli_si128(s, 8));
	s = _mm_add_epi32(s, _mm_srli_si128(s, 4));
	int32_t output = _mm_cvtsi128_si32(s);

	output /= QA;
	output += static_cast<int32_t>(g_nnueParams.outputBias);
	output *= SCALE;
	output /= QA * QB;

	return static_cast<Score>(output);
}
