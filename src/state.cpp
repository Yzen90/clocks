#include "state.hpp"

// Internal

struct SampleText {
  const wchar_t* text;
};

struct PluginState {
  int item_count;
  SampleText sample;
};

static unique_ptr<PluginState> state{new PluginState{0}};
static unique_ptr<map<uint8_t, ClockData>> clocks{};

void add_clock(string time_zone, wstring label) {
  auto payload = to_string(state->item_count) + time_zone;
  auto id = widen(format("{:x}", XXH3_64bits(&payload, payload.length())));

  (*clocks)[state->item_count] =
      ClockData{time_zone, id.c_str(), label.c_str(), (L"🕜" + widen(time_zone)).c_str(), L""};

  state->item_count++;
}

// Shared

void init_state() { add_clock("UTC", L"UTC"); }

int item_count() { return state->item_count; }
const wchar_t* sample_text() { return state->sample.text; }

ClockData* get_item(uint8_t index) { return &(*clocks)[index]; }
