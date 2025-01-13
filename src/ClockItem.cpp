#include "ClockItem.hpp"

ClockItem::ClockItem(Index index, const StateStore& store) : index(index), store(store) {};

const wchar_t* ClockItem::GetItemName() const { return store.get_item(index)->name; }
const wchar_t* ClockItem::GetItemId() const { return store.get_item(index)->id; }
const wchar_t* ClockItem::GetItemLableText() const { return store.get_item(index)->label; }
const wchar_t* ClockItem::GetItemValueText() const { return store.get_item(index)->time; }
const wchar_t* ClockItem::GetItemValueSampleText() const { return store.sample_text(); }
