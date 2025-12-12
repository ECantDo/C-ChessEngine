//
// Created by ECanDo on 2025-12-10.
//

#include <fstream>
#include "buildBook.h"
#include "moveWeighting.h"
#include "Evaluation/opening_book.h"

BookPosition getBookPosition(std::string &fen, WeightingConfig &config, StockfishProcess &sf) {
    Board board = Board(fen);

    // Get a vector
    std::vector<BookEntry> bookEntries;
    bookEntries.reserve(10);

    BookPosition bookPosition;
    bookPosition.zobristKey = board.zobristHash;

    std::vector<PVLine> topMoves = getTopMoves(sf, fen, 15, 10, false);
    int bestScore = topMoves[0].scoreCp; // Will always have a best move

    for (int i = 0; i < topMoves.size(); i++) {
        PVLine pv = topMoves[i];
        uint8_t weight = calculateWeight(bestScore, pv.scoreCp, i + 1, config);

        if (weight == 0) {
            break; // All further weights are also going to be 0
        }

        Move m = stringToMove(pv.move, board);
//        std::cout << "(" << moveToString(m) << ", " << pv.move << ") ";
        bookEntries.emplace_back(m, weight);
    }
    std::cout << std::endl << std::flush;
    bookPosition.entries = bookEntries;

    return bookPosition;
}

int writeBook(std::vector<std::string> &fenPositions, std::string &outputFilename) {
    StockfishProcess sf;

    if (!sf.start("E:\\Chess Stuff\\STOCKFISH\\stockfish-windows-x86-64-sse41-popcnt\\"
                  "stockfish\\stockfish-windows-x86-64-sse41-popcnt.exe")) {
        std::cerr << "Failed to launch Stockfish." << std::endl;
        return 1;
    }

    std::ofstream file(outputFilename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open output file: " << outputFilename << std::endl;
        return 1;
    }

    // Set up the Config
    WeightingConfig config;
    config.useTop4Only = false;
    config.maxScoreDiff = 25;

    std::vector<BookPosition> bookPositions;
    bookPositions.reserve(fenPositions.size());

    std::cout << "Processing " << fenPositions.size() << " positions..." << std::endl;

    for (size_t i = 0; i < fenPositions.size(); i++) {
        std::string &fen = fenPositions[i];

        std::cout << "[" << (i + 1) << "/" << fenPositions.size() << "] " << std::flush;

        BookPosition bookPosition = getBookPosition(fen, config, sf);
        if (bookPosition.entries.empty()) {
            std::cout << "Skipped (no valid moves)" << std::endl;
            continue;
        }
        std::cout << "OK (" << bookPosition.entries.size() << " moves)" << std::endl;
        bookPositions.push_back(bookPosition);
    }

    // Done what we need to with stockfish
    sf.stop();

    std::cout << "Writing to book..." << std::endl;

    BookHeader header;
    header.numPositions = static_cast<uint32_t>(bookPositions.size());

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));

    for (const BookPosition &pos: bookPositions) {
        PositionHeader posHeader;
        posHeader.zobristKey = pos.zobristKey;
        posHeader.numEntries = static_cast<uint16_t>(pos.entries.size());

        file.write(reinterpret_cast<const char *>(&posHeader), sizeof(posHeader));

        for (BookEntry be: pos.entries) {
//            std::cout << "BE: (m." << be.move << " w." << ((uint8_t) be.weight) << ")  ";
            file.write(reinterpret_cast<const char *>(&be), sizeof(BookEntry));
        }

        std::cout << std::endl << std::flush;

        // Empty positions will not be considered as they are not added to the list (see above fen loop)
//        file.write(reinterpret_cast<const char *>(pos.entries.data()),
//                   pos.entries.size() * sizeof(BookEntry));

    }

    file.close();

    std::cout << "\n=== Opening Book Created ===" << std::endl;
    std::cout << "File: " << outputFilename << std::endl;
    std::cout << "Positions: " << bookPositions.size() << std::endl;
    std::cout << "Total moves: " << [&]() {
        size_t total = 0;
        for (const auto &p: bookPositions) total += p.entries.size();
        return total;
    }() << std::endl;

    // Get actual file size
    std::ifstream sizeCheck(outputFilename, std::ios::binary | std::ios::ate);
    size_t actualSize = sizeCheck.tellg();
    sizeCheck.close();

    std::cout << "File size: " << actualSize << " bytes ("
              << (actualSize / 1024.0) << " KB)" << std::endl;
    return 0;
}