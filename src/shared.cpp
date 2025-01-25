#include "shared.hpp"

template <typename T>
size_t fl_count(const forward_list<T> &list) {
  size_t count = 0;

  for (auto &_ : list) count++;

  return count;
};

string context(const string &context_id) { return "[" + context_id + "] "; }
