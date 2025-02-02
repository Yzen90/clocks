#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>

#include "i18n/l10n.hpp"

using namespace std::chrono;

using std::forward_list;
using std::map;
using std::optional;
using std::string;
using std::unique_ptr;
using std::wstring;
using std::filesystem::path;

using Contexts = L10N::StateStore::Contexts;
using Messages = L10N::StateStore::Messages;

struct State;

enum class ClockType { FormatAuto, Format24h, Format12h };

struct Clock {
  string timezone;
  string label;
};

enum class LogLevel { TRACE, DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL };
#ifdef NOT_RELEASE_MODE
const LogLevel DEFAULT_LOG_LEVEL = LogLevel::DEBUG;
#else
const LogLevel DEFAULT_LOG_LEVEL = LogLevel::INFO;
#endif

struct Configuration {
  ClockType clock_type = ClockType::FormatAuto;
  bool show_day_difference = true;
  forward_list<Clock> clocks;
  Locale locale = Locale::Auto;
  LogLevel log_level = DEFAULT_LOG_LEVEL;
};

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
  StateStore();
  static StateStore instance;

  static unique_ptr<State> state;
  static unique_ptr<Clocks> clocks;

  static void set_log_file(path log_file);
  static void set_log_level();
  static void set_locale();
  static void set_time_formatter();

  static void load_configuration(optional<Configuration> configuration);
  static void use_configuration();
  static void save_configuration();

  static void refresh_time(const time_point<system_clock>& now);
  inline static wstring get_time(const time_zone* tz, const wstring& timezone);
  static void add_clock(const time_zone* tz, string timezone, wstring label);

  static const Contexts* contexts;
  static const Messages* messages;

 public:
  static StateStore& Instance();

  static void set_config_dir(const wchar_t* config_dir);
  static void refresh();
  static ClockData* get_clock(Index index);
  static ItemCount item_count();
};
