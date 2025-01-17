#include "StateStore.hpp"

#include <glaze/glaze.hpp>

static Logger* logger = Loggers::getLogger("state");

enum class ClockType { FormatAuto, Format24h, Format12h };
template <>
struct glz::meta<ClockType> {
  using enum ClockType;
  static constexpr auto value = enumerate(FormatAuto, Format24h, Format12h);
};

struct Configuration {
  ClockType clock_type = ClockType::FormatAuto;
  bool show_day_difference = true;
};

struct State {
  ItemCount item_count;
  path configuration_path;

  Configuration configuration;
};

StateStore::StateStore() { std::locale::global(std::locale(".utf-8")); }

unique_ptr<State> StateStore::state;
unique_ptr<Clocks> StateStore::clocks;

void StateStore::initialize(const wchar_t* config_dir) {
  state = make_unique<State>(State{0, path{config_dir} / CONFIG_FILENAME});
  logger->info(CONTEXT_STATE_INIT + " Configuration path: " + state->configuration_path.string());

  if (exists(state->configuration_path)) {
    auto error = glz::read_file_json(state->configuration, state->configuration_path.string(), string{});
    if (error) {
      logger->error(CONTEXT_CONFIG_READ + " Cause: " + glz::format_error(error));
      logger->warn(CONTEXT_STATE_INIT + " Default configuration values will be used.");
      save_configuration();
    }
  } else {
    save_configuration();
    logger->info(CONTEXT_STATE_INIT + " Using default configuration.");
  }

  clocks = make_unique<Clocks>();
  add_clock(locate_zone("UTC"), L"UTC", L"UTC:");

  logger->info(CONTEXT_STATE_INIT + " Plugin state initialized.");
}

void StateStore::save_configuration() {
  auto error = glz::write_file_json<glz::opts{.prettify = true}>(state->configuration,
                                                                 state->configuration_path.string(), string{});
  if (error)
    logger->error(CONTEXT_CONFIG_SAVE + " Cause: " + glz::format_error(error));
  else
    logger->info(CONTEXT_CONFIG_SAVE + " The configuration was saved.");
}

void StateStore::add_clock(const time_zone* tz, wstring timezone, wstring label) {
  auto payload = std::to_string(state->item_count) + narrow(timezone);
  auto id = widen(format("{:x}", XXH3_64bits(&payload, payload.length())));

  DateTimeFormatter formatter{L"shorttime"};
  auto output = formatter.Format(clock::now());

  (*clocks)[state->item_count] = ClockData{tz, timezone, id, label, L"🕜 " + timezone, wstring(output), L"--:--"};

  state->item_count++;
}

ItemCount StateStore::item_count() { return state->item_count; }

ClockData* StateStore::get_item(Index index) { return &(*clocks)[index]; }

void StateStore::refresh() {
  auto now = system_clock::now();
  auto minutes = hh_mm_ss{now - floor<days>(now)}.minutes();

  for (auto& [_, clock] : *clocks) {
    const zoned_time zoned_dt{clock.tz, now};
    auto xd = format("{:%X}", zoned_dt);
  }
}

/* inline wstring format_time(const State& state, const wstring& time_zone, const time_point<system_clock>& time) {
  const zoned_time zoned{tz, now};

  if (state.show_24h)
    return widen(format("%X", zoned));
  else
    return widen(format("%r", zoned));
} */
