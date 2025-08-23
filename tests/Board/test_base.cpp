//
// Created by ECanDo on 2025-08-23.
//
#include "Board/board.h"
#include "test_base.h"
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

void test_base() {
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

    int passed = 0, failed = 0;

    for (auto &t: charIndexTests) {
        int got = getBoardIndex(t.file, t.rank);
        if (got == t.expected) {
            passed++;
        } else {
            failed++;
            std::cout << "[FAIL] Board.getBoardIndex('" << t.file << "', '" << t.rank << "') expected " << t.expected
                      << ", got " << got << "\n";
        }
    }

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
            passed++;
        } else {
            failed++;
            std::cout << "[FAIL] Board.getBoardIndex(" << t.file << ", " << t.rank << ") expected " << t.expected
                      << ", got " << got << "\n";
        }
    }

    std::vector<PositionTest> positionTests = {
            // Pass Cases
            {0, "a1"},
            {1, "a2"},
            {2, "a3"},
            {3, "a4"},
            {4, "a5"},
            {5, "a6"},
            {6, "a7"},
            {7, "a8"},
            {8, "b1"},
            {63, "h8"},

            // Fail Cases
            {-1, ""},
            {64, ""}
    };

    for (auto &t: positionTests) {
        std::string got = getBoardPosition(t.index);
        if (got == t.expected) {
            passed++;
        } else {
            failed++;
            std::cout << "[FAIL] Board.getPostion(" << t.index << ") expected " << t.expected
                      << ", got " << got << "\n";
        }
    }

    std::cout << "\nSummary: " << passed << "/" << (passed + failed) << " tests passed for `Board/board`.\n";
}