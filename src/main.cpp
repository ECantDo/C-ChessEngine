#include "Board/board.h"
#include <iostream>

int main() {
//    std::string fen = "7k/3N2qp/b5r1/2p1Q1N1/Pp4PK/7P/1P3p2/6r1 w - - 7 4";

    Board board = Board();
    board.loadStartPosition();
    board.printBoard();
    std::cout << board.generateFen() << '\n';
//    std::cout << fen << '\n';
    return 0;
}