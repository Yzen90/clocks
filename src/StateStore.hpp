#pragma once

#define XXH_INLINE_ALL
#include <xxhash.h>

#include <boost/nowide/convert.hpp>
#include <format>
#include <map>
#include <memory>
#include <string>

using boost::nowide::widen;
using std::format;
using std::make_unique;
using std::map;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::wstring;

struct State;

struct ClockData {
  string time_zone;
  const wchar_t* id;
  const wchar_t* label;
  const wchar_t* name;
  const wchar_t* time;
};

typedef unsigned char Index;
typedef map<Index, ClockData> Clocks;

class StateStore {
 private:
  static unique_ptr<State> state;
  static unique_ptr<Clocks> clocks;

  void add_clock(string time_zone, wstring label);

 public:
  StateStore();

  unsigned short int item_count() const;
  const wchar_t* sample_text() const;

  ClockData* get_item(Index index) const;
};
