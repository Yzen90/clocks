#pragma once

#include <PluginInterface.h>

#include "StateStore.hpp"

class ClockItem : public IPluginItem {
 private:
  Index index;
  StateStore* state;

 public:
  ClockItem(Index index, StateStore& state);

  virtual const wchar_t* GetItemName() const override;
  virtual const wchar_t* GetItemId() const override;
  virtual const wchar_t* GetItemLableText() const override;
  virtual const wchar_t* GetItemValueText() const override;
  virtual const wchar_t* GetItemValueSampleText() const override;
};
