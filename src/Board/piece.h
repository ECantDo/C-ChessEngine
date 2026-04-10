//
// Created by ECanDo on 2025-08-23.
//

#ifndef CHESSENGINE_PIECE_H
#define CHESSENGINE_PIECE_H

#include <array>
#include <cstdint>

// Piece encoding: 0b[color][type]
// Color: 01 = white, 10 = black
// Type: 001-110 for different pieces
enum Piece : uint8_t {
	NONE = 0b00000,

	// White pieces (0b01xxx)
	WHITE_PAWN = 0b01001,
	WHITE_KNIGHT = 0b01011,
	WHITE_BISHOP = 0b01100,
	WHITE_ROOK = 0b01010,
	WHITE_QUEEN = 0b01101,
	WHITE_KING = 0b01110,

	// Black pieces (0b10xxx)
	BLACK_PAWN = 0b10001,
	BLACK_KNIGHT = 0b10011,
	BLACK_BISHOP = 0b10100,
	BLACK_ROOK = 0b10010,
	BLACK_QUEEN = 0b10101,
	BLACK_KING = 0b10110
};

enum Color : uint8_t {
	WHITE = 0,
	BLACK = 1,
};

// Color masks
constexpr uint8_t WHITE_MASK = 0b01000;
constexpr uint8_t BLACK_MASK = 0b10000;
constexpr uint8_t COLOR_MASK = 0b11000;
constexpr uint8_t TYPE_MASK = 0b00111;

// Piece type values (without color)
enum PieceType : uint8_t {
	TYPE_NONE = 0b000,
	TYPE_PAWN = 0b001,
	TYPE_ROOK = 0b010,
	TYPE_KNIGHT = 0b011,
	TYPE_BISHOP = 0b100,
	TYPE_QUEEN = 0b101,
	TYPE_KING = 0b110
};

// All pieces array (for iteration)
constexpr std::array<Piece, 12> ALL_PIECES = {
	WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
	BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING
};

extern int16_t PIECE_VALUE_LUT[BLACK_KING + 1];

void initPieceLUTs();

// Functions
int getPieceValue(Piece piece);

// Inline helper functions
inline constexpr bool isWhite(const Piece piece) {
	return (piece & WHITE_MASK) != 0;
}

inline constexpr bool isBlack(const Piece piece) {
	return (piece & BLACK_MASK) != 0;
}

inline constexpr bool isPiece(const Piece piece) {
	return piece != NONE;
}

inline constexpr PieceType getPieceType(const Piece piece) {
	return static_cast<PieceType>(piece & TYPE_MASK);
}

inline constexpr bool isPawn(const Piece piece) {
	return getPieceType(piece) == TYPE_PAWN;
}

inline constexpr bool isKing(const Piece piece) {
	return getPieceType(piece) == TYPE_KING;
}

inline constexpr bool isRook(const Piece piece) {
	return getPieceType(piece) == TYPE_ROOK;
}

// Get piece with opposite color
inline constexpr Piece flipColor(const Piece piece) {
	return static_cast<Piece>(piece ^ COLOR_MASK);
}

// Create piece from color and type
inline constexpr Piece makePiece(const bool isWhite, const PieceType type) {
	return static_cast<Piece>((isWhite ? WHITE_MASK : BLACK_MASK) | type);
}

inline constexpr Piece charToPiece(const char pieceChar) {
	switch (pieceChar) {
		case 'P':
			return WHITE_PAWN;
		case 'B':
			return WHITE_BISHOP;
		case 'R':
			return WHITE_ROOK;
		case 'Q':
			return WHITE_QUEEN;
		case 'N':
			return WHITE_KNIGHT;
		case 'K':
			return WHITE_KING;

		case 'p':
			return BLACK_PAWN;
		case 'b':
			return BLACK_BISHOP;
		case 'r':
			return BLACK_ROOK;
		case 'q':
			return BLACK_QUEEN;
		case 'n':
			return BLACK_KNIGHT;
		case 'k':
			return BLACK_KING;
		default:
			return NONE;
	}
}

inline constexpr char pieceToChar(const Piece piece) {
	switch (piece) {
		case WHITE_PAWN:
			return 'P';
		case WHITE_KNIGHT:
			return 'N';
		case WHITE_BISHOP:
			return 'B';
		case WHITE_ROOK:
			return 'R';
		case WHITE_QUEEN:
			return 'Q';
		case WHITE_KING:
			return 'K';

		case BLACK_PAWN:
			return 'p';
		case BLACK_KNIGHT:
			return 'n';
		case BLACK_BISHOP:
			return 'b';
		case BLACK_ROOK:
			return 'r';
		case BLACK_QUEEN:
			return 'q';
		case BLACK_KING:
			return 'k';

		default:
			return '.';
	}
}

#endif //CHESSENGINE_PIECE_H
