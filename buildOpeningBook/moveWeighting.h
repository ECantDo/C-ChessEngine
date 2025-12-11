#ifndef CHESSENGINE_MOVEWEIGHTING_H
#define CHESSENGINE_MOVEWEIGHTING_H

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

#include "../src/Board/move.h"

// Returns 0 if move should be excluded, otherwise 1-255 (255 = best, 1 = worst included)
// Note: In opening book, higher weight = more likely to be picked

struct WeightingConfig {
    int maxScoreDiff = 50;      // Moves within 50cp of best are "reasonable"
    int minScore = -100;        // Don't include moves worse than -100cp
    bool useTop4Only = false;   // Only include top 4 moves regardless of score
};

// Strategy 1: Score-based threshold (recommended for opening books)
// Includes moves within maxScoreDiff of best move
inline uint8_t calculateWeightThreshold(int bestScore, int currentScore, int rank, const WeightingConfig &config = {}) {
    // Exclude terrible moves
    if (currentScore < config.minScore) {
        return 0;  // Don't include
    }

    // Only top 4 moves
    if (config.useTop4Only && rank > 4) {
        return 0;
    }

    int scoreDiff = bestScore - currentScore;

    // Exclude moves too far from best
    if (scoreDiff > config.maxScoreDiff) {
        return 0;  // Don't include
    }

    // Linear scale: best move = 255, moves at threshold = 1
    // Formula: weight = 255 - (scoreDiff * 254 / maxScoreDiff)
    int weight = 255 - (scoreDiff * 254 / config.maxScoreDiff);
    return static_cast<uint8_t>(std::clamp(weight, 1, 255));
}

// Strategy 2: Exponential decay (heavily favors best moves)
inline uint8_t calculateWeightExponential(int bestScore, int currentScore, int rank, const WeightingConfig &config = {}) {
    if (currentScore < config.minScore || (config.useTop4Only && rank > 4)) {
        return 0;
    }

    int scoreDiff = bestScore - currentScore;

    if (scoreDiff > config.maxScoreDiff) {
        return 0;
    }

    // Exponential decay: e^(-x/20) where x is score difference
    // This heavily favors the best move
    double ratio = exp(-scoreDiff / 20.0);
    int weight = static_cast<int>(ratio * 254) + 1;
    return static_cast<uint8_t>(std::clamp(weight, 1, 255));
}

// Smart hybrid approach
// Combines rank-based and score-based criteria
// Works for both White (positive scores) and Black (negative scores)
inline uint8_t calculateWeight(int bestScore, int currentScore, int rank, const WeightingConfig &config = {}) {
    // Determine if this is from Black's perspective (scores are negative)
    // Best move has the HIGHEST evaluation from current player's perspective
    // For White: best=+50, current=+40 -> diff=10
    // For Black: best=-40, current=-50 -> diff=10 (abs value comparison)

    // Calculate score difference (always positive or zero)
    // For Black, bestScore is LESS negative (higher) than worse moves
    int scoreDiff = abs(bestScore - currentScore);

    // For minimum score threshold, use absolute value
    int absCurrentScore = abs(currentScore);
    int absMinScore = abs(config.minScore);

    // Hard exclusions
    // Exclude if move is too bad (too far from 0 in wrong direction)
    bool isBadMove = false;
    if (bestScore >= 0) {
        // White's turn - lower scores are worse
        isBadMove = (currentScore < config.minScore);
    } else {
        // Black's turn - higher scores (less negative) are worse
        isBadMove = (currentScore > -config.minScore);
    }

    if (isBadMove) {
        return 0;  // Terrible move
    }

    if (config.useTop4Only && rank > 4) {
        return 0;  // Too low rank
    }

    // Exclude moves too far from best
    if (scoreDiff > config.maxScoreDiff) {
        return 0;
    }

    // Special case: if position is losing (best move still bad), be more lenient
    bool difficultPosition = abs(bestScore) > 50 &&
                             ((bestScore < 0 && bestScore < -30) ||  // Black losing badly
                              (bestScore > 0 && bestScore < 30));     // White winning but position complex
    int effectiveMaxDiff = difficultPosition ? config.maxScoreDiff + 30 : config.maxScoreDiff;

    if (scoreDiff > effectiveMaxDiff) {
        return 0;
    }

    // Base weight from score difference (linear interpolation)
    double scoreRatio = 1.0 - (static_cast<double>(scoreDiff) / effectiveMaxDiff);
    int baseWeight = static_cast<int>(scoreRatio * 254) + 1;

    // Apply rank bonus/penalty
    // Rank 1 gets small bonus, rank 5+ gets penalty
    double rankMultiplier = 1.0;
    if (rank == 1) {
        rankMultiplier = 1.1;  // 10% bonus for best move
    } else if (rank >= 5) {
        rankMultiplier = 0.8;  // 20% penalty for 5th+ move
    }

    int finalWeight = static_cast<int>(baseWeight * rankMultiplier);
    return static_cast<uint8_t>(std::clamp(finalWeight, 1, 255));
}

#endif // CHESSENGINE_MOVEWEIGHTING_H