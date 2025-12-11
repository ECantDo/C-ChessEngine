//
// Created by ECanDo on 2025-12-10.
//

#include <iostream>
#include "readFenFile.h"
#include "buildBook.h"

int main() {
    std::string inputFile = R"(E:\CLionProjects\chessEngine\buildOpeningBook\openings.txt)";
    std::string outputFile = R"(E:\CLionProjects\chessEngine\buildOpeningBook\openingBook.bin)";
    std::vector<std::string> fens = readFENFile(inputFile);

    std::cout << "Loaded " << fens.size() << " FEN positions:\n";
//    for (const auto &fen: fens) {
//        std::cout << fen << "\n";
//    }

    return writeBook(fens, outputFile);
}