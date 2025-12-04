//
// Created by ECanDo on 2025-12-04.
//

#include "test_moves.h"

struct MakeMoveTestData {
    std::string resultingFen;
    Move move;
};

void testMoveMaking() {
    TestResult testResults = {0, 0};

    std::cout << '\n';

    testMakeMove(testResults);

    std::cout << "\nSummary: " << testResults.pass << "/" << (testResults.pass + testResults.fail)
              << " tests pass for Move Making.\n";
}

void testMakeMove(TestResult &result) {
    // Give a FEN, set the position, list of moves, list of resulting FENs
    std::string startingFEN = "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1";

    Board board = Board(startingFEN);

    if (startingFEN != board.generateFen()) {
        std::cout << "[CRITICAL ERROR] Starting FEN does not match generated FEN -- "
                     "cannot continue with makeMove() testing";
        return;
    }

    std::vector<MakeMoveTestData> testData = {
            // Take and promote
            {"Q3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                    encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                               MOVE_FLAG_PROMOTION | PROMOTE_TO_QUEEN | MOVE_FLAG_CAPTURE)},
            {"R3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                    encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                               MOVE_FLAG_PROMOTION | PROMOTE_TO_ROOK | MOVE_FLAG_CAPTURE)},
            {"B3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                    encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                               MOVE_FLAG_PROMOTION | PROMOTE_TO_BISHOP | MOVE_FLAG_CAPTURE)},
            {"N3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                    encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                               MOVE_FLAG_PROMOTION | PROMOTE_TO_KNIGHT | MOVE_FLAG_CAPTURE)},

            // En Passant
            {"r3k2r/1P6/3P4/8/8/8/1p6/R3K2R b KQkq - 0 1",
                    encodeMove(getBoardIndex('c', '5'), getBoardIndex('d', '6'),
                               MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE)}
    };

    for (auto &t: testData) {
        board.makeMove(t.move);
        std::string r = board.generateFen();
        if (r == t.resultingFen) {
            result.pass++;
        } else {
            result.fail++;
            std::cout << "[FAIL] Move " << moveToString(t.move) << " resulted in FEN: \n\"" << r
                      << "\" instead of \n\"" << t.resultingFen << "\" Starting with FEN: \n\"" << startingFEN
                      << "\"\n\n";
        }

        board.loadFenPosition(startingFEN);
    }
}
