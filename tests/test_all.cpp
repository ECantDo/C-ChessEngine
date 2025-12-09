//
// Created by ECanDo on 2025-08-23.
//

#include "Board/move.h"
#include "Board/test_board.h"
#include "Board/test_moves.h"
#include "MoveGeneration/test_move_generation.h"
#include "Board/test_zobrist.h"


#include <chrono>

int main() {

    testBoard();
    testMoveMaking();

    testZobrist();

//    std::cout << "\nTESTING MOVE GENERATION\n";
//    auto start = std::chrono::high_resolution_clock::now();
//    testMoveGeneration(5);
//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//    std::cout << "Took " << duration.count() << " ms\n";

//    perftDivideTesting();

    //todo: Put into it's own file

    /* Test repetition detection */
    Board board;
    board.loadStartPosition();

    /* Play moves that repeat */
    board.gameHistory.push_back(board.zobristHash);
    board.makeMove(stringToMove((std::string &) "g1f3", board));  /* Nf3 */

    board.gameHistory.push_back(board.zobristHash);
    board.makeMove(stringToMove((std::string &) "g8f6", board));  /* Nf6 */

    board.gameHistory.push_back(board.zobristHash);
    board.makeMove(stringToMove((std::string &) "f3g1", board));  /* Ng1 */

    board.gameHistory.push_back(board.zobristHash);
    board.makeMove(stringToMove((std::string &) "f6g8", board));  /* Ng8 */

    /* Repeat one more time */
    board.gameHistory.push_back(board.zobristHash);
    board.makeMove(stringToMove((std::string &) "g1f3", board));

    board.gameHistory.push_back(board.zobristHash);
    board.makeMove(stringToMove((std::string &) "g8f6", board));

    std::cout << "Is draw? " << (board.isDraw() ? "YES" : "NO") << std::endl;
    /* Should print YES after 3rd repetition */

    return 0;
}