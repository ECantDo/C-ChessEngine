//
// Created by ECanDo on 2025-12-04.
//

#include "generate_moves.h"

const uint64_t FILE_MASK = 0x0101010101010101ULL;
const uint64_t RANK_MASK = 0x00000000000000FFULL;

// =====================================================================================================================
// Single generator functions
// =====================================================================================================================
void generateKingMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	uint64_t kingBitBoard;
	uint64_t myPieces, theirPieces;

	if (board.turn == 1) {
		myPieces = board.getWhiteBitboard();
		theirPieces = board.getBlackBitboard();
		kingBitBoard = board.whiteKing;
	} else {
		myPieces = board.getBlackBitboard();
		theirPieces = board.getWhiteBitboard();
		kingBitBoard = board.blackKing;
	}

	/* Use bitboards to find move squares as it should be a faster process than looping over all the squares.
	 * Although, that being said, looping over everything still might be needed... But the good news is that in
	 * every loop, there will be FAR FEWER operations to compete. That and there can be fewer move options to
	 * loop through.
	 */
	int kingSquare = std::countr_zero(kingBitBoard);

	uint64_t attacks = getKingAttacks(kingSquare);
	uint64_t legal = attacks & ~myPieces;

	uint64_t captures = legal & theirPieces;

	while (captures) {
		int to = std::countr_zero(captures);
		captures &= captures - 1;
		moveList[moveCount++] = encodeMove(kingSquare, to, MOVE_FLAG_CAPTURE);
	}

	if (!capturesOnly) {
		uint64_t quiet = legal & ~theirPieces;
		while (quiet) {
			int to = std::countr_zero(quiet);
			quiet &= quiet - 1;
			moveList[moveCount++] = encodeMove(kingSquare, to, 0);
		}

		// Can't castle with king in check
		if (isSquareAttacked(board, kingSquare, -board.turn)) {
			return;
		}


		uint64_t allPieceBitboard = myPieces | theirPieces;
		// White Castling moves
		if (board.turn == 1 && kingSquare == 4) {
			if ((board.castling & 0b1000) && (0x60 & allPieceBitboard) == 0 &&
				!isSquareAttacked(board, 5, -1)) { // White kingside
				moveList[moveCount++] = encodeMove(4, 6, MOVE_FLAG_CASTLING);
			}
			if ((board.castling & 0b0100) && (0x0E & allPieceBitboard) == 0 &&
				!isSquareAttacked(board, 3, -1)) { // White queen side
				moveList[moveCount++] = encodeMove(4, 2, MOVE_FLAG_CASTLING);
			}
		}
		if (board.turn == -1 && kingSquare == 60) { // Black Castling moves
			if ((board.castling & 0b0010) && (0x6000000000000000 & allPieceBitboard) == 0 &&
				!isSquareAttacked(board, 61, 1)) { // Black kingside
				moveList[moveCount++] = encodeMove(60, 62, MOVE_FLAG_CASTLING);
			}
			if ((board.castling & 0b0001) && (0x0E00000000000000 & allPieceBitboard) == 0 &&
				!isSquareAttacked(board, 59, 1)) { // Black queen side
				moveList[moveCount++] = encodeMove(60, 58, MOVE_FLAG_CASTLING);
			}
		}
	}
}

void generateRookMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	uint64_t rookBitBoard;
	uint64_t myPieces, theirPieces;

	if (board.turn == 1) {
		myPieces = board.getWhiteBitboard();
		theirPieces = board.getBlackBitboard();

		rookBitBoard = board.whiteRooks;
	} else {
		myPieces = board.getBlackBitboard();
		theirPieces = board.getWhiteBitboard();

		rookBitBoard = board.blackRooks;
	}

	uint64_t blockers = myPieces | theirPieces;

	while (rookBitBoard) {
		int startingSquare = std::countr_zero(rookBitBoard);
		rookBitBoard &= rookBitBoard - 1; // Clear the bit we just processed

		uint64_t attacks = getRookAttacks(startingSquare, blockers);
		attacks &= ~myPieces;

		uint64_t captures = attacks & theirPieces; // Use for making moves with the capture flag
		while (captures) {
			int destinationSquare = std::countr_zero(captures);
			captures &= captures - 1;
			moveList[moveCount++] = encodeMove(startingSquare, destinationSquare, MOVE_FLAG_CAPTURE);
		}

		if (!capturesOnly) {
			uint64_t quietMoves = attacks & ~theirPieces;
			while (quietMoves) {
				int destinationSquare = std::countr_zero(quietMoves);
				quietMoves &= quietMoves - 1;
				moveList[moveCount++] = encodeMove(startingSquare, destinationSquare, 0);
			}
		}
	}
}

void generateBishopMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	uint64_t bishopBitboard;
	uint64_t myPieces, theirPieces;

	if (board.turn == 1) {
		myPieces = board.getWhiteBitboard();
		theirPieces = board.getBlackBitboard();

		bishopBitboard = board.whiteBishops;
	} else {
		myPieces = board.getBlackBitboard();
		theirPieces = board.getWhiteBitboard();

		bishopBitboard = board.blackBishops;
	}

	uint64_t blockers = myPieces | theirPieces;

	while (bishopBitboard) {
		int startingSquare = std::countr_zero(bishopBitboard);
		bishopBitboard &= bishopBitboard - 1; // Clear the bit we just processed

		uint64_t attacks = getBishopAttacks(startingSquare, blockers);
		attacks &= ~myPieces;

		uint64_t captures = attacks & theirPieces; // Use for making moves with the capture flag
		while (captures) {
			int destinationSquare = std::countr_zero(captures);
			captures &= captures - 1;
			moveList[moveCount++] = encodeMove(startingSquare, destinationSquare, MOVE_FLAG_CAPTURE);
		}

		if (!capturesOnly) {
			uint64_t quietMoves = attacks & ~theirPieces;
			while (quietMoves) {
				int destinationSquare = std::countr_zero(quietMoves);
				quietMoves &= quietMoves - 1;
				moveList[moveCount++] = encodeMove(startingSquare, destinationSquare, 0);
			}
		}

	}
}

void generateQueenMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	uint64_t queenBitboard;
	uint64_t myPieces, theirPieces;

	if (board.turn == 1) {
		myPieces = board.getWhiteBitboard();
		theirPieces = board.getBlackBitboard();
		queenBitboard = board.whiteQueens;
	} else {
		myPieces = board.getBlackBitboard();
		theirPieces = board.getWhiteBitboard();
		queenBitboard = board.blackQueens;
	}

	uint64_t blockers = myPieces | theirPieces;

	while (queenBitboard) {
		int startingSquare = std::countr_zero(queenBitboard);
		queenBitboard &= queenBitboard - 1; // Clear the bit we just processed

		uint64_t attacks = getQueenAttacks(startingSquare, blockers);
		attacks &= ~myPieces;

		uint64_t captures = attacks & theirPieces; // Use for making moves with the capture flag
		while (captures) {
			int destinationSquare = std::countr_zero(captures);
			captures &= captures - 1;
			moveList[moveCount++] = encodeMove(startingSquare, destinationSquare, MOVE_FLAG_CAPTURE);
		}

		if (!capturesOnly) {
			uint64_t quietMoves = attacks & ~theirPieces;
			while (quietMoves) {
				int destinationSquare = std::countr_zero(quietMoves);
				quietMoves &= quietMoves - 1;
				moveList[moveCount++] = encodeMove(startingSquare, destinationSquare, 0);
			}
		}
	}
}

void generateKnightMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	uint64_t knightBitboard;
	uint64_t myPieces, theirPieces;

	if (board.turn == 1) {
		myPieces = board.getWhiteBitboard();
		theirPieces = board.getBlackBitboard();
		knightBitboard = board.whiteKnights;
	} else {
		myPieces = board.getBlackBitboard();
		theirPieces = board.getWhiteBitboard();
		knightBitboard = board.blackKnights;
	}

	while (knightBitboard) {
		int knightSquare = std::countr_zero(knightBitboard);
		knightBitboard &= knightBitboard - 1;

		uint64_t attacks = KNIGHT_ATTACKS[knightSquare];
		uint64_t legal = attacks & ~myPieces;

		// Will always generate the captures only
		uint64_t captures = legal & theirPieces;
		while (captures) {
			int to = std::countr_zero(captures);
			captures &= captures - 1;
			moveList[moveCount++] = encodeMove(knightSquare, to, MOVE_FLAG_CAPTURE);
		}

		// only generate non-captures if we are not generating only captures
		if (!capturesOnly) {
			uint64_t quiet = legal & ~theirPieces;
			while (quiet) {
				int to = std::countr_zero(quiet);
				quiet &= quiet - 1;
				moveList[moveCount++] = encodeMove(knightSquare, to, 0);
			}
		}
	}
}

void generatePawnMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	uint64_t pawnBitboard;
	uint64_t myPieces, theirPieces;
	int direction;  /* +8 for white (moving up), -8 for black (moving down) */
	int startRank, promotionRank;

	if (board.turn == 1) {  /* White */
		myPieces = board.getWhiteBitboard();
		theirPieces = board.getBlackBitboard();
		pawnBitboard = board.whitePawns;
		direction = 8;
		startRank = 1;  /* Rank 2 in 0-indexed */
		promotionRank = 7;  /* Rank 8 */
	} else {  /* Black */
		myPieces = board.getBlackBitboard();
		theirPieces = board.getWhiteBitboard();
		pawnBitboard = board.blackPawns;
		direction = -8;
		startRank = 6;  /* Rank 7 in 0-indexed */
		promotionRank = 0;  /* Rank 1 */
	}

	uint64_t occupied = myPieces | theirPieces;

	while (pawnBitboard) {
		int pawnSquare = std::countr_zero(pawnBitboard);
		pawnBitboard &= pawnBitboard - 1;

		int pawnRank = pawnSquare >> 3;
		int pawnFile = pawnSquare & 0x7;

		// === 1. Moving Forward ===
		if (!capturesOnly) {
			int oneForward = pawnSquare + direction;

			if (isValidSquare(oneForward) && !(occupied & (1ULL << oneForward))) {
				if (pawnRank + (direction >> 3) == promotionRank) {
					// Add promotion moves
					moveList[moveCount++] = encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_QUEEN);
					moveList[moveCount++] = encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_ROOK);
					moveList[moveCount++] = encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_BISHOP);
					moveList[moveCount++] = encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_KNIGHT);
				} else {
					// Normal move forwards
					moveList[moveCount++] = encodeMove(pawnSquare, oneForward, 0);

					// === 2. Double Forwards ===
					if (pawnRank == startRank) {
						int twoForward = pawnSquare + (direction << 1); // Fast mult by 2
						if (!(occupied & (1ULL << twoForward))) {
							moveList[moveCount++] = encodeMove(pawnSquare, twoForward, 0);
						}
					}
				}
			}
		}

		// === 3. CAPTURES ===
		int captureOffsets[2] = {direction - 1, direction + 1};

		for (int captureOffset: captureOffsets) {
			int captureSquare = pawnSquare + captureOffset;

			// Check for going off the end
			if (!isValidSquare(captureSquare)) continue;

			// Check for wrap
			int captureFile = captureSquare % 8;
			if (abs(captureFile - pawnFile) != 1) continue;

			if (theirPieces & (1ULL << captureSquare)) {
				if (pawnRank + (direction / 8) == promotionRank) {
					//Promotion captures
					moveList[moveCount++] = (encodeMove(pawnSquare, captureSquare,
														MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_QUEEN));
					moveList[moveCount++] = (encodeMove(pawnSquare, captureSquare,
														MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_ROOK));
					moveList[moveCount++] = (encodeMove(pawnSquare, captureSquare,
														MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_BISHOP));
					moveList[moveCount++] = (encodeMove(pawnSquare, captureSquare,
														MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_KNIGHT));
				} else {
					// Normal capture
					moveList[moveCount++] = (encodeMove(pawnSquare, captureSquare, MOVE_FLAG_CAPTURE));
				}
			}
		}

		// === 4. En Passant ===
		if (board.enPassantSquare >= 0 && board.enPassantSquare < 64) {
			int epSquare = board.enPassantSquare;
			int epFile = epSquare % 8;

			// Check if we can capture
			if (abs(epFile - pawnFile) == 1 && epSquare == pawnSquare + direction - 1) {
				moveList[moveCount++] = (encodeMove(pawnSquare, epSquare, MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE));
			} else if (abs(epFile - pawnFile) == 1 && epSquare == pawnSquare + direction + 1) {
				moveList[moveCount++] = (encodeMove(pawnSquare, epSquare, MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE));
			}
		}

		// And that's pawns... goodness... the simplest piece has the most rules...
	}
}
// =====================================================================================================================
// Helper functions
// =====================================================================================================================

bool isEnemyPiece(const Board &board, int square, int myColor) {
	uint64_t bitboard = myColor == -1 ? board.getWhiteBitboard() : board.getBlackBitboard();
	uint64_t mask = 1ULL << square;

	return (bitboard & mask) != 0;
}

bool isEmpty(const Board &board, int square) {
	uint64_t bitboard = board.getWhiteBitboard() | board.getBlackBitboard();
	uint64_t mask = 1ULL << square;

	return (bitboard & mask) == 0;
}

bool isValidSquare(int square) {
	return square < 64 && square >= 0;
}

// =====================================================================================================================
// Generate moves
// =====================================================================================================================
uint64_t computePinnedPieces(const Board &board, int kingSquare) {
//	Board board = boardR;
	uint64_t pinned = 0;
	int turn = board.turn;

	// Get enemy sliders
	uint64_t enemyRooks = (turn == 1) ? board.blackRooks : board.whiteRooks;
	uint64_t enemyBishops = (turn == 1) ? board.blackBishops : board.whiteBishops;
	uint64_t enemyQueens = (turn == 1) ? board.blackQueens : board.whiteQueens;

	uint64_t allPieces = board.getWhiteBitboard() | board.getBlackBitboard();
	uint64_t ourPieces = (turn == 1) ? board.getWhiteBitboard() : board.getBlackBitboard();

	// Directions: N, S, E, W, NE, NW, SE, SW
	const int dirs[8] = {8, -8, 1, -1, 9, 7, -7, -9};

	for (int d = 0; d < 8; ++d) {
		int dir = dirs[d];
		uint64_t potentialPin = 0;

		// Start from square just beyond king
		for (int sq = kingSquare + dir; ; sq += dir) {
			// Check bounds
			if (sq < 0 || sq >= 64) break;

			// Check for board edge wrap
			int fromFile = (sq - dir) % 8;
			int toFile = sq % 8;
			if (abs(fromFile - toFile) > 1) break; // Wrapped across board edge

			uint64_t sqMask = 1ULL << sq;

			if (allPieces & sqMask) {
				if (ourPieces & sqMask) {
					// First friendly piece encountered
					if (potentialPin == 0) {
						potentialPin = sqMask;
					} else {
						// Second friendly piece - not a pin
						break;
					}
				} else {
					// Enemy piece
					Piece enemy = board.pieceAtSquare(sq);
					bool isSlider = false;

					// Check if enemy is appropriate slider for this direction
					if (d < 4) { // N, S, E, W (rook directions)
						if (enemyRooks & sqMask || enemyQueens & sqMask) {
							isSlider = true;
						}
					} else { // NE, NW, SE, SW (bishop directions)
						if (enemyBishops & sqMask || enemyQueens & sqMask) {
							isSlider = true;
						}
					}

					if (isSlider && potentialPin != 0) {
						pinned |= potentialPin;
					}
					break;
				}
			}
		}
	}

	return pinned;
}

void generatePseudoLegalMoves(const Board &board, Move *moveList, int &moveCount, bool capturesOnly) {
	generateRookMoves(board, moveList, moveCount, capturesOnly);
	generateBishopMoves(board, moveList, moveCount, capturesOnly);
	generateQueenMoves(board, moveList, moveCount, capturesOnly);
	generateKnightMoves(board, moveList, moveCount, capturesOnly);
	generatePawnMoves(board, moveList, moveCount, capturesOnly);
	generateKingMoves(board, moveList, moveCount, capturesOnly);
}


void generateLegalMoves(Board &board, std::vector<Move> &moveList, bool capturesOnly) {
	Move pseudoLegal[MAX_MOVES];
	int moveCount = 0;
	generatePseudoLegalMoves(board, pseudoLegal, moveCount, capturesOnly);

	moveList.clear();
	moveList.reserve(moveCount);

	uint64_t ourKing = (board.turn == 1) ? board.whiteKing : board.blackKing;
	int kingSquare = std::countr_zero(ourKing);

	bool weAreInCheck = isSquareAttacked(board, kingSquare, -board.turn);

	uint64_t pinnedPieces = 0;
	if (!weAreInCheck) {
		pinnedPieces = computePinnedPieces(board, kingSquare);
	}

	for (int i = 0; i < moveCount; ++i) {
		Move m = pseudoLegal[i];
		int from = getMoveFrom(m);
		int to = getMoveTo(m);
		int flags = getMoveFlags(m);

		// Check if this is an en passant move
		bool isEnPassant = (flags & MOVE_FLAG_EN_PASSANT) != 0;

		// King moves, en passant, pinned pieces, or in check always need full verification
		if (from == kingSquare || isEnPassant || weAreInCheck || (pinnedPieces & (1ULL << from))) {
			UndoInfo undoInfo = board.makeMove(m);

			uint64_t newKing = (board.turn == -1) ? board.whiteKing : board.blackKing;
			int newKingSquare = std::countr_zero(newKing);

			bool causesCheck = isSquareAttacked(board, newKingSquare, board.turn);

			board.unmakeMove(m, undoInfo);

			if (!causesCheck) {
				moveList.push_back(m);
			}
		} else {
			// Not in check, not pinned, not en passant, not king move - move is legal
			moveList.push_back(m);
		}
	}
}