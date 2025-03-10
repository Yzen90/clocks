#include "StateStore.hpp"

#include <xxhash.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#include <winrt/Windows.Foundation.Collections.h>
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++26-extensions"
#include <glaze/glaze.hpp>
#pragma clang diagnostic pop
#include <format>
#include <iomanip>
#include <nowide/convert.hpp>
#include <sstream>

#include "i18n/l10n.hpp"
#include "i18n/schema.hpp"
#include "shared.hpp"

using namespace winrt::Windows::Globalization;
using namespace std::filesystem;

using el::ConfigurationType;

using nowide::narrow;
using nowide::widen;
using std::format;
using std::make_format_args;
using std::nullopt;
using std::ostringstream;
using std::string;
using std::to_string;
using std::vformat;
using winrt::clock;

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
struct glz::meta<Theme> {
  using enum Theme;
  static constexpr auto value = enumerate(Auto, Dark, Light);
};

template <>
struct glz::meta<ClockType> {
  using enum ClockType;
  static constexpr auto value = enumerate(FormatAuto, Format24h, Format12h);
};

template <>
struct glz::meta<Locale> {
  using enum Locale;
  static constexpr auto value = enumerate(Auto, EN, ES, JA, ZH);
};

StateStore::StateStore() {
  std::locale::global(std::locale(".utf-8"));

  use_l10n(contexts);
  use_l10n(messages);
}

StateStore& StateStore::instance() {
  static StateStore instance;
  return instance;
}

void StateStore::load_configuration(optional<Configuration> configuration) {
  if (configuration) {
    state.configuration = *configuration;
    use_configuration();
  } else {
    path configuration_path = state.configuration_location / CONFIG_FILENAME;
    set_log_file(state.configuration_location / LOG_FILENAME);

    if (exists(configuration_path)) {
      string buffer{};
      auto error = glz::read_file_json<glz::opts{.error_on_unknown_keys = false}>(
          state.configuration, configuration_path.string(), buffer
      );
      use_configuration();

      if (error) {
        logger->error(context(contexts->configuration) + l10n->generic.cause + " " + glz::format_error(error, buffer));
        logger->warn(context(contexts->configuration) + messages->warn_default_config);
      } else {
        if (state.configuration.clocks.empty())
          logger->warn(context(contexts->configuration) + messages->warn_no_clocks);
      }

    } else {
      use_configuration();
      logger->info(context(contexts->configuration) + messages->using_default_config);

      first_time = true;
    }

    if (state.clock_count == 0)
      logger->info(context(contexts->initialization) + messages->config_file + " " + configuration_path.string());
  }

  refresh_time(system_clock::now());
  state.clock_count = 0;
  clocks.clear();

  for (auto& clock : state.configuration.clocks) {
    if (state.clock_count == ITEM_MAX) {
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

  if (state.clock_count == 0) {
    if (!state.configuration.clocks.empty())
      logger->warn(context(contexts->configuration) + messages->warn_no_valid_clocks);

    state.configuration.clocks = {DEFAULT_CLOCK};
    add_clock(locate_zone(DEFAULT_CLOCK.timezone), DEFAULT_CLOCK.timezone, widen(DEFAULT_CLOCK.label));
  }

  save_configuration();
  logger->info(
      context(contexts->configuration) + vformat(messages->config_loaded, make_format_args(state.clock_count))
  );
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
  dummy_clock.id = widen(l10n->state_store.dummy_clock);
  dummy_clock.label = widen(l10n->state_store.dummy_clock);
  dummy_clock.name = widen(l10n->state_store.dummy_clock);
  dummy_clock.sample = widen(l10n->state_store.dummy_clock);
  dummy_clock.time = L"\u26D4\u26D4\u26D4\u26D4";
}

void StateStore::set_time_formatter() { state.time_formatter = get_time_formatter(state.configuration.clock_type); }

DateTimeFormatter StateStore::get_time_formatter(ClockType clock_type) {
  switch (clock_type) {
    case ClockType::FormatAuto:
      return DEFAULT_TIME_FORMATTER;

    case ClockType::Format12h:
      return {
          DEFAULT_TIME_FORMAT, DEFAULT_TIME_FORMATTER.Languages(), DEFAULT_TIME_FORMATTER.GeographicRegion(),
          DEFAULT_TIME_FORMATTER.Calendar(), ClockIdentifiers::TwelveHour()
      };

    case ClockType::Format24h:
      return {
          DEFAULT_TIME_FORMAT, DEFAULT_TIME_FORMATTER.Languages(), DEFAULT_TIME_FORMATTER.GeographicRegion(),
          DEFAULT_TIME_FORMATTER.Calendar(), ClockIdentifiers::TwentyFourHour()
      };
  }
}

void StateStore::save_configuration() {
  auto error = glz::write_file_json<glz::opts{.prettify = true, .indentation_width = 2}>(
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

wstring StateStore::get_time(const time_zone*& tz, const wstring& timezone) {
  return get_time(
      tz, timezone, state.configuration, state.time_formatter, state.time, state.winrt_time, state.local_days
  );
}

[[clang::always_inline]] inline wstring StateStore::get_time_inline(const time_zone*& tz, const wstring& timezone) {
  [[clang::always_inline]] return get_time(
      tz, timezone, state.configuration, state.time_formatter, state.time, state.winrt_time, state.local_days
  );
}

wstring StateStore::get_time(
    const time_zone*& tz, const wstring& timezone, const Configuration& configuration,
    const DateTimeFormatter& formatter, const TimeSystem& time, const TimeWinRT& winrt_time, const sys_days& local_days
) {
  wstring time_string{formatter.Format(winrt_time, timezone)};
  // Remove the U+200E "Left-To-Right Mark" character
  time_string.erase(remove(time_string.begin(), time_string.end(), L'\u200E'), time_string.end());

  if (configuration.show_day_difference) {
    sys_days tz_days{year_month_day{floor<days>(zoned_time{tz, time}.get_local_time())}};

    if (tz_days != local_days) {
      if (tz_days > local_days)
        time_string.append(L" " + NEXT_DAY);
      else
        time_string.append(L" " + PREV_DAY);
    }
  }

  return time_string;
}

void StateStore::add_clock(const time_zone* tz, string timezone, wstring label) {
  auto payload = std::to_string(state.clock_count) + timezone;
  auto id = widen(format("{:x}", XXH3_64bits(payload.data(), payload.size())));
  auto timezone_w = widen(timezone);

  auto time = get_time(tz, timezone_w);

  clocks[state.clock_count] = ClockData{tz, timezone_w, id, label, L"🕜" + timezone_w, time, time};

  state.clock_count++;
}

void StateStore::set_config_dir(const wchar_t* config_dir) {
  state.configuration_location = path{config_dir};
  load_configuration(nullopt);
}

string StateStore::get_log_file() { return (state.configuration_location / LOG_FILENAME).string(); }

void StateStore::refresh() {
  auto now = system_clock::now();
  auto minutes = hh_mm_ss{now - floor<days>(now)}.minutes();

  if (minutes != state.current_minutes) {
    refresh_time(now);

    for (auto& [_, clock] : clocks) {
      clock.time = get_time_inline(clock.tz, clock.timezone);
    }
  }
}

ClockData* StateStore::get_clock(Index index) { return index < state.clock_count ? &(clocks[index]) : &dummy_clock; }

ClockCount StateStore::clock_count() { return state.clock_count; }

string get_code_points(const wstring& ws) {
  ostringstream result;
  result.imbue(std::locale("C"));

  size_t i = 0;
  while (i < ws.size()) {
    wchar_t current = ws[i];
    bool processedSurrogate = false;

    // Check if wchar_t is 16-bit (UTF-16) and handle surrogate pairs
    if (sizeof(wchar_t) == 2) {
      if (current >= 0xD800 && current <= 0xDBFF) {  // High surrogate
        if (i + 1 < ws.size()) {
          wchar_t next = ws[i + 1];
          if (next >= 0xDC00 && next <= 0xDFFF) {  // Low surrogate
            uint32_t codePoint = 0x10000 + ((current - 0xD800) << 10) + (next - 0xDC00);
            result << "U+" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << codePoint << " ";
            i += 2;
            processedSurrogate = true;
          }
        }
      }
    }

    if (!processedSurrogate) {
      uint32_t codePoint = static_cast<uint32_t>(current);
      result << "U+" << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << codePoint << " ";
      ++i;
    }
  }

  string output = result.str();
  if (!output.empty()) {
    output.pop_back();  // Remove trailing space
  }
  return output;
}

Configuration StateStore::get_configuration() {
  if (state.configuration.log_level == LogLevel::DEBUG) {
    for (auto& [index, clock] : clocks) {
      logger->debug(
          to_string(index) + " - " + narrow(clock.label) + " [" + narrow(clock.time) + "] " +
          get_code_points(clock.time)
      );
    }
  }

  Configuration copy = state.configuration;
  return copy;
}

void StateStore::set_configuration(Configuration configuration) { load_configuration({configuration}); }
