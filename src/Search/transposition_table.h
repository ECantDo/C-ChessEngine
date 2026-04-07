//
// Created by ECanDo on 2025-12-08.
//

#ifndef CHESSENGINE_TRANSPOSITION_TABLE_H
#define CHESSENGINE_TRANSPOSITION_TABLE_H

#include <cstdint>
#include <mutex>
#include <vector>
#include <atomic>
#include "Board/move.h"

#define LOCK_SIZE_FACTOR 512
#define CLUSTER_SIZE 3

enum TTFlag : uint8_t {
	TT_EXACT = 0,
	TT_ALPHA = 1,
	TT_BETA = 2
};

struct TTEntry {
	uint64_t zobristKey;
	Move bestMove;
	int depth;
	int score;
	uint8_t flag;

	TTEntry() : zobristKey(0), bestMove(0), score(0), depth(0), flag(0) {
	}
};

struct TTCluster {
	TTEntry entry[CLUSTER_SIZE];

	TTCluster() : entry() {
		for (auto &e: entry) {
			e = TTEntry();
		}
	}
};

class TranspositionTable {
private:
	size_t size;
	size_t numLocks;
	TTCluster *table;
	std::vector<std::mutex> locks;

public:
	std::atomic<unsigned long long> overwriteSameKey{0}; // Make atomic
	std::atomic<unsigned long long> overwrites{0};
	std::atomic<unsigned long long> stored{0};

	explicit TranspositionTable(const size_t sizeMB)
		: size((sizeMB * 1024 * 1024) / sizeof(TTCluster)),
		  table(new TTCluster[size]),
		  numLocks((size / LOCK_SIZE_FACTOR) + 1),
		  locks(numLocks), // Construct vector with numLocks default-constructed mutexes
		  overwrites(0),
		  overwriteSameKey(0),
		  stored(0) {
	}

	~TranspositionTable() {
		delete[] table;
		table = nullptr;
	}

	void clear();

	void store(const uint64_t key, const Move bestMove, const int depth, const int score, const TTFlag flag) {
		const size_t index = key % size;
		const size_t lockIndex = index / LOCK_SIZE_FACTOR;

		// Lock this section of the table - other threads must wait
		std::lock_guard<std::mutex> lock(locks[lockIndex]);
		TTCluster &cluster = table[index];
		int8_t writeIndex = -1;

		// Find matching zobrist
		int8_t blankIdx = -1;
		int8_t matchingIdx = -1;
		for (int8_t i = 0; i < CLUSTER_SIZE; i++) {
			if (cluster.entry[i].zobristKey == 0 && blankIdx == -1) {
				blankIdx = i;
				continue;
			}
			if (cluster.entry[i].zobristKey == key) {
				matchingIdx = i;
				break;
			}
		}

		// Same position? Depth priority
		if (matchingIdx != -1) {
			// Always prefer deeper searches, regardless of flag
			if (cluster.entry[matchingIdx].depth > depth) {
				return; // Don't overwrite deeper with shallower
			}
			// If same depth, prefer exact scores
			if (cluster.entry[matchingIdx].depth == depth
				&& cluster.entry[matchingIdx].flag == TT_EXACT
				&& flag != TT_EXACT) {
				return; // Don't overwrite exact with bound
			}
			++overwriteSameKey;
			writeIndex = matchingIdx;
			goto writeToTable;
		}

		// Not same position, and there is blank, just write to blank
		if (blankIdx != -1) {
			++stored;
			writeIndex = blankIdx;
			goto writeToTable;
		}
		// Otherwise, shift values to the left; sudo-aging
		// and write to the right-most position, or the youngest spot
		writeIndex = CLUSTER_SIZE - 1;
		++overwrites;
		for (int8_t i = 0; i < writeIndex; i++) {
			cluster.entry[i] = cluster.entry[i + 1];
		}

	writeToTable:
		if (writeIndex < 0) return;
		table[index].entry[writeIndex].zobristKey = key;
		table[index].entry[writeIndex].bestMove = bestMove;
		table[index].entry[writeIndex].depth = depth;
		table[index].entry[writeIndex].score = score;
		table[index].entry[writeIndex].flag = flag;

		// Lock automatically releases here when the guard goes out of scope
	}

	[[nodiscard]] size_t getSize() const {
		return size;
	}

	/**
	 * Probe into the transposition table.
	 *
	 * First, have a peek into the table, and see if the zobrist key is the same at the mapped index.
	 * @param key The key to check against
	 * @param depth
	 * @param alpha
	 * @param beta
	 * @param entry
	 * @return
	 */
	bool probe(const uint64_t key, const int depth, const int alpha, const int beta, TTEntry &entry) {
		const size_t index = key % size;
		const size_t lockIndex = index / LOCK_SIZE_FACTOR;

		// Lock on read - prevent writing from another thread
		std::lock_guard<std::mutex> lock(locks[lockIndex]);

		TTCluster &cluster = table[index];
		// Find position
		int8_t eIdx = 0;
		for (; eIdx < CLUSTER_SIZE; eIdx++) {
			if (cluster.entry[eIdx].zobristKey == key) break;
		}
		// Miss
		if (eIdx == CLUSTER_SIZE) return false;

		const TTEntry &e = cluster.entry[eIdx];

		// From this point forwards, we can always use what is stored in the table; Might not be
		entry = e;

		// If the stored entry is from a shallower search, we cannot trust the bounds
		if (e.depth < depth) {
			return false;
		}

		// Exact score; fully evaluated at this node. Always usable.
		if (e.flag == TT_EXACT) {
			return true;
		}

		// TT_ALPHA means the stored score is a *upper bound* (true score ≤ stored score).
		//
		// If this upper bound is still ≤ alpha, then this position *cannot raise alpha*.
		// Therefore, the maximizing player cannot improve their best value using this node,
		// and the stored bound is sufficient to prove a prune.
		if (e.flag == TT_ALPHA && e.score <= alpha) {
			return true;
		}

		// TT_BETA means the stored score is an *upper bound* (true score ≥ stored score).
		//
		// If this upper bound is ≥ beta, then the true score is at least beta,
		// which is enough to trigger a beta cutoff. The minimizing player would avoid
		// this node, so this bound is usable for pruning.
		if (e.flag == TT_BETA && e.score >= beta) {
			return true;
		}

		return false;
	}
};

extern TranspositionTable globalTT;


#endif //CHESSENGINE_TRANSPOSITION_TABLE_H
