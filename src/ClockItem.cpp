#include "ClockItem.hpp"

ClockItem::ClockItem(Index index, StateStore& state) : index(index), state(&state) {};

const wchar_t* ClockItem::GetItemName() const { return state->get_clock(index)->name.data(); }
const wchar_t* ClockItem::GetItemId() const { return state->get_clock(index)->id.data(); }
const wchar_t* ClockItem::GetItemLableText() const { return state->get_clock(index)->label.data(); }
const wchar_t* ClockItem::GetItemValueText() const { return state->get_clock(index)->time.data(); }
const wchar_t* ClockItem::GetItemValueSampleText() const { return state->get_clock(index)->sample.data(); }
