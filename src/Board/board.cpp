//
// Created by ECanDo on 2025-08-22.
//

// =====================================================================================================================
// Todos
// =====================================================================================================================
// TODO: Refactor "rank" and "file" (rank -> row -> 1-8, file -> a-h) to align with the game properly

#include "board.h"


// =====================================================================================================================
// Constructors
// =====================================================================================================================

Board::Board()
        : whitePawns(0), whiteBishops(0), whiteKing(0), whiteKnights(0), whiteQueens(0), whiteRooks(0),
          blackPawns(0), blackBishops(0), blackKing(0), blackKnights(0), blackQueens(0), blackRooks(0),
          enPassantSquare(-1), turn(0), castling(0), halfMoveClock(0), fullMove(1) { loadStartPosition(); }

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

    fullMove = 1;
    halfMoveClock = 0;
    castling = 0b1111;
    turn = 1;
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

    return NONE_PIECE;
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
    // Pawns
    newBoard.whitePawns = 0;
    newBoard.blackPawns = 0;
    newBoard.whiteRooks = 0;
    newBoard.blackRooks = 0;
    newBoard.whiteKnights = 0;
    newBoard.blackKnights = 0;
    newBoard.whiteBishops = 0;
    newBoard.blackBishops = 0;
    newBoard.whiteQueens = 0;
    newBoard.blackQueens = 0;
    newBoard.whiteKing = 0;
    newBoard.blackKing = 0;
    newBoard.fullMove = 1;
    newBoard.halfMoveClock = 0;
    newBoard.castling = 0b1111;
    newBoard.turn = 1;


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
                case 'K':
                    newBoard.castling |= 0b1000;
                    break;
                case 'Q':
                    newBoard.castling |= 0b0100;
                    break;
                case 'k':
                    newBoard.castling |= 0b0010;
                    break;
                case 'q':
                    newBoard.castling |= 0b0001;
                    break;
                default:
                    return false;  /* Invalid castling character */
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
            if (pieceChar == NONE_PIECE) {
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

/* Takes numeric file (0-7) and rank (0-7) */
int getBoardIndex(int file, int rank) {
    if (file < 0 || file > 7 || rank < 0 || rank > 7)
        return -1;
    return rank * 8 + file;  /* RANK times 8, plus FILE */
}

/* Takes algebraic notation like 'b' and '7' */
int getBoardIndex(char file, char rank) {
    return getBoardIndex(file - 'a', rank - '1');
}

std::string getBoardPosition(int index) {
    if (index < 0 || index >= 64) return "";

    int rank = index >> 3; // index / 8;
    int file = index & 7; // index % 8;

    return std::string()
           + static_cast<char>(file + 'a')
           + static_cast<char>(rank + '1');
}

// =====================================================================================================================
// Move making
// =====================================================================================================================
UndoInfo Board::makeMove(Move m) {
    UndoInfo undoInfo = {0, 0, 0, 0};

    int fromLocation = getMoveFrom(m);
    int toLocation = getMoveTo(m);
    int flags = getMoveFlags(m);

    char thisPiece = pieceAtSquare(fromLocation);
    char capturedPiece = pieceAtSquare(toLocation);

    // Save undo info
    undoInfo.halfMoveClock = halfMoveClock;
    undoInfo.enPassantSquare = enPassantSquare;
    undoInfo.castlingRights = castling;

    // Move the piece
    setPieceAtSquare(toLocation, thisPiece);
    setPieceAtSquare(fromLocation, NONE_PIECE);

    // Handle Captures
    if (flags & MOVE_FLAG_EN_PASSANT) {
        int capturedPawnSquare = toLocation + (turn == 1 ? -8 : 8);
        undoInfo.capturedPiece = (turn == 1 ? BLACK_PAWN : WHITE_PAWN);
        setPieceAtSquare(capturedPawnSquare, NONE_PIECE);
    } else {
        undoInfo.capturedPiece = capturedPiece;  /* Regular capture */
    }
    // Other captures should already be natively handled.

    // Handle Castling
    if (flags & MOVE_FLAG_CASTLING && thisPiece == WHITE_KING) {
        if (toLocation == 6) { // King side
            setPieceAtSquare(5, WHITE_ROOK);
            setPieceAtSquare(7, NONE_PIECE);
        } else if (toLocation == 2) { // Queen side
            setPieceAtSquare(3, WHITE_ROOK);
            setPieceAtSquare(0, NONE_PIECE);
        }
        castling &= 0b0011; // Remove white right to castle
    } else if (flags & MOVE_FLAG_CASTLING && thisPiece == BLACK_KING) {
        if (toLocation == 62) {
            setPieceAtSquare(61, BLACK_ROOK);
            setPieceAtSquare(63, NONE_PIECE);
        } else if (toLocation == 58) {
            setPieceAtSquare(59, BLACK_ROOK);
            setPieceAtSquare(56, NONE_PIECE);
        }
        castling &= 0b1100; // Remove black right to castle
    }

    // Handle Promotion
    if (flags & MOVE_FLAG_PROMOTION) {
        int promotionPiece = flags & 0x3;
        char newPiece;
        if (turn == 1) {
            switch (promotionPiece) {
                case PROMOTE_TO_KNIGHT:
                    newPiece = WHITE_KNIGHT;
                    break;
                case PROMOTE_TO_BISHOP:
                    newPiece = WHITE_BISHOP;
                    break;
                case PROMOTE_TO_QUEEN:
                    newPiece = WHITE_QUEEN;
                    break;
                case PROMOTE_TO_ROOK:
                    newPiece = WHITE_ROOK;
                    break;
                default:
                    newPiece = NONE_PIECE;
            }
        } else {
            switch (promotionPiece) {
                case PROMOTE_TO_KNIGHT:
                    newPiece = BLACK_KNIGHT;
                    break;
                case PROMOTE_TO_BISHOP:
                    newPiece = BLACK_BISHOP;
                    break;
                case PROMOTE_TO_QUEEN:
                    newPiece = BLACK_QUEEN;
                    break;
                case PROMOTE_TO_ROOK:
                    newPiece = BLACK_ROOK;
                    break;
                default:
                    newPiece = NONE_PIECE;
            }
        }
        setPieceAtSquare(toLocation, newPiece);
    }

    // Update Castling Rights
    if (thisPiece == WHITE_ROOK) {
        if (fromLocation == 0) {
            castling &= ~0b0100;    // Remove queen side rights
        } else if (fromLocation == 7) {
            castling &= ~0b1000;    // Remove king side rights
        }
    } else if (thisPiece == BLACK_ROOK) {
        if (fromLocation == 56) {
            castling &= ~0b0001; // Queen side
        } else if (fromLocation == 63) {
            castling &= ~0b0010; // King side
        }
    } else if (thisPiece == WHITE_KING) {
        castling &= ~0b1100; // Remove white rights on king move
    } else if (thisPiece == BLACK_KING) {
        castling &= ~0b0011; // Remove black rights on king move
    }

    if (capturedPiece == WHITE_ROOK) {
        if (toLocation == 0) {
            castling &= ~0b0100; // Remove white rights on queen side
        }
        if (toLocation == 7) {
            castling &= ~0b1000; // Remove white rights on king side
        }
    } else if (capturedPiece == BLACK_ROOK) {
        if (toLocation == 56) {
            castling &= ~0b0001; // Remove black rights on queen side
        }
        if (toLocation == 63) {
            castling &= ~0b0010; // Remove black rights on king side
        }
    }

    // Update en passant square
    enPassantSquare = -1;
    if ((thisPiece == WHITE_PAWN && fromLocation / 8 == 1 && toLocation / 8 == 3) ||
        (thisPiece == BLACK_PAWN && fromLocation / 8 == 6 && toLocation / 8 == 4)) {
        enPassantSquare = (fromLocation + toLocation) / 2; // Square the pawn passed over
    }

    // Update half-move clock
    if (thisPiece == WHITE_PAWN || thisPiece == BLACK_PAWN || capturedPiece != NONE_PIECE) {
        halfMoveClock = 0;
    } else {
        halfMoveClock += 1;
    }

    // Update Turn
    turn = (int8_t) -turn;

    // Update full-move number
    if (turn == 1) { // Just switched to white, black just moved, therefore full move
        fullMove += 1;
    }

    return undoInfo;
}

void Board::unmakeMove(Move m, const UndoInfo &undoInfo) {
    int fromLocation = getMoveFrom(m);
    int toLocation = getMoveTo(m);
    int flags = getMoveFlags(m);

    /* Flip turn back first */
    turn = (int8_t) -turn;

    /* Decrement fullmove if we're back to black's turn */
    if (turn == -1) {
        fullMove -= 1;
    }

    /* Get the piece at destination (might be promoted piece) */
    char piece = pieceAtSquare(toLocation);

    /* If it was a promotion, restore the pawn */
    if (flags & MOVE_FLAG_PROMOTION) {
        piece = (turn == 1) ? WHITE_PAWN : BLACK_PAWN;
    }

    /* Move piece back */
    setPieceAtSquare(fromLocation, piece);
    setPieceAtSquare(toLocation, undoInfo.capturedPiece);

    /* Undo en passant capture */
    if (flags & MOVE_FLAG_EN_PASSANT) {
        int capturedPawnSquare = toLocation + (turn == 1 ? -8 : 8);
        setPieceAtSquare(capturedPawnSquare, undoInfo.capturedPiece);
        setPieceAtSquare(toLocation, NONE_PIECE);
    }

    /* Undo castling */
    if (flags & MOVE_FLAG_CASTLING) {
        if (toLocation == 6) {  /* White kingside */
            setPieceAtSquare(7, WHITE_ROOK);
            setPieceAtSquare(5, NONE_PIECE);
        } else if (toLocation == 2) {  /* White queenside */
            setPieceAtSquare(0, WHITE_ROOK);
            setPieceAtSquare(3, NONE_PIECE);
        } else if (toLocation == 62) {  /* Black kingside */
            setPieceAtSquare(63, BLACK_ROOK);
            setPieceAtSquare(61, NONE_PIECE);
        } else if (toLocation == 58) {  /* Black queenside */
            setPieceAtSquare(56, BLACK_ROOK);
            setPieceAtSquare(59, NONE_PIECE);
        }
    }

    /* Restore state */
    halfMoveClock = undoInfo.halfMoveClock;
    enPassantSquare = undoInfo.enPassantSquare;
    castling = undoInfo.castlingRights;
}