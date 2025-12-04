//
// Created by ECanDo on 2025-08-23.
//

#ifndef CHESSENGINE_TEST_BOARD_H
#define CHESSENGINE_TEST_BOARD_H

struct TestResult {
    int fail;
    int pass;
};

void testBoard();

void testGetBoardIndexLetters(TestResult &results);

void testGetBoardIndexNumbers(TestResult &results);

void testGetBoardPosition(TestResult &results);

void testConvertFenString(TestResult &results);

#endif //CHESSENGINE_TEST_BOARD_H
