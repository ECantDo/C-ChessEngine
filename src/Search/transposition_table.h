//
// Created by ECanDo on 2025-12-08.
//

#ifndef CHESSENGINE_TRANSPOSITION_TABLE_H
#define CHESSENGINE_TRANSPOSITION_TABLE_H

#include <cstdint>
#include "Board/move.h"

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

    TTEntry() : zobristKey(0), bestMove(0), score(0), depth(0), flag(0) {}
};

class TranspositionTable {
private:
    TTEntry *table;
    size_t size;
public:
    TranspositionTable(size_t sizeMB) {
        size = (sizeMB * 1024 * 1024) / sizeof(TTEntry);
        table = new TTEntry[size];
    }

    ~TranspositionTable() {
        delete[] table;
        table = nullptr;
    }

    void clear();

    void store(uint64_t key, Move bestMove, int depth, int score, TTFlag flag) {
        size_t index = key % size;

        table[index].zobristKey = key;
        table[index].bestMove = bestMove;
        table[index].depth = depth;
        table[index].score = score;
        table[index].flag = flag;
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
    bool probe(uint64_t key, int depth, int alpha, int beta, TTEntry &entry) {
        TTEntry &e = table[key % size];

        if (e.zobristKey != key) {
            return false; // Miss; position not in table
        }

        // From this point forwards, we can always use what is stored in the table; Might not be
        // able to use the score, but we can use the best move
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

#endif //CHESSENGINE_TRANSPOSITION_TABLE_H
