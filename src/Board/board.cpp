//
// Created by ECanDo on 2025-08-22.
//

// =====================================================================================================================
// Todos
// =====================================================================================================================

#include <cstring>
#include "board.h"
#include "Moves/generate_moves.h"


// =====================================================================================================================
// Constructors
// =====================================================================================================================

Board::Board()
		: whitePawns(0), whiteBishops(0), whiteKing(0), whiteKnights(0), whiteQueens(0), whiteRooks(0),
		  blackPawns(0), blackBishops(0), blackKing(0), blackKnights(0), blackQueens(0), blackRooks(0),
		  enPassantSquare(-1), turn(0), castling(0), halfMoveClock(0), fullMove(1), zobristHash(0),
		  pieceAtSquareArray() {
	loadStartPosition();
	Board::initCastlingTable();
	zobristHash = computeZobristHash();
}

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

	initPieceArrayFromBitboards();
}

void Board::initPieceArrayFromBitboards() {
	// Clear the array first
	memset(pieceAtSquareArray, NONE, sizeof(pieceAtSquareArray));

	// Set pieces based on bitboards
	for (int sq = 0; sq < 64; ++sq) {
		uint64_t mask = 1ULL << sq;

		if (whitePawns & mask) {
			pieceAtSquareArray[sq] = WHITE_PAWN;
		} else if (blackPawns & mask) {
			pieceAtSquareArray[sq] = BLACK_PAWN;
		} else if (whiteKnights & mask) {
			pieceAtSquareArray[sq] = WHITE_KNIGHT;
		} else if (blackKnights & mask) {
			pieceAtSquareArray[sq] = BLACK_KNIGHT;
		} else if (whiteBishops & mask) {
			pieceAtSquareArray[sq] = WHITE_BISHOP;
		} else if (blackBishops & mask) {
			pieceAtSquareArray[sq] = BLACK_BISHOP;
		} else if (whiteRooks & mask) {
			pieceAtSquareArray[sq] = WHITE_ROOK;
		} else if (blackRooks & mask) {
			pieceAtSquareArray[sq] = BLACK_ROOK;
		} else if (whiteQueens & mask) {
			pieceAtSquareArray[sq] = WHITE_QUEEN;
		} else if (blackQueens & mask) {
			pieceAtSquareArray[sq] = BLACK_QUEEN;
		} else if (whiteKing & mask) {
			pieceAtSquareArray[sq] = WHITE_KING;
		} else if (blackKing & mask) {
			pieceAtSquareArray[sq] = BLACK_KING;
		}
	}
}

// =====================================================================================================================
// Class based helper functions
// =====================================================================================================================

// Promotion piece lookup: [isWhite][promoType] -> piece
static const Piece PROMO_PIECES[2][4] = {
		{BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN}, // Black (turn = -1)
		{WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN}  // White (turn = 1)
};

// Castling rights removal: [piece type][square] -> rights to remove
// Initialize this in your Board constructor or init function
static uint8_t CASTLING_REMOVE[4][64];

// Call this once at program startup
void Board::initCastlingTable() {
	memset(CASTLING_REMOVE, 0, sizeof(CASTLING_REMOVE));

	// White king removes white castling rights from any square
	for (int sq = 0; sq < 64; sq++) {
		CASTLING_REMOVE[0][sq] = 0b1100;
	}

	// Black king removes black castling rights from any square
	for (int sq = 0; sq < 64; sq++) {
		CASTLING_REMOVE[1][sq] = 0b0011;
	}

	// White rooks
	CASTLING_REMOVE[2][0] = 0b0100;  // Queen side
	CASTLING_REMOVE[2][7] = 0b1000;  // King side

	// Black rooks
	CASTLING_REMOVE[3][56] = 0b0001; // Queen side
	CASTLING_REMOVE[3][63] = 0b0010; // King side
}

inline Piece getPromotedPiece(int flags, int turn) {
	int promoType = flags & 0x3;
	return PROMO_PIECES[turn == 1 ? 1 : 0][promoType];
}


Piece Board::pieceAtSquare(int square) const {
	if (square < 0 || square >= 64) return NONE;
	return pieceAtSquareArray[square];
}

void Board::addPieceAtSquare(int square, Piece piece) {
	if (square < 0 || square >= 64) {
		return;
	}

	uint64_t mask = 1ULL << square;


	// Get the right bitboard
	uint64_t *bitboard = getBitboardPointer(piece);

	// If bitboard is null; stop
	if (!bitboard) {
		return;
	}

	// Set the value in the right bitboard
	*bitboard |= mask;

	pieceAtSquareArray[square] = piece;
}

void Board::removePieceAtSquare(int square, Piece removePiece) {
	if (square < 0 || square >= 64) {
		return;
	}
	uint64_t *bitboard;

	uint64_t setMask = 1ULL << square;
	uint64_t clearMask = ~setMask;

	// Remove the existingPiece (if it exists)
	if (removePiece != NONE) {
		bitboard = getBitboardPointer(removePiece);
		*bitboard &= clearMask;
		pieceAtSquareArray[square] = NONE;
	}
}

// Print Board
void Board::printBoard() const {
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			int square = rank * 8 + file;
			std::cout << pieceToChar(pieceAtSquare(square)) << " ";
		}
		std::cout << "\n";
	}
}

uint64_t Board::getBitboard(Piece piece) const {
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

uint64_t *Board::getBitboardPointer(Piece piece) {
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
	newBoard.enPassantSquare = -1;
	newBoard.zobristHash = 0;
	memset(newBoard.pieceAtSquareArray, NONE, sizeof(newBoard.pieceAtSquareArray));


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
		Piece p = charToPiece(ch);
		newBoard.addPieceAtSquare(square, p);
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
	newBoard.zobristHash = newBoard.computeZobristHash();

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
			Piece piece = pieceAtSquare(idx);
			if (piece == NONE) {
				++empty;
			} else {
				if (empty > 0) {
					fen << empty;
					empty = 0;
				}
				fen << pieceToChar(piece);
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
	const int fromLocation = getMoveFrom(m);
	const int toLocation = getMoveTo(m);
	const int flags = getMoveFlags(m);

	// CACHE ALL LOOKUPS UP FRONT
	const Piece thisPiece = pieceAtSquare(fromLocation);
	const Piece capturedPiece = pieceAtSquare(toLocation);
	const bool isCapture = (flags & MOVE_FLAG_CAPTURE) != 0;
	const bool isPromotion = (flags & MOVE_FLAG_PROMOTION) != 0;
	const bool isCastling = (flags & MOVE_FLAG_CASTLING) != 0;
	const bool isEnPassant = (flags & MOVE_FLAG_EN_PASSANT) != 0;

	// Save undo info
	UndoInfo undoInfo = {
			.capturedPiece = isEnPassant
							 ? (turn == 1 ? BLACK_PAWN : WHITE_PAWN)
							 : capturedPiece,
			.enPassantSquare = (int8_t) enPassantSquare,
			.castlingRights = castling,
			.halfMoveClock = (uint8_t) halfMoveClock,
			.zobristHash = zobristHash,
	};

	// ========== UPDATE ZOBRIST HASH (Part 1: Removals) ==========

	// Remove piece from source square
	zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(thisPiece)][fromLocation];

	// Remove old castling rights
	zobristHash ^= Zobrist::castlingRights[castling];

	// Remove old en passant
	if (enPassantSquare >= 0) {
		zobristHash ^= Zobrist::enPassantFile[enPassantSquare & 0x7];
	}

	// Remove captured piece
	if (isEnPassant) {
		int offset;
		Piece remove;
		if (turn == 1) {
			offset = -8;
			remove = BLACK_PAWN;
		} else {
			offset = 8;
			remove = WHITE_PAWN;
		}

		int capturedPawnSquare = toLocation + offset;
		zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(undoInfo.capturedPiece)][capturedPawnSquare];
		removePieceAtSquare(capturedPawnSquare, remove);
	} else if (isPiece(capturedPiece)) {
		zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(capturedPiece)][toLocation];
		removePieceAtSquare(toLocation, capturedPiece);
	}

	// ========== MOVE THE PIECE ==========

	Piece finalPiece = thisPiece;
	if (isPromotion) {
		finalPiece = getPromotedPiece(flags, turn);
	}

	removePieceAtSquare(fromLocation, thisPiece);
	addPieceAtSquare(toLocation, finalPiece);

	// ========== HANDLE CASTLING ==========

	if (isCastling) {
		// Move rook based on king's destination
		if (toLocation == 6) {  // White kingside
			addPieceAtSquare(5, WHITE_ROOK);
			removePieceAtSquare(7, WHITE_ROOK);
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(WHITE_ROOK)][7];
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(WHITE_ROOK)][5];
		} else if (toLocation == 2) {  // White queenside
			addPieceAtSquare(3, WHITE_ROOK);
			removePieceAtSquare(0, WHITE_ROOK);
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(WHITE_ROOK)][0];
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(WHITE_ROOK)][3];
		} else if (toLocation == 62) {  // Black kingside
			addPieceAtSquare(61, BLACK_ROOK);
			removePieceAtSquare(63, BLACK_ROOK);
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(BLACK_ROOK)][63];
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(BLACK_ROOK)][61];
		} else if (toLocation == 58) {  // Black queenside
			addPieceAtSquare(59, BLACK_ROOK);
			removePieceAtSquare(56, BLACK_ROOK);
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(BLACK_ROOK)][56];
			zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(BLACK_ROOK)][59];
		}
	}

	// ========== UPDATE CASTLING RIGHTS ==========
	int castlingRemoveIndex = castlingPieceIndex(thisPiece);
	if (castlingRemoveIndex >= 0) {
		// Remove rights based on moving piece and captured piece (using lookup table)
		castling &= ~CASTLING_REMOVE[castlingRemoveIndex][fromLocation];
	}
	// Remove rights from captured piece (only if there was a capture)
	castlingRemoveIndex = castlingPieceIndex(capturedPiece);
	if (castlingRemoveIndex >= 0) {
		castling &= ~CASTLING_REMOVE[castlingRemoveIndex][toLocation];
	}

	// ========== UPDATE EN PASSANT SQUARE ==========

	enPassantSquare = -1;
	// Check for double pawn push
	if (thisPiece == WHITE_PAWN || thisPiece == BLACK_PAWN) {
		int rankFrom = fromLocation >> 3;
		int rankTo = toLocation >> 3;
		if (abs(rankFrom - rankTo) == 2) {
			enPassantSquare = (fromLocation + toLocation) >> 1;
		}
	}

	// ========== UPDATE HALF-MOVE CLOCK ==========

	if (thisPiece == WHITE_PAWN || thisPiece == BLACK_PAWN || isCapture) {
		halfMoveClock = 0;
	} else {
		halfMoveClock++;
	}

	// ========== ZOBRIST HASH (Part 2: Additions) ==========

	// Add piece to destination square
	zobristHash ^= Zobrist::pieceSquare[Zobrist::getZobristIndex(finalPiece)][toLocation];

	// Add new castling rights
	zobristHash ^= Zobrist::castlingRights[castling];

	// Add new en passant
	if (enPassantSquare >= 0) {
		zobristHash ^= Zobrist::enPassantFile[enPassantSquare & 0x7];
	}

	// ========== UPDATE TURN AND MOVE COUNTER ==========

	turn = (int8_t) -turn;

	// Flip side to move in zobrist
	zobristHash ^= Zobrist::sideToMove;

	// Update full move number
	if (turn == 1) {
		fullMove++;
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
	Piece piece = pieceAtSquare(toLocation);

	removePieceAtSquare(toLocation, piece);

	/* If it was a promotion, restore the pawn */
	if (flags & MOVE_FLAG_PROMOTION) {
		piece = (turn == 1) ? WHITE_PAWN : BLACK_PAWN;
	}

	/* Move piece back */
	addPieceAtSquare(fromLocation, piece);

	/* Restore captured piece (if not en passant) */
	if (undoInfo.capturedPiece != NONE && !(flags & MOVE_FLAG_EN_PASSANT)) {
		addPieceAtSquare(toLocation, undoInfo.capturedPiece);
	}

	/* Undo en passant capture */
	if (flags & MOVE_FLAG_EN_PASSANT) {
		int capturedPawnSquare = toLocation + (turn == 1 ? -8 : 8);
		addPieceAtSquare(capturedPawnSquare, undoInfo.capturedPiece);
	}

	/* Undo castling */
	if (flags & MOVE_FLAG_CASTLING) {
		if (toLocation == 6) {  /* White kingside */
			addPieceAtSquare(7, WHITE_ROOK);
			removePieceAtSquare(5, WHITE_ROOK);
		} else if (toLocation == 2) {  /* White queenside */
			addPieceAtSquare(0, WHITE_ROOK);
			removePieceAtSquare(3, WHITE_ROOK);
		} else if (toLocation == 62) {  /* Black kingside */
			addPieceAtSquare(63, BLACK_ROOK);
			removePieceAtSquare(61, BLACK_ROOK);
		} else if (toLocation == 58) {  /* Black queenside */
			addPieceAtSquare(56, BLACK_ROOK);
			removePieceAtSquare(59, BLACK_ROOK);
		}
	}

	/* Restore state */
	halfMoveClock = undoInfo.halfMoveClock;
	enPassantSquare = undoInfo.enPassantSquare;
	castling = undoInfo.castlingRights;
	zobristHash = undoInfo.zobristHash;
}

uint64_t Board::computeZobristHash() const {
	uint64_t hash = 0;

	// Hash board position
	for (int square = 0; square < 64; square++) {
		Piece piece = pieceAtSquare(square);
		if (isPiece(piece)) {
			int pIdx = Zobrist::getZobristIndex(piece);
			hash ^= Zobrist::pieceSquare[pIdx][square];
		}
	}

	// Hash side to move
	if (turn == -1) {
		hash ^= Zobrist::sideToMove;
	}

	// Hash castling rights
	hash ^= Zobrist::castlingRights[castling];

	if (enPassantSquare >= 0 && enPassantSquare < 64) {
		int file = enPassantSquare & 0x7; // Same as % 8, but faster
		hash ^= Zobrist::enPassantFile[file];
	}

	return hash;
}