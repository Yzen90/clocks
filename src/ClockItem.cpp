#include "ClockItem.hpp"

ClockItem::ClockItem(Index index, StateStore& state) : index(index), state(&state) {};

const wchar_t* ClockItem::GetItemName() const { return state->get_clock(index)->name.c_str(); }
const wchar_t* ClockItem::GetItemId() const { return state->get_clock(index)->id.c_str(); }
const wchar_t* ClockItem::GetItemLableText() const { return state->get_clock(index)->label.c_str(); }
const wchar_t* ClockItem::GetItemValueText() const { return state->get_clock(index)->time.c_str(); }
const wchar_t* ClockItem::GetItemValueSampleText() const { return state->get_clock(index)->sample.c_str(); }
