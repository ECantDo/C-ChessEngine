//
// Created by ECanDo on 2025-12-08.
//

#include "transposition_table.h"

void TranspositionTable::clear() {
    delete[] table;
    table = new TTEntry[size];
}
