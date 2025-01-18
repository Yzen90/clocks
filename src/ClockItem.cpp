#include "ClockItem.hpp"

ClockItem::ClockItem(Index index, const StateStore& store) : index(index), store(&store) {};

const wchar_t* ClockItem::GetItemName() const { return store->get_clock(index)->name.c_str(); }
const wchar_t* ClockItem::GetItemId() const { return store->get_clock(index)->id.c_str(); }
const wchar_t* ClockItem::GetItemLableText() const { return store->get_clock(index)->label.c_str(); }
const wchar_t* ClockItem::GetItemValueText() const { return store->get_clock(index)->time.c_str(); }
const wchar_t* ClockItem::GetItemValueSampleText() const { return store->get_clock(index)->sample.c_str(); }
