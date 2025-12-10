//
// Created by ECanDo on 2025-12-04.
//

#include "generate_moves.h"

const uint64_t FILE_MASK = 0x0101010101010101ULL;
const uint64_t RANK_MASK = 0x00000000000000FFULL;


void generateLegalMoves(Board &board, std::vector<Move> &moveList, bool capturesOnly) {
    std::vector<Move> pseudoLegal;
    generatePseudoLegalMoves(board, pseudoLegal, capturesOnly);

    moveList.clear();
    moveList.reserve(pseudoLegal.size());

    for (Move m: pseudoLegal) {
        UndoInfo undoInfo = board.makeMove(m);

        uint64_t ourKing = (board.turn == -1) ? board.whiteKing : board.blackKing;
        int kingSquare = std::countr_zero(ourKing);

        bool inCheck = isSquareAttacked(board, kingSquare, board.turn);

        board.unmakeMove(m, undoInfo);

        if (!inCheck) {
            moveList.push_back(m);
        }
    }

}

void generatePseudoLegalMoves(const Board &board, std::vector<Move> &moveList, bool capturesOnly) {
    //
    moveList.clear();
    if (capturesOnly) {
        // The maximum number of captures possible in a single, legally reachable chess board position is 13. - Google AI
        // So double it, add a bit of leeway, and we should be good to go for minimizing disc space without compromising
        // search time with reallocating memory
        moveList.reserve(32);

        generatePawnCaptures(board, moveList);
        generateKingCaptures(board, moveList);
        generateRookCaptures(board, moveList);
        generateBishopCaptures(board, moveList);
        generateQueenCaptures(board, moveList);
        generateKnightCaptures(board, moveList);

    } else {
        moveList.reserve(MAX_MOVES);

        generatePawnMoves(board, moveList);
        generateKingMoves(board, moveList);
        generateRookMoves(board, moveList);
        generateBishopMoves(board, moveList);
        generateQueenMoves(board, moveList);
        generateKnightMoves(board, moveList);
    }
}


// =====================================================================================================================
// Single capture functions
// =====================================================================================================================
void generateKingCaptures(const Board &board, std::vector<Move> &moveList) {
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
    int kingFile = kingSquare & 0x7; // %8, but faster ; 0-7
    int kingRank = kingSquare >> 3; // /8, but faster ; 8 >> 3 == 1

    // Files first
    uint64_t fileMask = (FILE_MASK << kingFile) |
                        (kingFile - 1 >= 0 ? FILE_MASK << (kingFile - 1) : 0) |
                        (kingFile + 1 < 8 ? FILE_MASK << (kingFile + 1) : 0);
    uint64_t rankMask = ((RANK_MASK << (kingRank << 3 /*rank mult by 8*/))) |
                        (kingRank - 1 >= 0 ? RANK_MASK << ((kingRank - 1) << 3) : 0) |
                        (kingRank + 1 < 8 ? RANK_MASK << ((kingRank + 1) << 3) : 0);

    uint64_t moveMask = fileMask & rankMask;
//    moveMask &= ~myPieces; // Dont need this because I'm only looking at captures anyways
    moveMask &= theirPieces; // Only here for the generating captures part.

    while (moveMask) {
        int moveToSquare = std::countr_zero(moveMask);
        moveMask &= moveMask - 1;

        // Flag is capture, because it will always be a capture
        moveList.push_back(encodeMove(kingSquare, moveToSquare, MOVE_FLAG_CAPTURE));
    }

    // No Castling, castling cannot produce captures
}

void generatePawnCaptures(const Board &board, std::vector<Move> &moveList) {
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

        int pawnRank = pawnSquare / 8;
        int pawnFile = pawnSquare % 8;

        // === 1. Moving Forward ===
        // Nothing to capture here...

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
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_QUEEN));
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_ROOK));
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_BISHOP));
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_KNIGHT));
                } else {
                    // Normal capture
                    moveList.push_back(encodeMove(pawnSquare, captureSquare, MOVE_FLAG_CAPTURE));
                }
            }
        }

        // === 4. En Passant ===
        if (board.enPassantSquare >= 0 && board.enPassantSquare < 64) {
            int epSquare = board.enPassantSquare;
            int epFile = epSquare % 8;

            // Check if we can capture
            if (abs(epFile - pawnFile) == 1 && epSquare == pawnSquare + direction - 1) {
                moveList.push_back(encodeMove(pawnSquare, epSquare, MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE));
            } else if (abs(epFile - pawnFile) == 1 && epSquare == pawnSquare + direction + 1) {
                moveList.push_back(encodeMove(pawnSquare, epSquare, MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE));
            }
        }

        // And that's pawns... goodness... the simplest piece has the most rules...
    }
}

void generateRookCaptures(const Board &board, std::vector<Move> &moveList) {
//TODO: Magic bitboards
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

    while (rookBitBoard) {
        int rookSquare = std::countr_zero(rookBitBoard);
        rookBitBoard &= rookBitBoard - 1; // Clear the bit we just processed

        for (int dir: rookOffsets) {
            int targetSquare = rookSquare + dir;

            // Keep sliding until we are off the board
            while (isValidSquare(targetSquare)) {
                // If horizontal movement; stop when wrapping around the board.
                if (dir == 1 || dir == -1) {
                    int fromFile = (targetSquare - dir) % 8;
                    int toFile = targetSquare % 8;
                    if (abs(toFile - fromFile) > 1) {
                        break;
                    }
                }

                uint64_t targetMask = 1ULL << targetSquare;

                // Hit our own piece --- stop
                if (targetMask & myPieces) break;

                // Hit opponent piece --- add and stop
                if (targetMask & theirPieces) {
                    moveList.push_back(encodeMove(rookSquare, targetSquare, MOVE_FLAG_CAPTURE));
                    break;
                }

                // Otherwise the square is empty
                targetSquare += dir;
            }
        }
    }
}

void generateBishopCaptures(const Board &board, std::vector<Move> &moveList) {
//TODO: Magic bitboards
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

    while (bishopBitboard) {
        int bishopSquare = std::countr_zero(bishopBitboard);
        bishopBitboard &= bishopBitboard - 1; // Clear the bit we just processed

        for (int dir: bishopOffsets) {
            int targetSquare = bishopSquare + dir;

            // Keep sliding until we are off the board
            while (isValidSquare(targetSquare)) {
                int fromFile = (targetSquare - dir) % 8;
                int toFile = targetSquare % 8;
                if (abs(toFile - fromFile) > 1) {
                    break;
                }

                uint64_t targetMask = 1ULL << targetSquare;

                // Hit our own piece --- stop
                if (targetMask & myPieces) break;

                // Hit opponent piece --- add and stop
                if (targetMask & theirPieces) {
                    moveList.push_back(encodeMove(bishopSquare, targetSquare, MOVE_FLAG_CAPTURE));
                    break;
                }

                // Otherwise the square is empty
                targetSquare += dir;
            }
        }
    }
}

void generateQueenCaptures(const Board &board, std::vector<Move> &moveList) {
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

    while (queenBitboard) {
        int queenSquare = std::countr_zero(queenBitboard);
        queenBitboard &= queenBitboard - 1;

        /* Queen moves = rook directions + bishop directions */
        const int directions[8] = {8, -8, 1, -1, 9, -9, 7, -7};

        for (int dir: directions) {
            int targetSquare = queenSquare + dir;

            while (isValidSquare(targetSquare)) {
                /* Check for wrap (horizontal or diagonal) */
                int fromFile = (targetSquare - dir) % 8;
                int toFile = targetSquare % 8;
                if (abs(toFile - fromFile) > 2) break;  /* Wrapped */

                uint64_t targetMask = 1ULL << targetSquare;

                if (targetMask & myPieces) break;

                if (targetMask & theirPieces) {
                    moveList.push_back(encodeMove(queenSquare, targetSquare, MOVE_FLAG_CAPTURE));
                    break;
                }
                targetSquare += dir;
            }
        }
    }
}

void generateKnightCaptures(const Board &board, std::vector<Move> &moveList) {
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

    const int knightOffsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};

    while (knightBitboard) {
        int knightSquare = std::countr_zero(knightBitboard);
        knightBitboard &= knightBitboard - 1;

        for (int offset: knightOffsets) {
            int targetSquare = knightSquare + offset;

            if (!isValidSquare(targetSquare)) continue;

            /* Knights wrap differently - check file distance is exactly 1 or 2 */
            int fromFile = knightSquare % 8;
            int toFile = targetSquare % 8;
            int fileDist = abs(toFile - fromFile);
            if (fileDist != 1 && fileDist != 2) continue;  /* Wrapped */

            uint64_t targetMask = 1ULL << targetSquare;

            if (targetMask & myPieces) continue;

            if (targetMask & theirPieces) {
                moveList.push_back(encodeMove(knightSquare, targetSquare, MOVE_FLAG_CAPTURE));
            }
        }
    }
}

// =====================================================================================================================
// Single generator functions
// =====================================================================================================================
void generateKingMoves(const Board &board, std::vector<Move> &moveList) {
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
    int kingFile = kingSquare & 0x7; // %8, but faster ; 0-7
    int kingRank = kingSquare >> 3; // /8, but faster ; 8 >> 3 == 1

    // Files first
    uint64_t fileMask = (FILE_MASK << kingFile) |
                        (kingFile - 1 >= 0 ? FILE_MASK << (kingFile - 1) : 0) |
                        (kingFile + 1 < 8 ? FILE_MASK << (kingFile + 1) : 0);
    uint64_t rankMask = ((RANK_MASK << (kingRank << 3 /*rank mult by 8*/))) |
                        (kingRank - 1 >= 0 ? RANK_MASK << ((kingRank - 1) << 3) : 0) |
                        (kingRank + 1 < 8 ? RANK_MASK << ((kingRank + 1) << 3) : 0);

    uint64_t moveMask = fileMask & rankMask;
    moveMask &= ~myPieces; // Don't onto my own pieces
//    moveMask &= theirPieces; // Only here for the generating captures part.

    while (moveMask) {
        int moveToSquare = std::countr_zero(moveMask);
        moveMask &= moveMask - 1;

        // Check for a capturev
        int flag = 0;
        if ((1ULL << moveToSquare) & theirPieces) {
            flag = MOVE_FLAG_CAPTURE;
        }
        moveList.push_back(encodeMove(kingSquare, moveToSquare, flag));
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
            moveList.push_back(encodeMove(4, 6, MOVE_FLAG_CASTLING));
        }
        if ((board.castling & 0b0100) && (0x0E & allPieceBitboard) == 0 &&
            !isSquareAttacked(board, 3, -1)) { // White queen side
            moveList.push_back(encodeMove(4, 2, MOVE_FLAG_CASTLING));
        }
    }
    if (board.turn == -1 && kingSquare == 60) { // Black Castling moves
        if ((board.castling & 0b0010) && (0x6000000000000000 & allPieceBitboard) == 0 &&
            !isSquareAttacked(board, 61, 1)) { // Black kingside
            moveList.push_back(encodeMove(60, 62, MOVE_FLAG_CASTLING));
        }
        if ((board.castling & 0b0001) && (0x0E00000000000000 & allPieceBitboard) == 0 &&
            !isSquareAttacked(board, 59, 1)) { // Black queen side
            moveList.push_back(encodeMove(60, 58, MOVE_FLAG_CASTLING));
        }
    }
}

void generateRookMoves(const Board &board, std::vector<Move> &moveList) {
    //TODO: Magic bitboards
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

    while (rookBitBoard) {
        int rookSquare = std::countr_zero(rookBitBoard);
        rookBitBoard &= rookBitBoard - 1; // Clear the bit we just processed

        for (int dir: rookOffsets) {
            int targetSquare = rookSquare + dir;

            // Keep sliding until we are off the board
            while (isValidSquare(targetSquare)) {
                // If horizontal movement; stop when wrapping around the board.
                if (dir == 1 || dir == -1) {
                    int fromFile = (targetSquare - dir) % 8;
                    int toFile = targetSquare % 8;
                    if (abs(toFile - fromFile) > 1) {
                        break;
                    }
                }

                uint64_t targetMask = 1ULL << targetSquare;

                // Hit our own piece --- stop
                if (targetMask & myPieces) break;

                // Hit opponent piece --- add and stop
                if (targetMask & theirPieces) {
                    moveList.push_back(encodeMove(rookSquare, targetSquare, MOVE_FLAG_CAPTURE));
                    break;
                }

                // Otherwise the square is empty
                moveList.push_back(encodeMove(rookSquare, targetSquare, 0));
                targetSquare += dir;
            }
        }
    }
}

void generateBishopMoves(const Board &board, std::vector<Move> &moveList) {
//TODO: Magic bitboards
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

    while (bishopBitboard) {
        int bishopSquare = std::countr_zero(bishopBitboard);
        bishopBitboard &= bishopBitboard - 1; // Clear the bit we just processed

        for (int dir: bishopOffsets) {
            int targetSquare = bishopSquare + dir;

            // Keep sliding until we are off the board
            while (isValidSquare(targetSquare)) {
                int fromFile = (targetSquare - dir) % 8;
                int toFile = targetSquare % 8;
                if (abs(toFile - fromFile) > 1) {
                    break;
                }

                uint64_t targetMask = 1ULL << targetSquare;

                // Hit our own piece --- stop
                if (targetMask & myPieces) break;

                // Hit opponent piece --- add and stop
                if (targetMask & theirPieces) {
                    moveList.push_back(encodeMove(bishopSquare, targetSquare, MOVE_FLAG_CAPTURE));
                    break;
                }

                // Otherwise the square is empty
                moveList.push_back(encodeMove(bishopSquare, targetSquare, 0));
                targetSquare += dir;
            }
        }
    }
}

void generateQueenMoves(const Board &board, std::vector<Move> &moveList) {
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

    while (queenBitboard) {
        int queenSquare = std::countr_zero(queenBitboard);
        queenBitboard &= queenBitboard - 1;

        /* Queen moves = rook directions + bishop directions */
        const int directions[8] = {8, -8, 1, -1, 9, -9, 7, -7};

        for (int dir: directions) {
            int targetSquare = queenSquare + dir;

            while (isValidSquare(targetSquare)) {
                /* Check for wrap (horizontal or diagonal) */
                int fromFile = (targetSquare - dir) % 8;
                int toFile = targetSquare % 8;
                if (abs(toFile - fromFile) > 2) break;  /* Wrapped */

                uint64_t targetMask = 1ULL << targetSquare;

                if (targetMask & myPieces) break;

                if (targetMask & theirPieces) {
                    moveList.push_back(encodeMove(queenSquare, targetSquare, MOVE_FLAG_CAPTURE));
                    break;
                }

                moveList.push_back(encodeMove(queenSquare, targetSquare, 0));
                targetSquare += dir;
            }
        }
    }
}

void generateKnightMoves(const Board &board, std::vector<Move> &moveList) {
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

    const int knightOffsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};

    while (knightBitboard) {
        int knightSquare = std::countr_zero(knightBitboard);
        knightBitboard &= knightBitboard - 1;

        for (int offset: knightOffsets) {
            int targetSquare = knightSquare + offset;

            if (!isValidSquare(targetSquare)) continue;

            /* Knights wrap differently - check file distance is exactly 1 or 2 */
            int fromFile = knightSquare % 8;
            int toFile = targetSquare % 8;
            int fileDist = abs(toFile - fromFile);
            if (fileDist != 1 && fileDist != 2) continue;  /* Wrapped */

            uint64_t targetMask = 1ULL << targetSquare;

            if (targetMask & myPieces) continue;

            int flags = (targetMask & theirPieces) ? MOVE_FLAG_CAPTURE : 0;
            moveList.push_back(encodeMove(knightSquare, targetSquare, flags));
        }
    }
}

void generatePawnMoves(const Board &board, std::vector<Move> &moveList) {
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

        int pawnRank = pawnSquare / 8;
        int pawnFile = pawnSquare % 8;

        // === 1. Moving Forward ===
        int oneForward = pawnSquare + direction;

        if (isValidSquare(oneForward) && !(occupied & (1ULL << oneForward))) {
            if (pawnRank + (direction / 8) == promotionRank) {
                // Add promotion moves
                moveList.push_back(encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_QUEEN));
                moveList.push_back(encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_ROOK));
                moveList.push_back(encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_BISHOP));
                moveList.push_back(encodeMove(pawnSquare, oneForward, MOVE_FLAG_PROMOTION | PROMOTE_TO_KNIGHT));
            } else {
                // Normal move forwards
                moveList.push_back(encodeMove(pawnSquare, oneForward, 0));

                // === 2. Double Forwards ===
                if (pawnRank == startRank) {
                    int twoForward = pawnSquare + (direction << 1); // Fast mult by 2
                    if (!(occupied & (1ULL << twoForward))) {
                        moveList.push_back(encodeMove(pawnSquare, twoForward, 0));
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
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_QUEEN));
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_ROOK));
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_BISHOP));
                    moveList.push_back(encodeMove(pawnSquare, captureSquare,
                                                  MOVE_FLAG_PROMOTION | MOVE_FLAG_CAPTURE | PROMOTE_TO_KNIGHT));
                } else {
                    // Normal capture
                    moveList.push_back(encodeMove(pawnSquare, captureSquare, MOVE_FLAG_CAPTURE));
                }
            }
        }

        // === 4. En Passant ===
        if (board.enPassantSquare >= 0 && board.enPassantSquare < 64) {
            int epSquare = board.enPassantSquare;
            int epFile = epSquare % 8;

            // Check if we can capture
            if (abs(epFile - pawnFile) == 1 && epSquare == pawnSquare + direction - 1) {
                moveList.push_back(encodeMove(pawnSquare, epSquare, MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE));
            } else if (abs(epFile - pawnFile) == 1 && epSquare == pawnSquare + direction + 1) {
                moveList.push_back(encodeMove(pawnSquare, epSquare, MOVE_FLAG_EN_PASSANT | MOVE_FLAG_CAPTURE));
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

bool isSquareAttacked(const Board &board, int square, int attackingColor) {
    /* Check if 'square' is attacked by pieces of 'attackingColor' */
    /* attackingColor: 1 = white, -1 = black */

    uint64_t occupied = board.getWhiteBitboard() | board.getBlackBitboard();
    int file = square % 8;
    int rank = square / 8;

    /* === Check Knight attacks === */
    const int knightOffsets[8] = {-17, -15, -10, -6, 6, 10, 15, 17};
    uint64_t enemyKnights = (attackingColor == 1) ? board.whiteKnights : board.blackKnights;

    for (int offset: knightOffsets) {
        int from = square + offset;
        if (!isValidSquare(from)) continue;

        int fromFile = from % 8;
        int fileDist = abs(fromFile - file);
        if (fileDist != 1 && fileDist != 2) continue;

        if (enemyKnights & (1ULL << from)) return true;
    }

    /* === Check King attacks === */
    const int kingOffsets[8] = {-9, -8, -7, -1, 1, 7, 8, 9};
    uint64_t enemyKing = (attackingColor == 1) ? board.whiteKing : board.blackKing;

    for (int offset: kingOffsets) {
        int from = square + offset;
        if (!isValidSquare(from)) continue;

        int fromFile = from % 8;
        if (abs(fromFile - file) > 1) continue;

        if (enemyKing & (1ULL << from)) return true;
    }

    /* === Check Pawn attacks === */
    uint64_t enemyPawns = (attackingColor == 1) ? board.whitePawns : board.blackPawns;
    int pawnDir = (attackingColor == 1) ? -8 : 8;  /* Pawns attack opposite direction */

    int leftAttack = square + pawnDir - 1;
    int rightAttack = square + pawnDir + 1;

    if (isValidSquare(leftAttack) && file > 0) {
        if (enemyPawns & (1ULL << leftAttack)) return true;
    }
    if (isValidSquare(rightAttack) && file < 7) {
        if (enemyPawns & (1ULL << rightAttack)) return true;
    }

    /* === Check Sliding pieces (Rook, Bishop, Queen) === */
    uint64_t enemyRooks = (attackingColor == 1) ? board.whiteRooks : board.blackRooks;
    uint64_t enemyBishops = (attackingColor == 1) ? board.whiteBishops : board.blackBishops;
    uint64_t enemyQueens = (attackingColor == 1) ? board.whiteQueens : board.blackQueens;

    /* Rook directions: N, S, E, W */
    const int rookDirs[4] = {8, -8, 1, -1};
    for (int dir: rookDirs) {
        int target = square + dir;
        while (isValidSquare(target)) {
            // Horizontal wrap check
            if (dir == 1 || dir == -1) {
                int fromFile = (target - dir) % 8;
                int toFile = target % 8;
                if (abs(toFile - fromFile) > 1) {
                    break;
                }
            }

            uint64_t targetMask = 1ULL << target;

            /* Hit a piece */
            if (occupied & targetMask) {
                /* Is it an attacking rook or queen? */
                if ((enemyRooks | enemyQueens) & targetMask) return true;
                break;  /* Blocked */
            }

            target += dir;
        }
    }

    /* Bishop directions: NE, NW, SE, SW */
    const int bishopDirs[4] = {9, 7, -7, -9};
    for (int dir: bishopDirs) {
        int target = square + dir;
        while (isValidSquare(target)) {
            /* Check wrap */
            int fromFile = (target - dir) % 8;
            int toFile = target % 8;
            if (abs(toFile - fromFile) > 1) {
                break;
            }

            uint64_t targetMask = 1ULL << target;

            if (occupied & targetMask) {
                if ((enemyBishops | enemyQueens) & targetMask) return true;
                break;
            }

            target += dir;
        }
    }

    return false;
}