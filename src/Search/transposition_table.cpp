//
// Created by ECanDo on 2025-12-08.
//

#include "transposition_table.h"

TranspositionTable globalTT(256);

void TranspositionTable::clear() {
	std::destroy_n(table, size);
	std::free(table);
	table = static_cast<TTCluster*>(std::aligned_alloc(64, size * sizeof(TTCluster)));
	new (table) TTCluster[size];
	overwrites = 0;
	stored = 0;
}

