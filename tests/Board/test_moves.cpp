//
// Created by ECanDo on 2025-12-04.
//

#include "test_moves.h"

struct MakeMoveTestData {
    std::string resultingFen;
    std::string startingFen;
    Move move;
};

std::vector<MakeMoveTestData> allTests = {
        /* ===== WHITE PROMOTIONS ===== */
        {"Q3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1",
                encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_QUEEN | MOVE_FLAG_CAPTURE)},

        {"R3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1",
                encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_ROOK | MOVE_FLAG_CAPTURE)},

        {"B3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1",
                encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_BISHOP | MOVE_FLAG_CAPTURE)},

        {"N3k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQk - 0 1",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1",
                encodeMove(getBoardIndex('b', '7'), getBoardIndex('a', '8'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_KNIGHT | MOVE_FLAG_CAPTURE)},

        /* White promotion without capture */
        {"rQ2k2r/8/8/2Pp4/8/8/1p6/R3K2R b KQkq - 0 1",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1",
                encodeMove(getBoardIndex('b', '7'), getBoardIndex('b', '8'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_QUEEN)},

        /* ===== BLACK PROMOTIONS ===== */
        {"r3k2r/1P6/8/2Pp4/8/8/8/q3K2R w Kkq - 0 2",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('b', '2'), getBoardIndex('a', '1'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_QUEEN | MOVE_FLAG_CAPTURE)},

        {"r3k2r/1P6/8/2Pp4/8/8/8/r3K2R w Kkq - 0 2",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('b', '2'), getBoardIndex('a', '1'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_ROOK | MOVE_FLAG_CAPTURE)},

        {"r3k2r/1P6/8/2Pp4/8/8/8/b3K2R w Kkq - 0 2",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('b', '2'), getBoardIndex('a', '1'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_BISHOP | MOVE_FLAG_CAPTURE)},

        {"r3k2r/1P6/8/2Pp4/8/8/8/n3K2R w Kkq - 0 2",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('b', '2'), getBoardIndex('a', '1'),
                           MOVE_FLAG_PROMOTION | PROMOTE_TO_KNIGHT | MOVE_FLAG_CAPTURE)},

        /* ===== EN PASSANT ===== */
        /* White captures black pawn */
        {"r3k2r/1P6/3P4/8/8/8/1p6/R3K2R b KQkq - 0 1",
                "r3k2r/1P6/8/2Pp4/8/8/1p6/R3K2R w KQkq d6 0 1",
                encodeMove(getBoardIndex('c', '5'), getBoardIndex('d', '6'),
                           MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE)},

        /* Black captures white pawn */
        {"r3k2r/1p6/8/8/8/3p4/1P6/R3K2R w KQkq - 0 2",
                "r3k2r/1p6/8/8/2pP4/8/1P6/R3K2R b KQkq d3 0 1",
                encodeMove(getBoardIndex('c', '4'), getBoardIndex('d', '3'),
                           MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE)},

        /* ===== CASTLING ===== */
        /* White kingside */
        {"r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1",
                "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                encodeMove(getBoardIndex('e', '1'), getBoardIndex('g', '1'),
                           MOVE_FLAG_CASTLING)},

        /* White queenside */
        {"r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1",
                "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                encodeMove(getBoardIndex('e', '1'), getBoardIndex('c', '1'),
                           MOVE_FLAG_CASTLING)},

        /* Black kingside */
        {"r4rk1/8/8/8/8/8/8/R3K2R w KQ - 1 2",
                "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('e', '8'), getBoardIndex('g', '8'),
                           MOVE_FLAG_CASTLING)},

        /* Black queenside */
        {"2kr3r/8/8/8/8/8/8/R3K2R w KQ - 1 2",
                "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('e', '8'), getBoardIndex('c', '8'),
                           MOVE_FLAG_CASTLING)},

        /* ===== CASTLING RIGHTS LOST ===== */
        /* White king moves (lose both rights) */
        {"r3k2r/8/8/8/8/8/8/R2K3R b kq - 1 1",
                "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                encodeMove(getBoardIndex('e', '1'), getBoardIndex('d', '1'), 0)},

        /* Black king moves (lose both rights) */
        {"r2k3r/8/8/8/8/8/8/R3K2R w KQ - 1 2",
                "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
                encodeMove(getBoardIndex('e', '8'), getBoardIndex('d', '8'), 0)},

        /* White rook a1 moves (lose Q) */
        {"r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 1 1",
                "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                encodeMove(getBoardIndex('a', '1'), getBoardIndex('b', '1'), 0)},

        /* White rook h1 moves (lose K) */
        {"r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 1 1",
                "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                encodeMove(getBoardIndex('h', '1'), getBoardIndex('g', '1'), 0)},

        /* Black rook a8 captured (lose q) */
        {"R3k2r/8/8/8/8/8/8/4K2R b Kk - 0 1",
                "r3k2r/8/8/8/8/8/R7/4K2R w Kkq - 0 1",
                encodeMove(getBoardIndex('a', '2'), getBoardIndex('a', '8'),
                           MOVE_FLAG_CAPTURE)},

        /* Black rook h8 captured (lose k) */
        {"r3k2R/8/8/8/8/8/8/R3K3 b Qq - 0 1",
                "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
                encodeMove(getBoardIndex('h', '1'), getBoardIndex('h', '8'),
                           MOVE_FLAG_CAPTURE)},

        /* ===== PAWN DOUBLE MOVE (sets en passant) ===== */
        {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                encodeMove(getBoardIndex('e', '2'), getBoardIndex('e', '4'), 0)},

        {"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2",
                "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
                encodeMove(getBoardIndex('c', '7'), getBoardIndex('c', '5'), 0)},

        /* ===== HALFMOVE CLOCK ===== */
        /* Pawn move resets */
        {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                encodeMove(getBoardIndex('e', '2'), getBoardIndex('e', '4'), 0)},


};

void testMoveMaking() {
    TestResult testResults = {0, 0};

    std::cout << '\n';

    testMakeMove(testResults);
    testUndoMove(testResults);

    std::cout << "\nSummary: " << testResults.pass << "/" << (testResults.pass + testResults.fail)
              << " tests pass for Move Making and Undoing.\n";
}

void testMakeMove(TestResult &result) {
    Board board = Board();
    for (auto &t: allTests) {
        board.loadFenPosition(t.startingFen);
        board.makeMove(t.move);
        std::string r = board.generateFen();

        if (r == t.resultingFen) {
            result.pass++;
        } else {
            result.fail++;
            std::cout << "[FAIL] Move " << moveToString(t.move)
                      << " resulted in FEN: \n\"" << r
                      << "\" instead of \n\"" << t.resultingFen
                      << "\" Starting with FEN: \n\"" << t.startingFen
                      << "\"\n\n";
        }
    }
}

void testUndoMove(TestResult &result) {
    Board board = Board();
    for (auto &t: allTests) {
        board.loadFenPosition(t.startingFen);
        UndoInfo undoInfo = board.makeMove(t.move);
        board.unmakeMove(t.move, undoInfo);

        std::string r = board.generateFen();
        if (r == t.startingFen) {
            result.pass++;
        } else {
            result.fail++;
            std::cout << "[FAIL] UnmakeMove " << moveToString(t.move)
                      << " resulted in FEN: \n\"" << r
                      << "\" instead of \n\"" << t.resultingFen
                      << "\" Starting with FEN: \n\"" << t.startingFen
                      << "\"\n\n";
        }
    }
}
