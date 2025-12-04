//
// Created by ECanDo on 2025-12-03.
//

#include "move.h"
#include "board.h"

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