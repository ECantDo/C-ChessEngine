//
// Created by ECanDo on 2025-08-22.
//

// =====================================================================================================================
// Todos
// =====================================================================================================================
// TODO: Refactor "rank" and "file" (rank -> row -> 1-8, file -> a-h) to align with the game properly

#include "board.h"
#include "piece.h"
#include <iostream>
#include <sstream>

// =====================================================================================================================
// Constructors
// =====================================================================================================================

Board::Board()
        : whitePawns(0), whiteBishops(0), whiteKing(0), whiteKnights(0), whiteQueens(0), whiteRooks(0),
          blackPawns(0), blackBishops(0), blackKing(0), blackKnights(0), blackQueens(0), blackRooks(0),
          enPassantSquare(-1), turn(0), castling(0), halfMoveClock(0), fullMove(1) {}

Board::Board(std::string &fen) : Board() {
    if (!loadFenPosition(fen)) throw std::invalid_argument("Invalid FEN string: " + fen);
}

void Board::loadStartPosition() {
    // Pawns
    whitePawns = 0x000000000000FF00ULL;
    blackPawns = 0x00FF000000000000ULL;

    // Rooks
    whiteRooks = 0x0000000000000081ULL;
    blackRooks = 0x8100000000000000ULL;

    // Knights
    whiteKnights = 0x0000000000000042ULL;
    blackKnights = 0x4200000000000000ULL;

    // Bishops
    whiteBishops = 0x0000000000000024ULL;
    blackBishops = 0x2400000000000000ULL;

    // Queens
    whiteQueens = 0x0000000000000008ULL;
    blackQueens = 0x0800000000000000ULL;

    // Kings
    whiteKing = 0x0000000000000010ULL;
    blackKing = 0x1000000000000000ULL;
}

// =====================================================================================================================
// Class based helper functions
// =====================================================================================================================
char Board::pieceAtSquare(int square) const {
    uint64_t mask = 1ULL << square;

    if (whitePawns & mask) return WHITE_PAWN;
    if (blackPawns & mask) return BLACK_PAWN;

    if (whiteKnights & mask) return WHITE_KNIGHT;
    if (whiteBishops & mask) return WHITE_BISHOP;
    if (whiteRooks & mask) return WHITE_ROOK;
    if (whiteQueens & mask) return WHITE_QUEEN;

    if (blackKnights & mask) return BLACK_KNIGHT;
    if (blackBishops & mask) return BLACK_BISHOP;
    if (blackRooks & mask) return BLACK_ROOK;
    if (blackQueens & mask) return BLACK_QUEEN;

    if (whiteKing & mask) return WHITE_KING;
    if (blackKing & mask) return BLACK_KING;

    return NONE;
}

void Board::setPieceAtSquare(int square, char piece) {
    if (square < 0 || square >= 64) {
        return;
    }

    uint64_t mask = 1ULL << square;
    uint64_t invertedMask = ~mask;

    // Clear this square from all bitboards
    whitePawns &= invertedMask;
    whiteKnights &= invertedMask;
    whiteBishops &= invertedMask;
    whiteRooks &= invertedMask;
    whiteQueens &= invertedMask;
    whiteKing &= invertedMask;

    blackPawns &= invertedMask;
    blackKnights &= invertedMask;
    blackBishops &= invertedMask;
    blackRooks &= invertedMask;
    blackQueens &= invertedMask;
    blackKing &= invertedMask;

    // Get the right bitboard
    uint64_t *bitboard = getBitboardPointer(piece);

    // If bitboard is null; stop
    if (!bitboard) {
        return;
    }

    // Set the value in the right bitboard
    *bitboard |= mask;

}

// Print Board
void Board::printBoard() const {
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            std::cout << pieceAtSquare(square) << " ";
        }
        std::cout << "\n";
    }
}

uint64_t Board::getBitboard(char piece) const {
    switch (piece) {
        case WHITE_PAWN:
            return whitePawns;
        case WHITE_KNIGHT:
            return whiteKnights;
        case WHITE_ROOK:
            return whiteRooks;
        case WHITE_BISHOP:
            return whiteBishops;
        case WHITE_QUEEN:
            return whiteQueens;
        case WHITE_KING:
            return whiteKing;
        case BLACK_PAWN:
            return blackPawns;
        case BLACK_KNIGHT:
            return blackKnights;
        case BLACK_ROOK:
            return blackRooks;
        case BLACK_BISHOP:
            return blackBishops;
        case BLACK_QUEEN:
            return blackQueens;
        case BLACK_KING:
            return blackKing;
        default:
            return -1;
    }
}

uint64_t *Board::getBitboardPointer(char piece) {
    switch (piece) {
        case WHITE_PAWN:
            return &whitePawns;
        case WHITE_KNIGHT:
            return &whiteKnights;
        case WHITE_ROOK:
            return &whiteRooks;
        case WHITE_BISHOP:
            return &whiteBishops;
        case WHITE_QUEEN:
            return &whiteQueens;
        case WHITE_KING:
            return &whiteKing;
        case BLACK_PAWN:
            return &blackPawns;
        case BLACK_KNIGHT:
            return &blackKnights;
        case BLACK_ROOK:
            return &blackRooks;
        case BLACK_BISHOP:
            return &blackBishops;
        case BLACK_QUEEN:
            return &blackQueens;
        case BLACK_KING:
            return &blackKing;
        default:
            return nullptr;
    }
}

uint64_t Board::getWhiteBitboard() const {
    return whiteRooks | whiteQueens | whiteKing | whiteKnights | whitePawns | whiteBishops;
}

uint64_t Board::getBlackBitboard() const {
    return blackRooks | blackQueens | blackKing | blackKnights | blackPawns | blackBishops;
}


// =====================================================================================================================
// Load board position
// =====================================================================================================================
bool Board::loadFenPosition(std::string &fen) {
    Board newBoard;
    size_t idx = 0;

    /* ===== PART 1: Piece Placement ===== */
    int rank = 7;  /* Start from rank 8 (index 7) */
    int file = 0;  /* Start from file a (index 0) */

    while (idx < fen.size() && fen[idx] != ' ') {
        char ch = fen[idx++];

        if (ch == '/') {
            /* Move to next rank */
            if (file != 8) return false;  /* Previous rank wasn't complete */
            rank--;
            file = 0;
            continue;
        }

        if (ch >= '1' && ch <= '8') {
            /* Empty squares */
            int emptyCount = ch - '0';
            file += emptyCount;
            if (file > 8) return false;  /* Too many squares in rank */
            continue;
        }

        /* Must be a piece character */
        if (file >= 8) return false;  /* Too many pieces in rank */

        int square = rank * 8 + file;
        newBoard.setPieceAtSquare(square, ch);
        file++;
    }

    /* Verify we ended on rank 1 (index 0) with all 8 files */
    if (rank != 0 || file != 8) return false;

    /* ===== PART 2: Active Color ===== */
    if (idx >= fen.size() || fen[idx] != ' ') return false;
    idx++;  /* Skip space */

    if (idx >= fen.size()) return false;
    if (fen[idx] == 'w') {
        newBoard.turn = 1;
    } else if (fen[idx] == 'b') {
        newBoard.turn = -1;
    } else {
        return false;
    }
    idx++;

    /* ===== PART 3: Castling Rights ===== */
    if (idx >= fen.size() || fen[idx] != ' ') return false;
    idx++;  /* Skip space */

    if (idx >= fen.size()) return false;

    newBoard.castling = 0;
    if (fen[idx] == '-') {
        /* No castling rights */
        idx++;
    } else {
        /* Parse castling rights */
        while (idx < fen.size() && fen[idx] != ' ') {
            char ch = fen[idx++];
            switch (ch) {
                case 'K': newBoard.castling |= 0b1000; break;
                case 'Q': newBoard.castling |= 0b0100; break;
                case 'k': newBoard.castling |= 0b0010; break;
                case 'q': newBoard.castling |= 0b0001; break;
                default: return false;  /* Invalid castling character */
            }
        }
    }

    /* ===== PART 4: En Passant Square ===== */
    if (idx >= fen.size() || fen[idx] != ' ') return false;
    idx++;  /* Skip space */

    if (idx >= fen.size()) return false;

    if (fen[idx] == '-') {
        /* No en passant square */
        newBoard.enPassantSquare = -1;
        idx++;
    } else {
        /* Parse en passant square (e.g., "e3") */
        if (idx + 1 >= fen.size()) return false;

        char fileChar = fen[idx++];
        char rankChar = fen[idx++];

        if (fileChar < 'a' || fileChar > 'h') return false;
        if (rankChar < '1' || rankChar > '8') return false;

        int epFile = fileChar - 'a';
        int epRank = rankChar - '1';
        newBoard.enPassantSquare = epRank * 8 + epFile;
    }

    /* ===== PART 5: Halfmove Clock ===== */
    if (idx >= fen.size() || fen[idx] != ' ') return false;
    idx++;  /* Skip space */

    if (idx >= fen.size() || !isdigit(fen[idx])) return false;

    int halfmove = 0;
    while (idx < fen.size() && isdigit(fen[idx])) {
        halfmove = halfmove * 10 + (fen[idx++] - '0');
    }
    newBoard.halfMoveClock = halfmove;

    /* ===== PART 6: Fullmove Number ===== */
    if (idx >= fen.size() || fen[idx] != ' ') return false;
    idx++;  /* Skip space */

    if (idx >= fen.size() || !isdigit(fen[idx])) return false;

    int fullmove = 0;
    while (idx < fen.size() && isdigit(fen[idx])) {
        fullmove = fullmove * 10 + (fen[idx++] - '0');
    }

    if (fullmove < 1) return false;  /* Fullmove must be at least 1 */
    newBoard.fullMove = fullmove;

    /* ===== Success - Update Board ===== */
    *this = newBoard;
    return true;
}

// =====================================================================================================================
// Output FEN
// =====================================================================================================================
std::string Board::generateFen() const {
    std::stringstream fen;

    // 1. Piece placement
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            int idx = file + rank * 8;
            char pieceChar = pieceAtSquare(idx);
            if (pieceChar == NONE) {
                ++empty;
            } else {
                if (empty > 0) {
                    fen << empty;
                    empty = 0;
                }
                fen << pieceChar;
            }
        }
        if (empty > 0) fen << empty;
        if (rank > 0) fen << '/';
    }

    // 2. Active color
    fen << ' ' << (turn == 1 ? 'w' : 'b');

    // 3. Castling rights
    fen << ' ';
    bool hasCastling = false;
    if (castling & 0b1000) {
        fen << 'K';
        hasCastling = true;
    }
    if (castling & 0b0100) {
        fen << 'Q';
        hasCastling = true;
    }
    if (castling & 0b0010) {
        fen << 'k';
        hasCastling = true;
    }
    if (castling & 0b0001) {
        fen << 'q';
        hasCastling = true;
    }
    if (!hasCastling) fen << '-';

    // 4. En passant
    fen << ' ';
    if (enPassantSquare >= 0 && enPassantSquare < 64) {
        int file = enPassantSquare % 8;
        int rank = enPassantSquare / 8;
        fen << (char) ('a' + file) << (char) ('1' + rank);
    } else {
        fen << '-';
    }

    // 5. Halfmove clock
    fen << ' ' << halfMoveClock;

    // 6. Fullmove number
    fen << ' ' << fullMove;

    return fen.str();
}
//======================================================================================================================
// Non-class helper functions
//======================================================================================================================

int getBoardIndex(int file, int rank) {
    if (file < 0 || file > 7 || rank < 0 || rank > 7)
        return -1;
    return file * 8 + rank;
}

int getBoardIndex(char file, char rank) {
    return getBoardIndex(file - 'a', rank - '1');
}

std::string getBoardPosition(int index) {
    if (index < 0 || index >= 64) return "";

    int file = index >> 3; // index / 8;
    int rank = index & 7; // index % 8;

    return std::string()
           + static_cast<char>(file + 'a')
           + static_cast<char>(rank + '1');
}

// =====================================================================================================================
// Move making
// =====================================================================================================================