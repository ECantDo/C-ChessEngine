//
// Created by ECanDo on 2025-12-06.
//

#include <cmath>
#include "search.h"

BestMove selectMove(Board board, long timeLimitMS) {
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList);


    if (moveList.empty()) {
        return {"0000", 0};
    }

    return {
            moveToString(moveList[rand() % moveList.size()]),
            0
    };
}