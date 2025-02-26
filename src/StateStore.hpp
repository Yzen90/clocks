#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#pragma clang diagnostic pop
#include <easylogging++.h>

#include <chrono>
#include <filesystem>
#include <map>

#include "i18n/l10n.hpp"

using namespace std::chrono;
using namespace winrt::Windows::Globalization::DateTimeFormatting;

using el::Logger;
using el::Loggers;
using std::map;
using std::optional;
using std::string;
using std::unique_ptr;
using std::vector;
using std::wstring;
using std::filesystem::path;

using Contexts = L10N::StateStore::Contexts;
using Messages = L10N::StateStore::Messages;

const wstring DEFAULT_TIME_FORMAT = L"shorttime";
const DateTimeFormatter DEFAULT_TIME_FORMATTER{DEFAULT_TIME_FORMAT};

enum class ClockType { FormatAuto, Format24h, Format12h };

struct Clock {
  string timezone;
  string label;
};

const Clock DEFAULT_CLOCK{"UTC", "UTC:"};

enum class LogLevel { TRACE, DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL };
#ifdef NOT_RELEASE_MODE
const LogLevel DEFAULT_LOG_LEVEL = LogLevel::DEBUG;
#else
const LogLevel DEFAULT_LOG_LEVEL = LogLevel::INFO;
#endif

enum class Theme { Auto, Dark, Light };

struct Configuration {
  ClockType clock_type = ClockType::FormatAuto;
  bool show_day_difference = true;
  Locale locale = Locale::Auto;
  Theme theme = Theme::Auto;
  LogLevel log_level = DEFAULT_LOG_LEVEL;
  vector<Clock> clocks;
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
typedef unsigned short ClockCount;

const constexpr ClockCount ITEM_MAX = std::numeric_limits<Index>::max() + 1;

typedef time_point<system_clock> TimeSystem;
typedef time_point<winrt::clock> TimeWinRT;

class StateStore {
 private:
  StateStore();
  StateStore(const StateStore&) = delete;
  StateStore& operator=(const StateStore&) = delete;

  Logger* logger = Loggers::getLogger("StateStore");
  const Contexts* contexts;
  const Messages* messages;

  struct State {
    ClockCount clock_count = 0;
    path configuration_location;
    DateTimeFormatter time_formatter = DEFAULT_TIME_FORMATTER;

    TimeSystem time;
    TimeWinRT winrt_time;

    const time_zone* current_tz = current_zone();
    sys_days local_days;
    minutes current_minutes;

    Configuration configuration;
  } state;

  map<Index, ClockData> clocks;
  ClockData dummy_clock;

  void set_log_file(path log_file);
  void set_log_level();
  void set_locale();
  void set_time_formatter();

  void load_configuration(optional<Configuration> configuration);
  void use_configuration();
  void save_configuration();

  wstring get_time(const time_zone*& tz, const wstring& timezone);
  inline wstring get_time_inline(const time_zone*& tz, const wstring& timezone);

  void add_clock(const time_zone* tz, string timezone, wstring label);
  void refresh_time(const time_point<system_clock>& now);

 public:
  static StateStore& instance();

  bool first_time = false;

  void set_config_dir(const wchar_t* config_dir);
  void refresh();
  ClockData* get_clock(Index index);
  ClockCount clock_count();
  Configuration get_configuration();
  void set_configuration(Configuration configuration);
  string get_log_file();

  static wstring get_time(
      const time_zone*& tz, const wstring& timezone, const Configuration& configuration,
      const DateTimeFormatter& formatter, const TimeSystem& time, const TimeWinRT& winrt_time,
      const sys_days& local_days
  );
  static DateTimeFormatter get_time_formatter(ClockType clock_type);
};
