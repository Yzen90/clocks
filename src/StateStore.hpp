#pragma once

#include <chrono>
#include <map>
#include <memory>

using std::map;
using std::string;
using std::unique_ptr;
using std::wstring;

using namespace std::chrono;

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
  static void save_configuration_with_default_clock();
  static void update_time_formatter();
  static void refresh_time(const time_point<system_clock>& now);
  inline static wstring get_time(const time_zone* tz, const wstring& timezone);
  static void add_clock(const time_zone* tz, string timezone, wstring label);

 public:
  StateStore();

  static void initialize(const wchar_t* config_dir);
  static void refresh();
  static ClockData* get_clock(Index index);
  static ItemCount item_count();
};
