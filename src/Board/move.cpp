//
// Created by ECanDo on 2025-12-03.
//

#include "move.h"

std::string moveToString(Move m) {
    std::string result = getBoardPosition(getMoveFrom(m)) + getBoardPosition(getMoveTo(m));

    if (getMoveFlags(m) & MOVE_FLAG_PROMOTION) {
        int promoPiece = getMoveFlags(m) & 0x3;
        switch (promoPiece) {
            case PROMOTE_TO_KNIGHT:
                result += "n";
                break;
            case PROMOTE_TO_BISHOP:
                result += "b";
                break;
            case PROMOTE_TO_ROOK:
                result += "r";
                break;
            case PROMOTE_TO_QUEEN:
                result += "q";
                break;
            default:
                // Do nothing, nothing else can really happen. Don't want to crash.
                break;
        }
    }

    return result;
}

Move stringToMove(std::string &str, const Board &board) {
    int from = getBoardIndex(str[0], str[1]);
    int to = getBoardIndex(str[2], str[3]);

    int flags = 0;

    char piece = board.pieceAtSquare(from);
    char target = board.pieceAtSquare(to);

    int8_t side = board.turn;

    if (target != NONE_PIECE) {
        flags |= MOVE_FLAG_CAPTURE;
    }

    // Promotion
    if (str.size() == 5) {
        flags |= MOVE_FLAG_PROMOTION;
        switch (str[4]) {
            case 'n':
                flags |= PROMOTE_TO_KNIGHT;
                break;
            case 'b':
                flags |= PROMOTE_TO_BISHOP;
                break;
            case 'r':
                flags |= PROMOTE_TO_ROOK;
                break;
            case 'q':
                flags |= PROMOTE_TO_QUEEN;
                break;
        }
    }

    // --- Detect en passant ---
    if (piece == WHITE_PAWN || piece == BLACK_PAWN) {
        if (to == board.enPassantSquare) {
            flags |= MOVE_FLAG_EN_PASSANT;
            flags |= MOVE_FLAG_CAPTURE; // must mark as capture too
        }
    }

    // --- Detect castling ---
    if (piece == WHITE_KING && from == 4) {
        if (to == 6) flags |= MOVE_FLAG_CASTLING; // e1g1
        if (to == 2) flags |= MOVE_FLAG_CASTLING; // e1c1
    }

    if (piece == BLACK_KING && from == 60) {
        if (to == 62) flags |= MOVE_FLAG_CASTLING; // e8g8
        if (to == 58) flags |= MOVE_FLAG_CASTLING; // e8c8
    }

    return encodeMove(from, to, flags);
}