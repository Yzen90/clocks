#pragma once

#include <forward_list>
#include <string>

using std::forward_list;
using std::string;

template <typename T>
size_t fl_count(const forward_list<T>& list);

string context(const string& context_id);
