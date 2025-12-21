//
// Created by ECanDo on 2025-12-13.
//


#include <bit>
#include <valarray>
#include "magicBitboards.h"
#include "Board/piece.h"

// ============================================================================
// LOOKUP TABLES (Filled at startup)
// ============================================================================

uint64_t rook_attacks[64][4096];   // Max table size
uint64_t bishop_attacks[64][512];  // Max table size

uint64_t KNIGHT_ATTACKS[64];
uint64_t KING_ATTACKS[64];

uint64_t PAWN_PUSHES[2][64];
uint64_t PAWN_ATTACKS[2][64];  // [color][square]
uint64_t PAWN_DOUBLE[64];

uint64_t BISHOP_ATTACK_MASKS[64];
uint64_t ROOK_ATTACK_MASKS[64];


// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

uint64_t getRookMask(int square) {
	return ROOK_ATTACK_MASKS[square];
}

uint64_t getBishopMask(int square) {
	return BISHOP_ATTACK_MASKS[square];
}

uint64_t index_to_occupancy(int index, uint64_t mask) {
	uint64_t occupancy = 0ULL;
	int bits[64];
	int bit_count = 0;

	// Extract bit positions from mask
	for (int i = 0; i < 64; i++) {
		if (mask & (1ULL << i)) {
			bits[bit_count++] = i;
		}
	}

	// Set bits according to index
	for (int i = 0; i < bit_count; i++) {
		if (index & (1 << i)) {
			occupancy |= 1ULL << bits[i];
		}
	}

	return occupancy;
}

uint64_t calculate_rook_attacks(int square, uint64_t occupancy) {
	uint64_t attacks = 0ULL;
	int rank = square >> 3;
	int file = square & 0x7;

	// North
	for (int r = rank + 1; r < 8; r++) {
		attacks |= 1ULL << (r * 8 + file);
		if (occupancy & (1ULL << (r * 8 + file))) break;
	}

	// South
	for (int r = rank - 1; r >= 0; r--) {
		attacks |= 1ULL << (r * 8 + file);
		if (occupancy & (1ULL << (r * 8 + file))) break;
	}

	// East
	for (int f = file + 1; f < 8; f++) {
		attacks |= 1ULL << (rank * 8 + f);
		if (occupancy & (1ULL << (rank * 8 + f))) break;
	}

	// West
	for (int f = file - 1; f >= 0; f--) {
		attacks |= 1ULL << (rank * 8 + f);
		if (occupancy & (1ULL << (rank * 8 + f))) break;
	}

	return attacks;
}

uint64_t calculate_bishop_attacks(int square, uint64_t occupancy) {
	uint64_t attacks = 0ULL;
	int rank = square / 8;
	int file = square % 8;

	// NE
	for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
		attacks |= 1ULL << (r * 8 + f);
		if (occupancy & (1ULL << (r * 8 + f))) break;
	}

	// NW
	for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--) {
		attacks |= 1ULL << (r * 8 + f);
		if (occupancy & (1ULL << (r * 8 + f))) break;
	}

	// SE
	for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++) {
		attacks |= 1ULL << (r * 8 + f);
		if (occupancy & (1ULL << (r * 8 + f))) break;
	}

	// SW
	for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
		attacks |= 1ULL << (r * 8 + f);
		if (occupancy & (1ULL << (r * 8 + f))) break;
	}

	return attacks;
}

// ============================================================================
// INITIALIZATION (Call this once at program startup)
// ============================================================================
void initRookMasks() {
	for (int square = 0; square < 64; square++) {
		uint64_t mask = 0;

		// Same as square /8 * 8, with integer math -> /8 shift right x3, *8 shift left x3
		//  -> results in the lowest 3 bits being 0, and the rest being the same
		int rankShift = square & (~0x7);
		int file = square & 0x7;

		// Rank (exclude edges)
		mask |= RANK_BLOCKER_MASK << rankShift;

		// File (exclude edges)
		mask |= FILE_BLOCKER_MASK << file;

		mask &= ~(1ULL << square);

		ROOK_ATTACK_MASKS[square] = mask;
	}
}

void initBishopMasks() {
	for (int square = 0; square < 64; square++) {
		int64_t mask = 0ULL;
		int rank = square >> 3;
		int file = square & 0x7;

		// I'm sad I can't do the same fun mask trick as the rook...

		// NE
		for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++) {
			mask |= 1ULL << (r * 8 + f);
		}

		// NW
		for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--) {
			mask |= 1ULL << (r * 8 + f);
		}

		// SE
		for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++) {
			mask |= 1ULL << (r * 8 + f);
		}

		// SW
		for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--) {
			mask |= 1ULL << (r * 8 + f);
		}
		BISHOP_ATTACK_MASKS[square] = mask;
	}
}

void initAttackTables() {
	int kingOffsets[8] = {-9, -8, -7, -1, 1, 7, 8, 9};
	int knightOffsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};

	for (int sq = 0; sq < 64; sq++) {
		// Knight attacks
		uint64_t attacks = 0;
		for (int offset: knightOffsets) {
			int target = sq + offset;
			if (target >= 0 && target < 64) {
				int fileDiff = abs((sq & 7) - (target & 7));
				int rankDiff = abs((sq >> 3) - (target >> 3));
				if ((fileDiff == 2 && rankDiff == 1) || (fileDiff == 1 && rankDiff == 2)) {
					attacks |= (1ULL << target);
				}
			}
		}
		KNIGHT_ATTACKS[sq] = attacks;

		// King attacks
		attacks = 0;
		for (int offset: kingOffsets) {
			int target = sq + offset;
			if (target >= 0 && target < 64) {
				int fileDiff = abs((sq & 7) - (target & 7));
				if (fileDiff <= 1) {
					attacks |= (1ULL << target);
				}
			}
		}
		KING_ATTACKS[sq] = attacks;
	}
}

void initPawnMoveTables() {
	for (int sq = 0; sq < 64; ++sq) {
		int rank = sq >> 3;
		int file = sq & 7;

		// White pawn pushes
		if (rank < 7) PAWN_PUSHES[WHITE][sq] = 1ULL << (sq + 8);
		if (rank == 1) PAWN_DOUBLE[sq] = 1ULL << (sq + 16);

		// Black pawn pushes
		if (rank > 0) PAWN_PUSHES[BLACK][sq] = 1ULL << (sq - 8);
		if (rank == 6) PAWN_DOUBLE[sq] = 1ULL << (sq - 16);

		// Attacks (same as before for isSquareAttacked)
		PAWN_ATTACKS[WHITE][sq] = 0;
		if (rank < 7) {
			if (file > 0) PAWN_ATTACKS[WHITE][sq] |= 1ULL << (sq + 7);
			if (file < 7) PAWN_ATTACKS[WHITE][sq] |= 1ULL << (sq + 9);
		}

		PAWN_ATTACKS[BLACK][sq] = 0;
		if (rank > 0) {
			if (file > 0) PAWN_ATTACKS[BLACK][sq] |= 1ULL << (sq - 9);
			if (file < 7) PAWN_ATTACKS[BLACK][sq] |= 1ULL << (sq - 7);
		}
	}
}

void initMagicBitboards() {
	initAttackTables();
	initRookMasks();
	initBishopMasks();
	initPawnMoveTables();

	// Initialize rook attack tables
	for (int square = 0; square < 64; square++) {
		uint64_t mask = getRookMask(square);
		int bits = std::popcount(mask);
		int permutations = 1 << bits;

		for (int i = 0; i < permutations; i++) {
			uint64_t occupancy = index_to_occupancy(i, mask);
			uint64_t attacks = calculate_rook_attacks(square, occupancy);

			// Hash it using magic number
			int index = (int) ((occupancy * ROOK_MAGICS[square]) >> ROOK_SHIFTS[square]);

			rook_attacks[square][index] = attacks;
		}
	}

	// Initialize bishop attack tables
	for (int square = 0; square < 64; square++) {
		uint64_t mask = getBishopMask(square);
		int bits = std::popcount(mask);
		int permutations = 1 << bits;

		for (int i = 0; i < permutations; i++) {
			uint64_t occupancy = index_to_occupancy(i, mask);
			uint64_t attacks = calculate_bishop_attacks(square, occupancy);

			// Hash it using magic number
			int index = (int) ((occupancy * BISHOP_MAGICS[square]) >> BISHOP_SHIFTS[square]);

			bishop_attacks[square][index] = attacks;
		}
	}
}

// ============================================================================
// RUNTIME LOOKUP (Fast! Just array access)
// ============================================================================

uint64_t getRookAttacks(int square, uint64_t occupancy) {
	occupancy &= getRookMask(square);
	int index = (int) ((occupancy * ROOK_MAGICS[square]) >> ROOK_SHIFTS[square]);
	return rook_attacks[square][index];
}

uint64_t getBishopAttacks(int square, uint64_t occupancy) {
	occupancy &= getBishopMask(square);
	int index = (int) ((occupancy * BISHOP_MAGICS[square]) >> BISHOP_SHIFTS[square]);
	return bishop_attacks[square][index];
}

uint64_t getQueenAttacks(int square, uint64_t occupancy) {
	return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}
