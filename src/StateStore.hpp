#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>

#include "i18n/l10n.hpp"

using namespace std::chrono;

using std::map;
using std::string;
using std::unique_ptr;
using std::wstring;
using std::filesystem::path;

using Contexts = L10N::StateStore::Contexts;
using Messages = L10N::StateStore::Messages;

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

  static void load_configuration(path config_dir);
  static void save_configuration();
  static void save_configuration_with_default_clock();
  static void update_time_formatter();
  static void refresh_time(const time_point<system_clock>& now);
  inline static wstring get_time(const time_zone* tz, const wstring& timezone);
  static void add_clock(const time_zone* tz, string timezone, wstring label);

  static Contexts* contexts;
  static Messages* messages;

 public:
  StateStore();

  static void set_config_dir(const wchar_t* config_dir);
  static void refresh();
  static ClockData* get_clock(Index index);
  static ItemCount item_count();
};
