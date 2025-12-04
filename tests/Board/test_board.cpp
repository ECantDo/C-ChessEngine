//
// Created by ECanDo on 2025-08-23.
//
#include "Board/board.h"
#include "test_board.h"
#include <iostream>
#include <vector>
#include <string>

struct IndexTestChar {
    char file;
    char rank;
    int expected;
};

struct IndexTestInt {
    int file;
    int rank;
    int expected;
};

struct PositionTest {
    int index;
    std::string expected;
};

void testBoard() {
    TestResult testResults = {0, 0};

    // Test getting the index: a1 -> 0; (0, 0) -> 0; h8 -> 63; (7, 7) -> 63
    testGetBoardIndexLetters(testResults);
    testGetBoardIndexNumbers(testResults);

    // Test going the other way
    testGetBoardPosition(testResults);

    // Test Board gen from FEN
    testConvertFenString(testResults);


    std::cout << "\nSummary: " << testResults.pass << "/" << (testResults.pass + testResults.fail)
              << " tests pass for `Board/board`.\n";
}

void testConvertFenString(TestResult &results) {
    std::vector<std::string> fenTests = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2",

            "7k/3N2qp/b5r1/2p1Q1N1/Pp4PK/7P/1P3p2/6r1 w - - 7 4",
    };

    Board board = Board();
    std::string resultingFEN;

    for (auto &t: fenTests) {
        board.loadFenPosition(t);

        resultingFEN = board.generateFen();

        if (resultingFEN != t) {
            std::cout << "[FAIL] Board.generateFen() does not match provided FEN;\nInput: " << t << "\nOutput: "
                      << resultingFEN << "\n\n";
            board.printBoard();
            results.fail++;
            continue;
        }

        results.pass++;
    }
}

void testGetBoardIndexLetters(TestResult &results) {
    std::vector<IndexTestChar> charIndexTests = {
            // Pass Cases
            {'a', '1', 0},
            {'a', '2', 1},
            {'a', '3', 2},
            {'a', '4', 3},
            {'a', '5', 4},
            {'a', '6', 5},
            {'a', '7', 6},
            {'a', '8', 7},
            {'b', '1', 8},
            {'b', '2', 9},
            {'h', '1', 56},
            {'h', '2', 57},
            {'h', '3', 58},
            {'h', '4', 59},
            {'h', '5', 60},
            {'h', '6', 61},
            {'h', '7', 62},
            {'h', '8', 63},

            // Fail Cases
            {'a', '0', -1},
            {'`', '1', -1},
            {'a', '9', -1},
            {'i', '1', -1}

    };

    for (auto &t: charIndexTests) {
        int got = getBoardIndex(t.file, t.rank);
        if (got == t.expected) {
            results.pass++;
        } else {
            results.fail++;
            std::cout << "[FAIL] Board.getBoardIndex('" << t.file << "', '" << t.rank << "') expected " << t.expected
                      << ", got " << got << "\n";
        }
    }
}

void testGetBoardIndexNumbers(TestResult &results) {
    std::vector<IndexTestInt> intIndexTests = {
            // Pass Cases
            {0,  0,  0},
            {0,  1,  1},
            {0,  2,  2},
            {0,  3,  3},
            {0,  4,  4},
            {0,  5,  5},
            {0,  6,  6},
            {0,  7,  7},
            {1,  0,  8},
            {7,  7,  63},

            // Fail Cases
            {-1, 0,  -1},
            {0,  -1, -1},
            {8,  0,  -1},
            {0,  8,  -1}
    };

    for (auto &t: intIndexTests) {
        int got = getBoardIndex(t.file, t.rank);
        if (got == t.expected) {
            results.pass++;
        } else {
            results.fail++;
            std::cout << "[FAIL] Board.getBoardIndex(" << t.file << ", " << t.rank << ") expected " << t.expected
                      << ", got " << got << "\n";
        }
    }
}

void testGetBoardPosition(TestResult &results) {
    std::vector<PositionTest> positionTests = {
            // Pass Cases
            {0,  "a1"},
            {1,  "a2"},
            {2,  "a3"},
            {3,  "a4"},
            {4,  "a5"},
            {5,  "a6"},
            {6,  "a7"},
            {7,  "a8"},
            {8,  "b1"},
            {63, "h8"},

            // Fail Cases
            {-1, ""},
            {64, ""}
    };

    for (auto &t: positionTests) {
        std::string got = getBoardPosition(t.index);
        if (got == t.expected) {
            results.pass++;
        } else {
            results.fail++;
            std::cout << "[FAIL] Board.getPosition(" << t.index << ") expected " << t.expected
                      << ", got " << got << "\n";
        }
    }
}