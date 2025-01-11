#include "ClocksPlugin.hpp"

#include <cstdint>

#include "state.hpp"

ClocksPlugin::ClocksPlugin() {
  init_state();
  items = item_count();
  clocks.reserve(items);

  for (uint8_t index = 0; index < items; index++) {
    clocks[index] = ClockItem{index};
  }
}

ClocksPlugin ClocksPlugin::instance;
ClocksPlugin& ClocksPlugin::Instance() { return instance; }

IPluginItem* ClocksPlugin::GetItem(int index) {
  if (index == items)
    return nullptr;
  else
    return &clocks[index];
}

void ClocksPlugin::DataRequired() {}

const wchar_t* ClocksPlugin::GetInfo(PluginInfoIndex index) {
  switch (index) {
    case TMI_NAME:
      return NAME;
    case TMI_DESCRIPTION:
      return DESCRIPTION;
    case TMI_AUTHOR:
      return AUTHOR;
    case TMI_COPYRIGHT:
      return COPYRIGHT;
    case TMI_URL:
      return URL;
      break;
    case TMI_VERSION:
      return VERSION;
    default:
      break;
  }

  return nullptr;
}
