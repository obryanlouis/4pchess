#include <cassert>
#include <optional>
#include <iostream>

#include "transposition_table.h"

namespace chess {

TranspositionTable::TranspositionTable(size_t table_size) {
  assert((table_size > 0) && "transposition table_size = 0");
  table_size_ = table_size;
  hash_table_ = (HashTableEntry*) calloc(table_size, sizeof(HashTableEntry));
  assert(
      (hash_table_ != nullptr) && 
      "Can't create transposition table. Try using a smaller size.");
}

const HashTableEntry* TranspositionTable::Get(int64_t key) {
  size_t n = key % table_size_;
  HashTableEntry* entry = hash_table_ + n;
  if (entry->key == key) {
    return entry;
  }
  return nullptr;
}

void TranspositionTable::Save(
    int64_t key, int depth, std::optional<Move> move, int score,
    ScoreBound bound, bool is_pv) {
  size_t n = key % table_size_;
  HashTableEntry& entry = hash_table_[n];
  if (bound == EXACT
      || entry.key != key
      || entry.depth < depth) {
    entry.key = key;
    entry.depth = depth;
    entry.move = move;
    entry.score = score;
    entry.bound = bound;
    entry.is_pv = is_pv;
  }
}


}  // namespace chess

