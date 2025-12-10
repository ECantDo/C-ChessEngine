//
// Created by ECanDo on 2025-12-08.
//

#include "test_zobrist.h"


/* Test that incremental updates match from-scratch computation */
void testZobrist() {
    Zobrist::init();
    Board board;
    board.loadStartPosition();

    uint64_t hash1 = board.zobristHash;
    uint64_t hash2 = board.computeZobristHash();

    std::cout << "Hash from init: " << std::hex << hash1 << std::endl;
    std::cout << "Hash from compute: " << std::hex << hash2 << std::endl;
    std::cout << "Match: " << (hash1 == hash2 ? "YES" : "NO") << std::endl;

    /* Make a move */
    Move m = encodeMove(12, 28, 0);  /* e2e4 */
    UndoInfo undo = board.makeMove(m);

    hash1 = board.zobristHash;
    hash2 = board.computeZobristHash();

    std::cout << "After e2e4:" << std::endl;
    std::cout << "Hash incremental: " << std::hex << hash1 << std::endl;
    std::cout << "Hash from scratch: " << std::hex << hash2 << std::endl;
    std::cout << "Match: " << (hash1 == hash2 ? "YES" : "NO") << std::endl;

    std::cout << std::dec;
}