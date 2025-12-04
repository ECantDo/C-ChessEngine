//
// Created by ECanDo on 2025-12-04.
//

#ifndef CHESSENGINE_TEST_MOVES_H
#define CHESSENGINE_TEST_MOVES_H

#include <iostream>
#include <vector>

#include "../testing_essentials.h"
#include "Board/board.h"


void testMoveMaking();

void testMakeMove(TestResult &result);

void testUndoMove(TestResult &result);

#endif //CHESSENGINE_TEST_MOVES_H
