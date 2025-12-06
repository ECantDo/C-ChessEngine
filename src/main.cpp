#include "Board/board.h"
#include <iostream>

int main() {
    std::string fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";

    Board board = Board();
    board.loadStartPosition();
    board.printBoard();
    std::cout << board.generateFen() << '\n';
    std::cout << fen << '\n';
    return 0;
}