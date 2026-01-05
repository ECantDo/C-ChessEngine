//
// Created by ECanDo on 2025-12-08.
//

#include "transposition_table.h"

TranspositionTable globalTT(256);

void TranspositionTable::clear() {
    delete[] table;
    table = new TTCluster[size];
    overwrites = 0;
    stored = 0;
}
