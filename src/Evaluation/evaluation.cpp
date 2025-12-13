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
    uint64_t kingsideRookSquare = (side == 1) ? (1ULL << 7) : (1ULL << 63);   // h1/h8
    uint64_t queensideRookSquare = (side == 1) ? (1ULL << 0) : (1ULL << 56);  // a1/a8

    bool kingsideCastle = (kingFile >= 6) && !(rookBitBoard & kingsideRookSquare);
    bool queensideCastle = (kingFile <= 2) && !(rookBitBoard & queensideRookSquare);

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
//                    score -= PAWN_STORM_BONUS[advancementRank] / 2;
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


    for (int i = 0; i < 8; i++) {
        int extraPawnsInFile = __builtin_popcount(myPawns & (FILE_MASK << i)) - 1;

        // Penalty = (x-1)^2 * 25 | x > 1 , where x = number of pawns in file
        // The idea is to have a smaller penalty for 2 pawns doubled, but a much larger one for 3+ pawns
        // with 2 pawns, penalty is -25; 3 pawns is -100, or a whole pawn, which is effectively what it is
        if (extraPawnsInFile > 0) {
            score -= extraPawnsInFile * extraPawnsInFile * 12; // TODO: Test this value
        }
    }

    // TODO:
    //  Backwards Pawns
    //  Incorporate doubled pawn checks into the below loop

    uint64_t bitBoard = myPawns;
    while (bitBoard) {
        int pawnSquare = std::countr_zero(bitBoard);
        bitBoard &= bitBoard - 1;

        int pawnFile = pawnSquare & 0x7;
        int pawnRank = pawnSquare >> 3;

        uint64_t mask = (pawnFile - 1 >= 0 ? FILE_MASK << (pawnFile - 1) : 0) |
                        (pawnFile + 1 < 8 ? FILE_MASK << (pawnFile + 1) : 0);

        // Isolated pawn
        // [myPawns] & [mask] -> get pawns on adjacent files
        if ((myPawns & mask) == 0) {
            // There are no pawns on adjacent files; apply penalty
            score -= 50; // TODO: Test this value
        }

        // Add in the center file to check for a passed pawn
        mask |= FILE_MASK << pawnFile;

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
//    score += evaluatePawnShelter(board, side, kingSquare, kingFile, kingRank, myPawns, theirPawns);

    return score;
}

int kingBetweenRooksScore(Board &board, int side) {
    int score = 0;

    uint64_t rookBitboard, kingBitboard;
    int backRank, castlingRights;
    if (side == 1) {
        rookBitboard = board.whiteRooks;
        kingBitboard = board.whiteKing;
        backRank = 0;
        castlingRights = board.castling & 0b1100;
    } else {
        rookBitboard = board.blackRooks;
        kingBitboard = board.blackKing;
        backRank = 7;
        castlingRights = board.castling & 0b0011;
    }

    // Check if the king is trapping the rook

    // If it can castle, not trapped
    if (castlingRights) {
        return 0;
    }

    int kingSquare = std::countr_zero(kingBitboard);
    int kingFile = kingSquare & 0x7;
    int kingRank = kingSquare >> 3;

    // If king not on back rank, doesn't matter
    if (kingRank != backRank) {
        return 0;
    }

    // There is a rook trapped on the king side \\ queen side
    if ((kingFile > 4 && (0xC0 << (backRank << 3)) & rookBitboard)
        || (kingFile <= 4 && (0x03 << (backRank << 3)) & rookBitboard)) {
        score -= 40;
    }

    return score;
}

int evaluateBoard(Board &board) {
    int score = 0;

    for (char piece: ALL_PIECES) {
        uint64_t bitboard = board.getBitboard(piece);
        // Sum piece values
        score += std::popcount(bitboard) * getPieceValue(piece);

        // Piece square table values
        bool isWhite = isupper(piece);
        while (bitboard) {
            int sq = std::countr_zero(bitboard);
            bitboard &= bitboard - 1;

            if (isWhite) { // Add white score
                score += getPieceSquareValue(piece, sq);
            } else { // Subtract black score
                score -= getPieceSquareValue(piece, sq);
            }
        }
    }

    score += evaluatePawns(board, 1); // Add the score for white; when score is negative, bad for white
    score -= evaluatePawns(board, -1); // Subtract the score for black; when score is negative, good for white

    // ==== Mobility ====
    // TODO
    // Should just be [mobility_bonus * (#whitemoves - #blackmoves)] and it should be good enough (for now)

    // Doesn't help, bot now does ~3 ELO worse than V10.2
//    score += kingBetweenRooksScore(board, 1);
//    score -= kingBetweenRooksScore(board, -1);

    // Doesn't seem to help ~30 ELO worse than V10.2
//    std::vector<Move> moves;
//    moves.reserve(50);
//
//    generateRookMoves(board, moves);
//    size_t numMyMoves = moves.size();
//    board.turn *= -1;
//
//    moves.clear();
//    generateRookMoves(board, moves);
//    board.turn *= -1;
//
//    score += (int) (numMyMoves - moves.size()) * 4;





    // TODO:
    //  Open files near king
    //  Game phase
    //  Hanging pieces

    // Return from current player's perspective; black does need to be negative
    return board.turn == 1 ? score : -score;
}

int getPieceSquareValue(char piece, int square) {
    /* For black pieces, flip the square vertically */
    bool isWhite = isupper(piece);
    int sq = isWhite ? flipIndex(square) : square; // Seems backwards, but is fine

    switch (tolower(piece)) {
        case 'p':
            return pawnTable[sq];
        case 'n':
            return knightTable[sq];
        case 'b':
            return bishopTable[sq];
        case 'r':
            return rookTable[sq];
        case 'q':
            return queenTable[sq];
        case 'k':
            return kingMiddleGameTable[sq];
        default:
            return 0;
    }
}