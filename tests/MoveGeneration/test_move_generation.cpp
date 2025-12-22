//
// Created by ECanDo on 2025-12-05.
//
#include <chrono>
#include "test_move_generation.h"
#include "string"

struct TestPerftResults {
    std::string startingPosition;
    std::vector<uint64_t> results;
};

std::vector<TestPerftResults> testPerft = {
        {
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                {1, 20, 400,  8902,  197281,  4865609}
        },
        {
                "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                {1, 48, 2039, 97862, 4085603, 193690690}
        },
        {
                "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                {1, 14, 191,  2812,  43238,   674624}
        },
        {
                "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                {1, 6,  264,  9467,  422333,  15833292}
        },
        {
                "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                {1, 44, 1486, 62379, 2103487, 89941194}
        },
        {
                "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
                {1, 46, 2079, 89890, 3894594, 164075551}
        }
};

void testMoveGeneration(int depth) {
    // Test from several starting locations

    if (depth < 1 || depth > 5) {
        std::cout << "Cannot test below plys of 1, or above 5. 5 is the max plys that I have recorded\n";
        return;
    }

    Board board = Board();
//    std::string fen = "2R5/1R6/k7/8/8/8/8/4K3 b - - 10 8";
//    board.loadFenPosition(fen);
//    std::vector<Move> captures;
//    generateLegalMoves(board, captures, true);
//    for (Move m : captures){
//        std::cout << moveToString(m) << std::endl << std::flush;
//    }

    for (TestPerftResults &test: testPerft) {
        board.loadFenPosition(test.startingPosition);
        std::cout << "Testing Position " << test.startingPosition << "\n";

        auto start = std::chrono::high_resolution_clock::now();
        uint64_t nodes = perft(depth, board);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "perft found " << nodes << " in " << duration << ", at a plys of " << depth << "\n";
        if (nodes != test.results[depth]) {
            std::cout << "[FAIL] Should have found " << (long) test.results[depth] << "\n\n";
//            perftDivide(board, plys);
//            break;
        } else {
            std::cout << "[PASS]\n\n";
        }
    }
}

uint64_t perftDivide(Board &board, int depth) {
    std::vector<Move> moves;
    generateLegalMoves(board, moves);

    uint64_t totalNodes = 0;

    for (Move m: moves) {
        UndoInfo undo = board.makeMove(m);
        uint64_t nodes = perft(depth - 1, board); // plys = 1 for counting leaf moves
        board.unmakeMove(m, undo);

        totalNodes += nodes;
        std::cout << moveToString(m) << ": " << nodes << std::endl << std::flush;
    }

    std::cout << "\n\nNodes searched: " << totalNodes << "\n\n" << std::flush;
    return totalNodes;
}

uint64_t perft(int depth, Board &board) {
    if (depth <= 0) return 1;

    std::vector<Move> moveList;

    generateLegalMoves(board, moveList);

    if (depth == 1) {
        return moveList.size();
    }


    uint64_t nodes = 0;

    for (Move m: moveList) {
        UndoInfo undoInfo = board.makeMove(m);
        nodes += perft(depth - 1, board);
        board.unmakeMove(m, undoInfo);
    }

    return nodes;
}
