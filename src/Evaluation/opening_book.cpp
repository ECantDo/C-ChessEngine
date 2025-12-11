//
// Created by ECanDo on 2025-12-10.
//

#include "opening_book.h"
#include <random>

std::vector<BookPosition> OPENING_BOOK;
std::unordered_map<uint64_t, std::vector<BookEntry>> OPENING_BOOK_MAP;

void readBook(const std::string &filename) {
    std::vector<BookPosition> positions;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Could not open book file: " << filename << std::endl;
        OPENING_BOOK = positions;
        return;  // Must return here!
    }

    BookHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (header.magic != OPENING_BOOK_MAGIC) {
        std::cerr << "Invalid book file (bad magic number: 0x"
                  << std::hex << header.magic << std::dec << ")" << std::endl;
        file.close();
        OPENING_BOOK = positions;
        return;  // Must return here!
    }

//    if (!isCompatibleVersion(header.version)) {
//        std::cerr << "Incompatible book version " << header.version << std::endl;
//        std::cerr << "Expected version " << OPENING_BOOK_VERSION << std::endl;
//        file.close();
//        OPENING_BOOK = positions;
//        return;  // Must return here!
//    }

    std::cout << "Reading opening book version " << header.version
              << " with " << header.numPositions << " positions..." << std::endl;

    positions.reserve(header.numPositions);

    // Read each position
    for (uint32_t i = 0; i < header.numPositions; i++) {
        PositionHeader posHeader;
        file.read(reinterpret_cast<char *>(&posHeader), sizeof(posHeader));

        BookPosition pos;
        pos.zobristKey = posHeader.zobristKey;
        pos.entries.resize(posHeader.numEntries);

        if (posHeader.numEntries > 0) {
            file.read(reinterpret_cast<char *>(pos.entries.data()),
                      posHeader.numEntries * sizeof(BookEntry));
        }

        positions.push_back(std::move(pos));
    }

    file.close();
    OPENING_BOOK = positions;

    std::cout << "Successfully loaded " << positions.size() << " positions." << std::endl;
}

// Load book into a hash map for fast lookup during games
void loadBookToHashMap(const std::string &filename) {
    readBook(filename);

    if (OPENING_BOOK.empty()) {
        std::cerr << "Warning: Opening book is empty!" << std::endl;
        return;
    }

    for (auto &pos: OPENING_BOOK) {
        OPENING_BOOK_MAP[pos.zobristKey] = std::move(pos.entries);
    }

    std::cout << "Loaded " << OPENING_BOOK_MAP.size() << " positions into hash map." << std::endl;
}

// Look up a specific position and return a book move
Move lookupBookPosition(const Board &board) {
    // Lazy load: only load book if not already loaded
    if (OPENING_BOOK_MAP.empty()) {
        loadBookToHashMap("./openingBook.bin");
    }

    // Still empty after trying to load? No book available
    if (OPENING_BOOK_MAP.empty()) {
        return 0;  // No book move available
    }

    uint64_t zobristKey = board.zobristHash;

    auto it = OPENING_BOOK_MAP.find(zobristKey);

    // Position not in book
    if (it == OPENING_BOOK_MAP.end()) {
        return 0;  // No book move for this position
    }

    const auto &entries = it->second;

    // No moves available (shouldn't happen, but safety check)
    if (entries.empty()) {
        return 0;
    }

    // Strategy 1: Always pick the best move (highest weight)
    return entries[0].move;
}

// Alternative: Get all book moves for a position (useful for debugging or UI)
std::vector<BookEntry> getAllBookMoves(const Board &board) {
    if (OPENING_BOOK_MAP.empty()) {
        loadBookToHashMap("./openingBook.bin");
    }

    uint64_t zobristKey = board.zobristHash;
    auto it = OPENING_BOOK_MAP.find(zobristKey);

    if (it == OPENING_BOOK_MAP.end()) {
        return {};  // Empty vector
    }

    return it->second;
}

// Check if a position is in the opening book
bool isInBook(const Board &board) {
    if (OPENING_BOOK_MAP.empty()) {
        loadBookToHashMap("./openingBook.bin");
    }

    uint64_t zobristKey = board.zobristHash;
    return OPENING_BOOK_MAP.find(zobristKey) != OPENING_BOOK_MAP.end();
}

// Weighted random move selection
Move getWeightedRandomBookMove(const Board &board) {
    auto entries = getAllBookMoves(board);

    if (entries.empty()) {
        return 0;
    }

    // Calculate total weight
    int totalWeight = 0;
    for (const auto &entry : entries) {
        totalWeight += entry.weight;
    }

    // Random selection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, totalWeight - 1);
    int random = dis(gen);

    // Find corresponding move
    int currentWeight = 0;
    for (const auto &entry : entries) {
        currentWeight += entry.weight;
        if (random < currentWeight) {
            return entry.move;
        }
    }

    return entries[0].move;  // Fallback
}