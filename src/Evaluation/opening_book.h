//
// Created by ECanDo on 2025-12-10.
//

#ifndef CHESSENGINE_OPENING_BOOK_H
#define CHESSENGINE_OPENING_BOOK_H

#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_map>
#include "Evaluation/opening_book.h"
#include "Board/move.h"
#include "Board/board.h"

#include "Board/move.h"

// 16 bits, just in case I have 255+ versions, 65k versions is not going to happen
const uint16_t OPENING_BOOK_VERSION = 1;

// Magic number: "BOOK" in ASCII
// B = 0x42, O = 0x4F, O = 0x4F, K = 0x4B
// Combined: 0x424F4F4B
// This identifies the file as an opening book
const uint32_t OPENING_BOOK_MAGIC = 0x424F4F4B;

struct BookHeader {
    uint32_t magic;         // Must be OPENING_BOOK_MAGIC (0x424F4F4B)
    uint16_t version;       // File format version
    uint16_t padding1;      // Align to 8 bytes
    uint32_t numPositions;  // How many positions are stored
    uint32_t padding2;      // Align to 16 bytes total

    BookHeader()
            : magic(OPENING_BOOK_MAGIC), version(OPENING_BOOK_VERSION), padding1(0), numPositions(0), padding2(0) {}
};

// Position header (16 bytes)
struct PositionHeader {
    uint64_t zobristKey;    // Hash of the position
    uint16_t numEntries;    // How many moves for this position
    uint16_t padding[3];    // Align to 16 bytes

    PositionHeader()
            : zobristKey(0), numEntries(0), padding{0, 0, 0} {}
};

// Single book entry (variable size, depends on Move type)
struct BookEntry {
    Move move;       // Your Move type (uint32_t = 4 bytes)
    uint8_t weight;  // 1-255, where 255 = best move
    uint8_t padding[3];  // Align to 8 bytes (if Move is 4 bytes)

    BookEntry() : move(0), weight(0), padding{0, 0, 0} {}

    BookEntry(Move m, uint8_t w) : move(m), weight(w), padding{0, 0, 0} {}
};

// In-memory representation of a position
struct BookPosition {
    uint64_t zobristKey;
    std::vector<BookEntry> entries;

    BookPosition() : zobristKey(0) {}
};

extern std::vector<BookPosition> OPENING_BOOK;
extern std::unordered_map<uint64_t, std::vector<BookEntry>> OPENING_BOOK_MAP;

void readBook(const std::string &filename);

void loadBookToHashMap(const std::string &filename);

Move lookupBookPosition(const Board &board);

std::vector<BookEntry> getAllBookMoves(const Board &board);

bool isInBook(const Board &board);

Move getWeightedRandomBookMove(const Board &board);


#endif //CHESSENGINE_OPENING_BOOK_H
