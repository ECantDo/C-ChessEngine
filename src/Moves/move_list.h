//
// Created by ECanDo on 2026-01-01.
//

#ifndef CHESSENGINE_MOVE_LIST_H
#define CHESSENGINE_MOVE_LIST_H


#include <array>
#include "Board/move.h"

const size_t MoveLimit = 256;

class MoveList {
private:
    std::array<Move, MoveLimit> moves{};
    size_t size = 0;
public:
    inline bool append(Move move){
        if (size == MoveLimit) return false;
        moves[size++] = move;
        return true;
    }

    inline void clear() {
        size = 0;
    }

    inline Move get(size_t idx){
        if (idx >= size) return 0;
        return moves[idx];
    }

    inline bool set(size_t idx, Move move){
        if (idx >= size) return false;
        moves[idx] = move;
        return true;
    }

    inline const size_t length(){
        return size;
    }

    inline const bool empty(){
        return size == 0;
    }
};


#endif //CHESSENGINE_MOVE_LIST_H
