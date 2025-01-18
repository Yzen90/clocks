#pragma once

#include <forward_list>

using std::forward_list;

template <typename T>
size_t fl_count(const forward_list<T> &list) {
  size_t count = 0;

  for (auto &_ : list) count++;

  return count;
};
