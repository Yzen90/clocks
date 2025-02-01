#include "StateStore.hpp"

#include <easylogging++.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/base.h>
#include <xxhash.h>

#include <filesystem>
#include <format>
#include <glaze/glaze.hpp>
#include <nowide/convert.hpp>
#include <string>
#include <vector>

#include "i18n/l10n.hpp"
#include "i18n/schema.hpp"
#include "shared.hpp"

using namespace winrt::Windows::Globalization::DateTimeFormatting;
using namespace winrt::Windows::Globalization;
using namespace std::filesystem;

using el::ConfigurationType;
using el::Logger;
using el::Loggers;
using nowide::narrow;
using nowide::widen;
using std::format;
using std::make_unique;
using std::string;
using std::vector;
using winrt::clock;

static Logger* logger;

const ItemCount ITEM_MAX = std::numeric_limits<Index>::max() + 1;
const string MAX_ITEM = std::to_string(ITEM_MAX);

const path EAGER_PATH{"plugins/"};
const string EAGER_FILENAME = "clocks.dll.dat";
const path EAGER_DATA_PATH = EAGER_PATH / EAGER_FILENAME;
struct EagerData {
  string config_location;
};

const string CONFIG_FILENAME = "clocks.dll.json";
const string LOG_FILENAME = "clocks.dll.log";

const wstring NEXT_DAY = L"↷";
const wstring PREV_DAY = L"↶";

enum class LogLevel { TRACE, DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL };
template <>
struct glz::meta<LogLevel> {
  using enum LogLevel;
  static constexpr auto value = enumerate(TRACE, DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL);
};

#ifdef NOT_RELEASE_MODE
const LogLevel DEFAULT_LOG_LEVEL = LogLevel::DEBUG;
#else
const LogLevel DEFAULT_LOG_LEVEL = LogLevel::INFO;
#endif

enum class ClockType { FormatAuto, Format24h, Format12h };
template <>
struct glz::meta<ClockType> {
  using enum ClockType;
  static constexpr auto value = enumerate(FormatAuto, Format24h, Format12h);
};

struct Clock {
  string timezone;
  string label;
};

const Clock DEFAULT_CLOCK{"UTC", "UTC:"};

struct Configuration {
  ClockType clock_type = ClockType::FormatAuto;
  bool show_day_difference = true;
  forward_list<Clock> clocks;
  Locale locale = Locale::Auto;
  LogLevel log_level = DEFAULT_LOG_LEVEL;
};

const wstring DEFAULT_TIME_FORMAT = L"shorttime";
const DateTimeFormatter DEFAULT_TIME_FORMATTER{DEFAULT_TIME_FORMAT};

struct State {
  ItemCount item_count;
  path configuration_location;
  DateTimeFormatter time_formatter = DEFAULT_TIME_FORMATTER;

  time_point<system_clock> time;
  time_point<winrt::clock> winrt_time;

  const time_zone* current_tz = current_zone();
  day current_day;
  minutes current_minutes;

  Configuration configuration;
  bool eager_initialization = false;
};

unique_ptr<State> StateStore::state;
unique_ptr<Clocks> StateStore::clocks;

StateStore::StateStore() {
  std::locale::global(std::locale(".utf-8"));

  clocks = make_unique<Clocks>();
  state = make_unique<State>(State{0});

  use_l10n(contexts);
  use_l10n(messages);

  if (is_regular_file(EAGER_DATA_PATH)) {
    EagerData data;
    std::ignore = glz::read_file_beve_untagged(data, EAGER_DATA_PATH.string(), vector<std::byte>{});

    if (data.config_location.length() > 0) {
      state->configuration_location = path{data.config_location};
      state->eager_initialization = true;

      load_configuration({});

      logger->verbose(
          0, context(contexts->initialization) +
                 (state->eager_initialization ? messages->eager_config_load : messages->no_eager_config_load) + " " +
                 state->configuration_location.string()
      );
    }
  }
}

StateStore StateStore::instance;
StateStore& StateStore::Instance() { return instance; }

const Contexts* StateStore::contexts;
const Messages* StateStore::messages;

void StateStore::load_configuration(optional<Configuration> configuration) {
  if (!configuration) {
    auto configuration_path = state->configuration_location / CONFIG_FILENAME;

    if (exists(configuration_path)) {
      auto error = glz::read_file_json(state->configuration, state->configuration_path.string(), string{});
      if (error) {
        logger->error(context(contexts->config_load) + l10n->generic.cause + " " + glz::format_error(error));
        logger->warn(context(contexts->config_load) + messages->warn_default_config);
        save_configuration_with_default_clock();

      } else if (fl_count(state->configuration.clocks) == 0) {
        logger->warn(context(contexts->config_load) + messages->warn_no_clocks);
        save_configuration_with_default_clock();
      }

    } else {
      save_configuration_with_default_clock();
      logger->info(context(contexts->config_load) + messages->using_default_config);
    }
  }

  logger_config(state->configuration_location / LOG_FILENAME);
  set_locale();

  /*
  logger->info(context(contexts->initialization) + " Configuration path: " + state->configuration_path.string());


  if (state->configuration.clock_type != ClockType::FormatAuto) update_time_formatter();

  refresh_time(system_clock::now());

  for (auto& clock : state->configuration.clocks) {
    if (state->item_count == ITEM_MAX) {
      logger->warn(
          CONTEXT_STATE_INIT + " More clocks than " + std::to_string(ITEM_MAX) + " where provided, ignoring the rest."
      );
      break;
    }

    try {
      add_clock(locate_zone(clock.timezone), clock.timezone, widen(clock.label));
    } catch (...) {
      logger->warn(CONTEXT_STATE_INIT + " Ignoring clock with invalid timezone \"" + clock.timezone + "\".");
    }
  }

  if (state->item_count == 0) {
    logger->warn(
        CONTEXT_STATE_INIT + " No valid clocks were defined in the configuration, default clock will be used."
    );
    save_configuration_with_default_clock();
    add_clock(locate_zone(DEFAULT_CLOCK.timezone), DEFAULT_CLOCK.timezone, widen(DEFAULT_CLOCK.label));
  }

  state->configuration.clocks.clear();
  logger->info(CONTEXT_STATE_INIT + " Plugin state initialized with " + std::to_string(state->item_count) + " clocks.");
*/
}

void StateStore::logger_config(path log_file) {
  el::Configurations log_config{*el::Loggers::defaultConfigurations()};
  log_config.setGlobally(ConfigurationType::Enabled, "false");

  switch (state->configuration.log_level) {
    case LogLevel::TRACE:
      log_config.set(el::Level::Trace, ConfigurationType::Enabled, "true");
    case LogLevel::DEBUG:
      log_config.set(el::Level::Debug, ConfigurationType::Enabled, "true");
    case LogLevel::VERBOSE:
      log_config.set(el::Level::Verbose, ConfigurationType::Enabled, "true");
    case LogLevel::INFO:
      log_config.set(el::Level::Info, ConfigurationType::Enabled, "true");
    case LogLevel::WARNING:
      log_config.set(el::Level::Warning, ConfigurationType::Enabled, "true");
    case LogLevel::ERROR:
      log_config.set(el::Level::Error, ConfigurationType::Enabled, "true");
    case LogLevel::FATAL:
      log_config.set(el::Level::Fatal, ConfigurationType::Enabled, "true");
  }

  log_config.setGlobally(ConfigurationType::Filename, log_file.string());
  el::Loggers::setDefaultConfigurations(log_config, true);
}

void StateStore::set_locale() {
  load_locale(state->configuration.locale);
  logger = Loggers::getLogger(l10n->state_store.logger_id);
}

void StateStore::set_config_dir(const wchar_t* config_dir) {
  path configuration_location{config_dir};

  if (state->eager_initialization) {
    state->eager_initialization = false;
    std::error_code ignore{};

    if (equivalent(state->configuration_location / CONFIG_FILENAME, configuration_location / CONFIG_FILENAME, ignore)) {
      logger->verbose(0, context(contexts->config_load) + messages->eager_load_confirmed);
      return;
    }
  }

  state->configuration_location = configuration_location;
  load_configuration({});

  std::ignore = glz::write_file_beve_untagged(
      EagerData{configuration_location.string()}, EAGER_DATA_PATH.string(), vector<std::byte>{}
  );
}

void StateStore::save_configuration() {
  auto error = glz::write_file_json<glz::opts{.prettify = true}>(
      state->configuration, state->configuration_path.string(), string{}
  );
  if (error)
    logger->error(context(contexts->config_save) + l10n->generic.cause + " " + glz::format_error(error));
  else
    logger->info(context(contexts->config_save) + messages->config_saved);
}

void StateStore::save_configuration_with_default_clock() {
  state->configuration.clocks = {DEFAULT_CLOCK};
  save_configuration();
}

void StateStore::update_time_formatter() {
  switch (state->configuration.clock_type) {
    case ClockType::FormatAuto:
      state->time_formatter = DEFAULT_TIME_FORMATTER;
      return;
    case ClockType::Format12h:
      state->time_formatter = {
          DEFAULT_TIME_FORMAT, DEFAULT_TIME_FORMATTER.Languages(), DEFAULT_TIME_FORMATTER.GeographicRegion(),
          DEFAULT_TIME_FORMATTER.Calendar(), ClockIdentifiers::TwelveHour()
      };
      return;
    case ClockType::Format24h:
      state->time_formatter = {
          DEFAULT_TIME_FORMAT, DEFAULT_TIME_FORMATTER.Languages(), DEFAULT_TIME_FORMATTER.GeographicRegion(),
          DEFAULT_TIME_FORMATTER.Calendar(), ClockIdentifiers::TwentyFourHour()
      };
      return;
  }
}

void StateStore::refresh_time(const time_point<system_clock>& now) {
  if (state->configuration.show_day_difference) {
    state->time = now;
    state->winrt_time = clock::from_sys(now);

    state->current_day = year_month_day{floor<days>(zoned_time{state->current_tz, now}.get_local_time())}.day();

  } else {
    state->winrt_time = clock::from_sys(now);
  }

  state->current_minutes = hh_mm_ss{now - floor<days>(now)}.minutes();
}

inline wstring StateStore::get_time(const time_zone* tz, const wstring& timezone) {
  wstring time_string{state->time_formatter.Format(state->winrt_time, timezone)};

  if (state->configuration.show_day_difference) {
    auto day = year_month_day{floor<days>(zoned_time{tz, state->time}.get_local_time())}.day();

    if (day > state->current_day)
      time_string.append(L" " + NEXT_DAY);
    else if (day < state->current_day)
      time_string.append(L" " + PREV_DAY);
  }

  return time_string;
}

void StateStore::add_clock(const time_zone* tz, string timezone, wstring label) {
  auto payload = std::to_string(state->item_count) + timezone;
  auto id = widen(format("{:x}", XXH3_64bits(&payload, payload.length())));
  auto timezone_w = widen(timezone);

  auto time = get_time(tz, timezone_w);

  (*clocks)[state->item_count] = ClockData{tz, timezone_w, id, label, L"🕜 " + timezone_w, time, time};

  state->item_count++;
}

void StateStore::refresh() {
  auto now = system_clock::now();
  auto minutes = hh_mm_ss{now - floor<days>(now)}.minutes();

  if (minutes != state->current_minutes) {
    refresh_time(now);

    for (auto& [_, clock] : *clocks) {
      clock.time = get_time(clock.tz, clock.timezone);
    }
  }
}

ItemCount StateStore::item_count() { return state->item_count; }

ClockData* StateStore::get_clock(Index index) { return &(*clocks)[index]; }
