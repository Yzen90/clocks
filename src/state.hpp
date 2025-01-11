#pragma once

#include <xxhash.h>

#include <boost/nowide/convert.hpp>
#include <cstdint>
#include <format>
#include <map>
#include <memory>
#include <string>

using boost::nowide::widen;
using std::format;
using std::map;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::wstring;

struct ClockData {
  string time_zone;
  const wchar_t* id;
  const wchar_t* label;
  const wchar_t* name;
  const wchar_t* time;
};

void init_state();
int item_count();
const wchar_t* sample_text();

ClockData* get_item(uint8_t index);
