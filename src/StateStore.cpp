#include "StateStore.hpp"

#include <iostream>

using boost::nowide::narrow;
using std::cout;
using std::endl;

struct State {
  unsigned short int item_count;
  const wchar_t* sample_text;
};

StateStore::StateStore() {
  if (!state) {
    state = make_unique<State>(State{0, L"Colabola"});
    clocks = make_unique<Clocks>();

    add_clock("UTC", L"UTC");
  }
}

void StateStore::add_clock(string time_zone, wstring label) {
  auto payload = to_string(state->item_count) + time_zone;
  auto id = widen(format("{:x}", XXH3_64bits(&payload, payload.length())));

  cout << payload << endl;
  cout << narrow(id) << endl;

  (*clocks)[state->item_count] =
      ClockData{time_zone, id.c_str(), label.c_str(), (L"🕜" + widen(time_zone)).c_str(), L""};

  state->item_count++;
}

unsigned short int StateStore::item_count() const { return state->item_count; }
const wchar_t* StateStore::sample_text() const { return state->sample_text; }

ClockData* StateStore::get_item(uint8_t index) const { return &(*clocks)[index]; }
