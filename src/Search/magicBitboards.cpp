//
// Created by ECanDo on 2025-12-13.
//


#include <bit>
#include "magicBitboards.h"

// ============================================================================
// LOOKUP TABLES (Filled at startup)
// ============================================================================

uint64_t rook_attacks[64][4096];   // Max table size
uint64_t bishop_attacks[64][512];  // Max table size

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

uint64_t get_rook_mask(int square) {
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

    return mask;
}

uint64_t get_bishop_mask(int square) {
    uint64_t mask = 0ULL;
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

    return mask;
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

void initMagicBitboards() {
    // Initialize rook attack tables
    for (int square = 0; square < 64; square++) {
        uint64_t mask = get_rook_mask(square);
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
        uint64_t mask = get_bishop_mask(square);
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
    occupancy &= get_rook_mask(square);
    int index = (occupancy * ROOK_MAGICS[square]) >> ROOK_SHIFTS[square];
    return rook_attacks[square][index];
}

uint64_t getBishopAttacks(int square, uint64_t occupancy) {
    occupancy &= get_bishop_mask(square);
    int index = (occupancy * BISHOP_MAGICS[square]) >> BISHOP_SHIFTS[square];
    return bishop_attacks[square][index];
}

uint64_t getQueenAttacks(int square, uint64_t occupancy) {
    return getRookAttacks(square, occupancy) | getBishopAttacks(square, occupancy);
}
