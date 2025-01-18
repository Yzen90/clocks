#pragma once

#include <easylogging++.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/base.h>

#define XXH_INLINE_ALL
#include <xxhash.h>

#include <filesystem>
#include <nowide/convert.hpp>

using el::Logger;
using el::Loggers;
using nowide::narrow;
using nowide::widen;
using std::format;
using std::forward_list;
using std::make_unique;
using std::map;
using std::string;
using std::unique_ptr;
using std::wstring;
using std::filesystem::exists;
using std::filesystem::path;
using winrt::clock;

using namespace std::chrono;
using namespace winrt::Windows::Globalization::DateTimeFormatting;

struct Configuration;
struct State;

struct ClockData {
  const time_zone* tz;
  wstring timezone;
  wstring id;
  wstring label;
  wstring name;
  wstring time;
  wstring sample;
};

typedef unsigned char Index;
typedef unsigned short ItemCount;
typedef map<Index, ClockData> Clocks;

class StateStore {
 private:
  static unique_ptr<State> state;
  static unique_ptr<Clocks> clocks;

  static void save_configuration();
  static void add_clock(const time_zone* tz, wstring time_zone, wstring label);

 public:
  StateStore();

  static void initialize(const wchar_t* config_dir);
  static void refresh();
  static ClockData* get_item(Index index);
  static ItemCount item_count();
};

const string CONFIG_FILENAME = "clocks.dll.json";

const string CONTEXT_STATE_INIT = "[Initialization]";
const string CONTEXT_CONFIG_READ = "[Configuration read]";
const string CONTEXT_CONFIG_SAVE = "[Configuration save]";

const wstring NEXT_DAY = L"↷";
const wstring PREV_DAY = L"↶";
