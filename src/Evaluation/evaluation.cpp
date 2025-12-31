//
// Created by ECanDo on 2025-12-09.
//

#include "evaluation.h"

int evaluatePawnShelter(Board &board, int side, int kingSquare, int kingFile, int kingRank,
						uint64_t myPawns, uint64_t theirPawns) {

	int totalPieces = std::popcount(board.getWhiteBitboard() | board.getBlackBitboard());
	if (totalPieces < 16) {
		return 0;
	}

	int score = 0;

	// Only evaluate if king is on back ranks
	bool kingOnBackRank = (side == 1 && kingRank <= 2) || (side == -1 && kingRank >= 5);
	if (!kingOnBackRank) {
		return 0;  // King has advanced, shelter less important
	}

	// Check if king has castled by seeing if rook has moved
	uint64_t rookBitBoard = (side == 1) ? board.whiteRooks : board.blackRooks;
	uint64_t kingsideRookMask; // = (side == 1) ? (1ULL << 7) : (1ULL << 63);   // h1/h8
	uint64_t queensideRookMask; // = (side == 1) ? (1ULL << 0) : (1ULL << 56);  // a1/a8

	if (side == 1) {
		kingsideRookMask = (1ULL << 7);
		queensideRookMask = (1ULL << 0);
	} else {
		kingsideRookMask = (1ULL << 63);   // h1/h8
		queensideRookMask = (1ULL << 56);
	}


	bool kingsideCastle = (kingFile >= 6) && !(rookBitBoard & kingsideRookMask);
	bool queensideCastle = (kingFile <= 2) && !(rookBitBoard & queensideRookMask);

	if (!(kingsideCastle || queensideCastle)) {
		return 0;
	}


	// Check shelter on files around the king
	for (int fileOffset = -1; fileOffset <= 1; fileOffset++) {
		int file = kingFile + fileOffset;

		// ... What is this???
//        if (file < 0 || file > 7) {
//            score -= 40;
//            continue;
//        }

		uint64_t fileMask = FILE_MASK << file;
		uint64_t myPawnsOnFile = myPawns & fileMask;
		uint64_t theirPawnsOnFile = theirPawns & fileMask;

		// === My Pawn Shelter ===
		if (myPawnsOnFile == 0) {
			score -= 35;
			if (fileOffset == 0) {
				score -= 20;
			}
		} else {
			int closestPawnSquare = -1;
			int closestDistance = 999;

			uint64_t filePawnsCopy = myPawnsOnFile;
			while (filePawnsCopy) {
				int pawnSquare = std::countr_zero(filePawnsCopy);
				filePawnsCopy &= filePawnsCopy - 1;
				int pawnRank = pawnSquare >> 3;
				int distance = abs(pawnRank - kingRank);

				bool correctSide = (side == 1 && pawnRank >= kingRank) ||
								   (side == -1 && pawnRank <= kingRank);

				if (correctSide && distance < closestDistance) {
					closestDistance = distance;
					closestPawnSquare = pawnSquare;
				}
			}

			if (closestPawnSquare != -1) {
				int pawnRank = closestPawnSquare >> 3;
				int startRank = (side == 1) ? 1 : 6;
				int distanceFromStart = abs(pawnRank - startRank);

				if (distanceFromStart < 4) {
					score += SHELTER_BONUS[distanceFromStart];
				} else {
					score -= 15;
				}

				if (fileOffset == 0 && closestDistance == 1) {
					score += 10;
				}
			}
		}


		// After minor tuning, doesn't add anything to it
		// === Enemy Pawn Storm ===
//        if (theirPawnsOnFile != 0) {
//            uint64_t stormPawns = theirPawnsOnFile;
//            while (stormPawns) {
//                int pawnSquare = std::countr_zero(stormPawns);
//                stormPawns &= stormPawns - 1;
//                int pawnRank = pawnSquare >> 3;
//
//                // Calculate advancement: how far from starting rank
//                int advancementRank;
//                if (side == 1) {
//                    // Black pawns advancing down (start rank 6)
//                    advancementRank = 6 - pawnRank;
//                } else {
//                    // White pawns advancing up (start rank 1)
//                    advancementRank = pawnRank - 1;
//                }
//
//                if (advancementRank >= 2 && advancementRank < 8) {
//                    score -= PAWN_STORM_BONUS[advancementRank] >> 3;
//
//                    if (fileOffset == 0) {
//                        score -= 10;
//                    }
//
//                    if (myPawnsOnFile == 0) {
//                        score -= 15;
//                    }
//                }
//            }
//        }
	}

	// === Fianchetto Bonus ===
//    if (kingsideCastle || queensideCastle) {
//        uint64_t myBishops = (side == 1) ? board.whiteBishops : board.blackBishops;
//
//        int fianchettoSquare, pawnSquare;
//        if (kingsideCastle) {
//            fianchettoSquare = (side == 1) ? 14 : 62;  // g2 or g7
//            pawnSquare = (side == 1) ? 23 : 55;        // h3 or h6
//        } else {
//            fianchettoSquare = (side == 1) ? 9 : 57;   // b2 or b7
//            pawnSquare = (side == 1) ? 16 : 48;        // a3 or a6
//        }
//
//        bool hasFianchettoBishop = myBishops & (1ULL << fianchettoSquare);
//        bool hasFianchettoPawn = myPawns & (1ULL << pawnSquare);
//
//        if (hasFianchettoBishop && hasFianchettoPawn) {
//            score += 20;
//        }
//    }

	return score;
}

int evaluatePawns(Board &board, int side) {
	int score = 0;
	// ==== Doubled Pawns ====
	// White pawns, subtract from total score (penalty)
	// Loop for each file
	uint64_t myPawns, theirPawns;
	int kingSquare, kingFile, kingRank;
	if (side == 1) {
		myPawns = board.whitePawns;
		theirPawns = board.blackPawns;

		kingSquare = std::countr_zero(board.whiteKing);
	} else {
		myPawns = board.blackPawns;
		theirPawns = board.whitePawns;

		kingSquare = std::countr_zero(board.blackKing);
	}
	kingFile = kingSquare & 0x7;
	kingRank = kingSquare >> 3;

	// TODO:
	//  Backwards Pawns
	//  Incorporate doubled pawn checks into the below loop

	uint64_t bitBoard = myPawns;
	while (bitBoard) {
		int pawnSquare = std::countr_zero(bitBoard);
		bitBoard &= bitBoard - 1;

		int pawnFile = pawnSquare & 0x7;
		int pawnRank = pawnSquare >> 3;

		uint64_t centerMask = FILE_MASK << pawnFile;
		// Doubled pawns

		int extraPawnsInFile = __builtin_popcount(myPawns & centerMask) - 1;
		if (extraPawnsInFile > 0) {
			// Real value = [x 12]; but for 2 pawns doubled, they are counted twice; so it becomes exponential
			//  for 3+ pawns stacked
			score -= extraPawnsInFile * extraPawnsInFile * 6; // TODO: Test this value
		}

		uint64_t mask = (pawnFile - 1 >= 0 ? FILE_MASK << (pawnFile - 1) : 0) |
						(pawnFile + 1 < 8 ? FILE_MASK << (pawnFile + 1) : 0);

		// Isolated pawn
		// [myPawns] & [mask] -> get pawns on adjacent files
		if ((myPawns & mask) == 0) {
			// There are no pawns on adjacent files; apply penalty
			score -= 50; // TODO: Test this value
		}

		// Add in the center file to check for a passed pawn
		mask |= centerMask;

		// Move mask to in front of the pawn
		if (side == 1) {
			mask <<= (pawnRank + 1) << 3; // ([Pawn rank] + 1) * 8
		} else {
			mask >>= (8 - pawnRank) << 3; // ([Pawn rank] - 1) * 8
		}

		if ((mask & theirPawns) == 0) {
			// There are no enemy pawns in front of our pawn, give bonus based on rank
			if (side == 1) {
				score += PASSED_PAWN_BONUS[pawnRank]; // 10x the rank, make sure to keep it under the value of a pawn on the 6th rank
			} else {
				// Still add the score for black, because it is still a good thing (from black POV)
				// 7 - pawnRank -> flip the rank so it is still based on how close it is to promoting
				score += PASSED_PAWN_BONUS[7 - pawnRank];
			}
		}

		// Pawn chains / Supported pawns
		uint64_t supportMask = 0;
		if (side == 1) {
			if (pawnFile > 0) supportMask |= 1ULL << (pawnSquare - 9);
			if (pawnFile < 7) supportMask |= 1ULL << (pawnSquare - 7);
		} else {
			if (pawnFile > 0) supportMask |= 1ULL << (pawnSquare + 7);
			if (pawnFile < 7) supportMask |= 1ULL << (pawnSquare + 9);
		}

		if (myPawns & supportMask) {
			score += 10;  // Bonus for supported pawn
		}
	}


	// ==== Pawns around the king, push the pawns on the other side ====
	// also known as pawn shelter
	score += evaluatePawnShelter(board, side, kingSquare, kingFile, kingRank, myPawns, theirPawns);

	return score;
}

int evaluateBoard(Board &board) {
	int score = 0;

	int mgScore = 0;
	int egScore = 0;

	int phase = 0;

	for (Piece piece: ALL_PIECES) {
		uint64_t bitboard = board.getBitboard(piece);
		// Sum piece values
		int pieceCount = std::popcount(bitboard);
		if (getPieceType(piece) != TYPE_PAWN) {
			phase += pieceCount;
		}
		score += pieceCount * getPieceValue(piece);

		// Piece square table values
		bool white = isWhite(piece);
		while (bitboard) {
			int sq = std::countr_zero(bitboard);
			bitboard &= bitboard - 1;

			if (white) { // Add white score
				mgScore += getPieceSquareValue(piece, sq);
				egScore += getEndGamePieceSquareValue(piece, sq);
			} else { // Subtract black score
				mgScore -= getPieceSquareValue(piece, sq);
				egScore -= getEndGamePieceSquareValue(piece, sq);
			}
		}
	}

	// Phase = sum of piece values (max 24 for opening position)
	// Knight/Bishop = 1, Rook = 2, Queen = 4
	phase = std::min(phase, 24);

	score += ((mgScore * phase) + (egScore * (24 - phase))) / 24;

	//score += evaluatePawns(board, 1); // Add the score for white; when score is negative, bad for white
	//score -= evaluatePawns(board, -1); // Subtract the score for black; when score is negative, good for white

	// ==== Mobility ====
	// TODO

	// TODO:
	//  Open files near king
	//  Game phase
	//  Hanging pieces

	// Return from current player's perspective; black does need to be negative
	return board.turn == 1 ? score : -score;
}

int getPieceSquareValue(Piece piece, int square) {
	/* For black pieces, flip the square vertically */
	int sq = isWhite(piece) ? flipIndex(square) : square; // Seems backwards, but is fine

	switch (getPieceType(piece)) {
		case TYPE_PAWN:
			return pawnTable[sq];
		case TYPE_KNIGHT:
			return knightTable[sq];
		case TYPE_BISHOP:
			return bishopTable[sq];
		case TYPE_ROOK:
			return rookTable[sq];
		case TYPE_QUEEN:
			return queenTable[sq];
		case TYPE_KING:
			return kingMiddleGameTable[sq];
		default:
			return 0;
	}
}

int getEndGamePieceSquareValue(Piece piece, int square) {
	/* For black pieces, flip the square vertically */
	int sq = isWhite(piece) ? flipIndex(square) : square; // Seems backwards, but is fine

	switch (getPieceType(piece)) {
		case TYPE_PAWN:
			return pawnEndGameTable[sq];
		case TYPE_KNIGHT:
			return knightTable[sq];
		case TYPE_BISHOP:
			return bishopTable[sq];
		case TYPE_ROOK:
			return rookTable[sq];
		case TYPE_QUEEN:
			return queenTable[sq];
		case TYPE_KING:
			return kingEndGameTable[sq];
		default:
			return 0;
	}
}