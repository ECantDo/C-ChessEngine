//
// Created by ECanDo on 2025-12-04.
//

#include "generate_moves.h"

void generatePseudoLegalMoves(Board &board, std::vector<Move> &moveList) {
    moveList.clear();
    moveList.reserve(MAX_MOVES);

    // Generate King moves
    generateKingMoves(board, moveList);

    // Generate Rook moves
    generateRookMoves(board, moveList);

    // Generate Bishop moves
    generateBishopMoves(board, moveList);

    // Generate Queen moves

    // Generate Knight moves

    // Generate Pawn moves
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

    // Same as log2(x), but we know x is a power of 2
    int kingSquare = std::countr_zero(kingBitBoard);

    // Generate general, normal moves
    for (int offset: kingOffsets) {
        int targetSquare = kingSquare + offset;

        // Stop if off the board
        if (!isValidSquare(targetSquare)) {
            continue;
        }

        // Stop wrapping around the board
        int fromFile = kingSquare % 8;
        int toFile = targetSquare % 8;
        if (abs(toFile - fromFile) > 1) {
            continue;
        }

        uint64_t targetMask = 1ULL << targetSquare;

        // If it intersects with one of my pieces, stop
        if (targetMask & myPieces) {
            continue;
        }

        int flags = 0;
        // If it intersects with opposing pieces, it's a capture move
        if (targetMask & theirPieces) {
            flags = MOVE_FLAG_CAPTURE;
        }

        // Otherwise it is pseudo-legal, add to move list
        moveList.push_back(encodeMove(kingSquare, targetSquare, flags));
    }

    uint64_t allPieceBitboard = myPieces | theirPieces;
    // White Castling moves
    if (board.turn == 1 && kingSquare == 4) {
        if ((board.castling & 0b1000) && (0x60 & allPieceBitboard) == 0) { // White kingside
            moveList.push_back(encodeMove(4, 6, MOVE_FLAG_CASTLING));
        }
        if ((board.castling & 0b0100) && (0x0E & allPieceBitboard) == 0) { // White queen side
            moveList.push_back(encodeMove(4, 2, MOVE_FLAG_CASTLING));
        }
    } else if (kingSquare == 60) { // Black Castling moves
        if ((board.castling & 0b0010) && (0x6000000000000000 & allPieceBitboard) == 0) { // Black kingside
            moveList.push_back(encodeMove(60, 62, MOVE_FLAG_CASTLING));
        }
        if ((board.castling & 0b0001) && (0x0E00000000000000 & allPieceBitboard) == 0) { // Black queen side
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

        for (int dir : directions) {
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