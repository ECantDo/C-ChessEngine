//
// Created by ECanDo on 2026-01-01.
//

#ifndef CHESSENGINE_MOVE_LIST_H
#define CHESSENGINE_MOVE_LIST_H


#include <array>
#include "Board/move.h"

constexpr size_t MoveLimit = 256;

class MoveList {
private:
	std::array<Move, MoveLimit> moves{};
	size_t size = 0;

public:
	bool append(const Move move) {
		if (size == MoveLimit) return false;
		moves[size++] = move;
		return true;
	}

	void clear() {
		size = 0;
	}

	[[nodiscard]] Move get(const size_t idx) const {
		if (idx >= size) return 0;
		return moves[idx];
	}

	[[nodiscard]] Move pop() {
		if (size == 0) return 0;
		return moves[--size];
	}

	inline bool set(const size_t idx, const Move move) {
		if (idx >= size) return false;
		moves[idx] = move;
		return true;
	}

	[[nodiscard]] size_t length() const {
		return size;
	}

	[[nodiscard]] bool empty() const {
		return size == 0;
	}
};


#endif //CHESSENGINE_MOVE_LIST_H
