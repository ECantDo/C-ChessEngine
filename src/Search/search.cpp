//
// Created by ECanDo on 2025-12-06.
//


#include "search.h"

BestMove selectMove(Board board, long timeLimitMS) {
    std::vector<Move> moveList;
    generateLegalMoves(board, moveList);

    if (moveList.empty()) {
        return {0, 0};
    }


    Move bestMove = moveList[0];
    int bestMoveScore = evaluate(board, moveList[0]);

    for (int i = 1; i < moveList.size(); i++) {
        Move move = moveList[i];
        int score = evaluate(board, move);

        if (board.turn == 1){
            // White wants to maximise the score
            if (score > bestMoveScore){
                bestMoveScore = score;
                bestMove = move;
            }
        } else {
            // Black wants to minimise the score
            if (score < bestMoveScore){
                bestMoveScore = score;
                bestMove = move;
            }
        }
    }

    return {
            bestMove,
            bestMoveScore
    };
}

int evaluate(Board &board, Move &move) {
    UndoInfo ui = board.makeMove(move);

    int rawPieceValue = 0;

    // Loop over all pieces except the last two; the kings
    for (int i = 0; i < ALL_PIECES.size() - 2; i++) {
        char p = ALL_PIECES[i];
        rawPieceValue += countPiece(board, p) * getValue(p);
    }

    board.unmakeMove(move, ui);

    return rawPieceValue;
}

int countPiece(Board &board, char piece) {
    uint64_t bitBoard = board.getBitboard(piece);
    return std::popcount(bitBoard);
}