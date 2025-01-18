#include "StateStore.hpp"

#include <filesystem>
#include <string>

#include "utils.hpp"

#define XXH_INLINE_ALL
#include <easylogging++.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <winrt/base.h>
#include <xxhash.h>

#include <glaze/glaze.hpp>
#include <nowide/convert.hpp>

using el::Logger;
using el::Loggers;
using nowide::widen;
using std::format;
using std::make_unique;
using std::string;
using std::filesystem::exists;
using std::filesystem::path;
using winrt::clock;

using namespace winrt::Windows::Globalization::DateTimeFormatting;
using namespace winrt::Windows::Globalization;

static Logger* logger = Loggers::getLogger("state");

const ItemCount ITEM_MAX = std::numeric_limits<Index>::max() + 1;

const string CONFIG_FILENAME = "clocks.dll.json";

const string CONTEXT_STATE_INIT = "[Initialization]";
const string CONTEXT_CONFIG_READ = "[Configuration read]";
const string CONTEXT_CONFIG_SAVE = "[Configuration save]";

const wstring NEXT_DAY = L"↷";
const wstring PREV_DAY = L"↶";

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
};

const wstring DEFAULT_TIME_FORMAT = L"shorttime";
const DateTimeFormatter DEFAULT_TIME_FORMATTER{DEFAULT_TIME_FORMAT};

struct State {
  ItemCount item_count;
  path configuration_path;
  DateTimeFormatter time_formatter = DEFAULT_TIME_FORMATTER;

  time_point<winrt::clock> win_time;
  time_point<system_clock> time;

  const time_zone* current_tz = current_zone();
  day current_day;
  minutes current_minutes;

  Configuration configuration;
};

StateStore::StateStore() { std::locale::global(std::locale(".utf-8")); }

unique_ptr<State> StateStore::state;
unique_ptr<Clocks> StateStore::clocks;

void StateStore::initialize(const wchar_t* config_dir) {
  clocks = make_unique<Clocks>();
  state = make_unique<State>(State{0, (path{config_dir} / CONFIG_FILENAME)});
  logger->info(CONTEXT_STATE_INIT + " Configuration path: " + state->configuration_path.string());

  if (exists(state->configuration_path)) {
    auto error = glz::read_file_json(state->configuration, state->configuration_path.string(), string{});
    if (error) {
      logger->error(CONTEXT_CONFIG_READ + " Cause: " + glz::format_error(error));
      logger->warn(CONTEXT_STATE_INIT + " Default configuration values will be used.");
      save_configuration_with_default_clock();
    } else if (fl_count(state->configuration.clocks) == 0) {
      logger->warn(CONTEXT_CONFIG_READ + " No clocks were defined in the configuration, default clock will be used.");
      save_configuration_with_default_clock();
    }
  } else {
    save_configuration_with_default_clock();
    logger->info(CONTEXT_STATE_INIT + " Using default configuration.");
  }

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
  logger->info(CONTEXT_STATE_INIT + " Plugin state initialized.");
}

void StateStore::save_configuration() {
  auto error = glz::write_file_json<glz::opts{.prettify = true}>(
      state->configuration, state->configuration_path.string(), string{}
  );
  if (error)
    logger->error(CONTEXT_CONFIG_SAVE + " Cause: " + glz::format_error(error));
  else
    logger->info(CONTEXT_CONFIG_SAVE + " The configuration was saved.");
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
    state->win_time = clock::from_sys(now);

    state->current_day = year_month_day{floor<days>(zoned_time{state->current_tz, now}.get_local_time())}.day();

  } else {
    state->win_time = clock::from_sys(now);
  }

  state->current_minutes = hh_mm_ss{now - floor<days>(now)}.minutes();
}

inline wstring StateStore::get_time(const wstring& timezone) {
  wstring time_string{state->time_formatter.Format(state->win_time, timezone)};

  if (state->configuration.show_day_difference) {
  }

  return time_string;
}

void StateStore::add_clock(const time_zone* tz, string timezone, wstring label) {
  auto payload = std::to_string(state->item_count) + timezone;
  auto id = widen(format("{:x}", XXH3_64bits(&payload, payload.length())));
  auto timezone_w = widen(timezone);

  auto time = get_time(timezone_w);

  (*clocks)[state->item_count] = ClockData{tz, timezone_w, id, label, L"🕜 " + timezone_w, time, time};

  state->item_count++;
}

void StateStore::refresh() {
  auto now = system_clock::now();
  auto minutes = hh_mm_ss{now - floor<days>(now)}.minutes();

  if (minutes != state->current_minutes) {
    refresh_time(now);
  }
}

ItemCount StateStore::item_count() { return state->item_count; }

ClockData* StateStore::get_clock(Index index) { return &(*clocks)[index]; }
