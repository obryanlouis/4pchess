#include <cassert>
#include <optional>
#include <iostream>

#include "transposition_table.h"

namespace chess {

const HashTableEntry* TranspositionTable::Get(uint64_t key) const {
    size_t n = key % table_size_;
    const HashTableEntry& entry = hash_table_[n];
    if (entry.key == key && entry.generation == currentGeneration) {
        return &entry;
    }
    return nullptr;
}

void TranspositionTable::Save(uint64_t key, int depth, std::optional<Move> move, int score, ScoreBound bound, int is_pv) {
    size_t n = key % table_size_;
    HashTableEntry& entry = hash_table_[n];
    if (entry.key == 0 || entry.generation < currentGeneration ||
       (entry.generation == currentGeneration && entry.depth < depth)) {
        entry.key = key;
        entry.depth = depth;
        entry.move = move;
        entry.score = score;
        entry.bound = bound;
        entry.is_pv = is_pv;
        entry.generation = currentGeneration;
    }
}

void TranspositionTable::clear() {
    std::fill(hash_table_.begin(), hash_table_.end(), HashTableEntry{});
}

void TranspositionTable::newSearch() {
    currentGeneration++;
}

}  // namespace chess

