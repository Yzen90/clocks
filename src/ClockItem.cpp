#include "ClockItem.hpp"

#include "state.hpp"

ClockItem::ClockItem(uint8_t index) { this->index = index; }

const wchar_t* ClockItem::GetItemName() const { return get_item(index)->name; }
const wchar_t* ClockItem::GetItemId() const { return get_item(index)->id; }
const wchar_t* ClockItem::GetItemLableText() const { return get_item(index)->label; }
const wchar_t* ClockItem::GetItemValueText() const { return get_item(index)->time; }
const wchar_t* ClockItem::GetItemValueSampleText() const { return sample_text(); }
