//
// Created by ECanDo on 2025-12-08.
//

#include "transposition_table.h"

#include <cstring>

TranspositionTable globalTT(16);

void TranspositionTable::clear() {
	memset(table, 0, size * sizeof(TTCluster));
	overwrites = 0;
	stored = 0;
}

