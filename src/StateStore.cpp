#include "StateStore.hpp"

#include <easylogging++.h>
#include <winrt/Windows.Foundation.Collections.h>
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

using namespace winrt::Windows::Globalization;
using namespace std::filesystem;

using el::ConfigurationType;
using el::Logger;
using el::Loggers;
using nowide::widen;
using std::format;
using std::make_format_args;
using std::string;
using std::vector;
using std::vformat;
using winrt::clock;

static Logger* logger;

const ItemCount ITEM_MAX = std::numeric_limits<Index>::max() + 1;

const string EAGER_FILENAME = "clocks.dll.dat";
const path EAGER_PATH = EAGER_LOCATION / EAGER_FILENAME;
struct EagerData {
  string config_location;
};

const string CONFIG_FILENAME = "clocks.dll.json";
const string LOG_FILENAME = "clocks.dll.log";

const wstring NEXT_DAY = L"↷";
const wstring PREV_DAY = L"↶";

template <>
struct glz::meta<LogLevel> {
  using enum LogLevel;
  static constexpr auto value = enumerate(TRACE, DEBUG, VERBOSE, INFO, WARNING, ERROR, FATAL);
};

template <>
struct glz::meta<ClockType> {
  using enum ClockType;
  static constexpr auto value = enumerate(FormatAuto, Format24h, Format12h);
};

template <>
struct glz::meta<Locale> {
  using enum Locale;
  static constexpr auto value = enumerate(Auto, EN, ES);
};

const Clock DEFAULT_CLOCK{"UTC", "UTC:"};

StateStore::StateStore() {
  std::locale::global(std::locale(".utf-8"));

  use_l10n(contexts);
  use_l10n(messages);

  if (is_regular_file(EAGER_PATH)) {
    EagerData data;
    std::ignore = glz::read_file_beve_untagged(data, EAGER_PATH.string(), vector<std::byte>{});

    if (data.config_location.length() > 0) {
      state.configuration_location = path{data.config_location};
      state.eager_initialization = true;

      load_configuration({});

      logger->verbose(
          0, context(contexts->initialization) +
                 (state.eager_initialization ? messages->eager_config_load : messages->no_eager_config_load) + " " +
                 state.configuration_location.string()
      );
    }
  }
}

StateStore& StateStore::instance() {
  static StateStore instance;
  return instance;
}

void StateStore::load_configuration(optional<Configuration> configuration) {
  bool save = true;

  if (configuration) {
    state.configuration = configuration.value();
    use_configuration();
  } else {
    path configuration_path = state.configuration_location / CONFIG_FILENAME;
    set_log_file(state.configuration_location / LOG_FILENAME);

    if (exists(configuration_path)) {
      string buffer{};
      auto error = glz::read_file_json(state.configuration, configuration_path.string(), buffer);
      use_configuration();

      if (error) {
        logger->error(context(contexts->configuration) + l10n->generic.cause + " " + glz::format_error(error, buffer));
        logger->warn(context(contexts->configuration) + messages->warn_default_config);
      } else {
        if (state.configuration.clocks.empty())
          logger->warn(context(contexts->configuration) + messages->warn_no_clocks);
        else
          save = false;
      }
      buffer.clear();

    } else {
      use_configuration();
      logger->info(context(contexts->configuration) + messages->using_default_config);
    }

    if (state.item_count == 0)
      logger->info(context(contexts->initialization) + messages->config_file + " " + configuration_path.string());
  }

  refresh_time(system_clock::now());
  state.item_count = 0;
  clocks.clear();

  for (auto& clock : state.configuration.clocks) {
    if (state.item_count == ITEM_MAX) {
      logger->warn(context(contexts->configuration) + vformat(messages->too_many_clocks, make_format_args(ITEM_MAX)));
      break;
    }

    try {
      add_clock(locate_zone(clock.timezone), clock.timezone, widen(clock.label));
    } catch (...) {
      logger->warn(
          context(contexts->configuration) + vformat(messages->invalid_timezone, make_format_args(clock.timezone))
      );
    }
  }

  if (state.item_count == 0) {
    state.configuration.clocks = {DEFAULT_CLOCK};
    add_clock(locate_zone(DEFAULT_CLOCK.timezone), DEFAULT_CLOCK.timezone, widen(DEFAULT_CLOCK.label));

    if (!save) {
      save = true;
      logger->warn(context(contexts->configuration) + messages->warn_no_valid_clocks);
    }
  }

  if (save) save_configuration();

  logger->info(context(contexts->configuration) + vformat(messages->config_loaded, make_format_args(state.item_count)));
}

void StateStore::use_configuration() {
  set_log_level();
  set_locale();
  set_time_formatter();
}

void StateStore::set_log_file(path log_file) {
  el::Configurations log_config{*el::Loggers::defaultConfigurations()};
  log_config.setGlobally(ConfigurationType::Filename, log_file.string());
  el::Loggers::setDefaultConfigurations(log_config, true);
}

void StateStore::set_log_level() {
  el::Configurations log_config{*el::Loggers::defaultConfigurations()};
  log_config.setGlobally(ConfigurationType::Enabled, "false");

  switch (state.configuration.log_level) {
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

  el::Loggers::setDefaultConfigurations(log_config, true);
}

void StateStore::set_locale() {
  load_locale(state.configuration.locale);
  logger = Loggers::getLogger(l10n->state_store.logger_id);
}

void StateStore::set_time_formatter() {
  switch (state.configuration.clock_type) {
    case ClockType::FormatAuto:
      state.time_formatter = DEFAULT_TIME_FORMATTER;
      return;
    case ClockType::Format12h:
      state.time_formatter = {
          DEFAULT_TIME_FORMAT, DEFAULT_TIME_FORMATTER.Languages(), DEFAULT_TIME_FORMATTER.GeographicRegion(),
          DEFAULT_TIME_FORMATTER.Calendar(), ClockIdentifiers::TwelveHour()
      };
      return;
    case ClockType::Format24h:
      state.time_formatter = {
          DEFAULT_TIME_FORMAT, DEFAULT_TIME_FORMATTER.Languages(), DEFAULT_TIME_FORMATTER.GeographicRegion(),
          DEFAULT_TIME_FORMATTER.Calendar(), ClockIdentifiers::TwentyFourHour()
      };
      return;
  }
}

void StateStore::save_configuration() {
  auto error = glz::write_file_json<glz::opts{.prettify = true}>(
      state.configuration, (state.configuration_location / CONFIG_FILENAME).string(), string{}
  );
  if (error)
    logger->error(context(contexts->configuration) + l10n->generic.cause + " " + glz::format_error(error));
  else
    logger->info(context(contexts->configuration) + messages->config_saved);
}

void StateStore::refresh_time(const time_point<system_clock>& now) {
  if (state.configuration.show_day_difference) {
    state.time = now;
    state.winrt_time = clock::from_sys(now);

    state.local_days = sys_days{year_month_day{floor<days>(zoned_time{state.current_tz, now}.get_local_time())}};

  } else {
    state.winrt_time = clock::from_sys(now);
  }

  state.current_minutes = hh_mm_ss{now - floor<days>(now)}.minutes();
}

inline wstring StateStore::get_time(const time_zone* tz, const wstring& timezone) {
  wstring time_string{state.time_formatter.Format(state.winrt_time, timezone)};

  if (state.configuration.show_day_difference) {
    sys_days tz_days{year_month_day{floor<days>(zoned_time{tz, state.time}.get_local_time())}};

    if (tz_days != state.local_days) {
      if (tz_days > state.local_days)
        time_string.append(L" " + NEXT_DAY);
      else
        time_string.append(L" " + PREV_DAY);
    }
  }

  return time_string;
}

void StateStore::add_clock(const time_zone* tz, string timezone, wstring label) {
  auto payload = std::to_string(state.item_count) + timezone;
  auto id = widen(format("{:x}", XXH3_64bits(&payload, payload.length())));
  auto timezone_w = widen(timezone);

  auto time = get_time(tz, timezone_w);

  clocks[state.item_count] = ClockData{tz, timezone_w, id, label, L"🕜 " + timezone_w, time, time};

  state.item_count++;
}

void StateStore::set_config_dir(const wchar_t* config_dir) {
  path configuration_location{config_dir};

  if (state.eager_initialization) {
    state.eager_initialization = false;
    std::error_code ignore{};

    if (equivalent(state.configuration_location / CONFIG_FILENAME, configuration_location / CONFIG_FILENAME, ignore)) {
      logger->verbose(0, context(contexts->configuration) + messages->eager_load_confirmed);
      return;
    }
  }

  state.configuration_location = configuration_location;
  load_configuration({});

  std::ignore = glz::write_file_beve_untagged(
      EagerData{configuration_location.string()}, EAGER_PATH.string(), vector<std::byte>{}
  );
}

void StateStore::refresh() {
  auto now = system_clock::now();
  auto minutes = hh_mm_ss{now - floor<days>(now)}.minutes();

  if (minutes != state.current_minutes) {
    refresh_time(now);

    for (auto& [_, clock] : clocks) {
      clock.time = get_time(clock.tz, clock.timezone);
    }
  }
}

ClockData* StateStore::get_clock(Index index) { return &(clocks[index]); }

ItemCount StateStore::item_count() { return state.item_count; }
