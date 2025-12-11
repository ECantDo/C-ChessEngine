//
// Created by ECanDo on 2025-12-10.
//

#include <iostream>
#include <fstream>
#include "opening_book.h"

std::vector<BookPosition> readBook(const std::string &filename) {
    std::vector<BookPosition> positions;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Could not open book file: " << filename << std::endl;
        return positions;
    }


    BookHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != 0x424F4F4B) {
        std::cerr << "Invalid book file (bad magic number)" << std::endl;
        return positions;
    }

    std::cout << "Reading book version " << header.version
              << " with " << header.numPositions << " positions..." << std::endl;

    positions.reserve(header.numPositions);

    // Read each position
    for (uint32_t i = 0; i < header.numPositions; i++) {

        PositionHeader posHeader;
        file.read(reinterpret_cast<char*>(&posHeader), sizeof(posHeader));

        BookPosition pos;
        pos.zobristKey = posHeader.zobristKey;
        pos.entries.resize(posHeader.numEntries);

        if (posHeader.numEntries > 0) {
            file.read(reinterpret_cast<char*>(pos.entries.data()),
                      posHeader.numEntries * sizeof(BookEntry));
        }

        positions.push_back(std::move(pos));
    }

    file.close();
    return positions;
}
