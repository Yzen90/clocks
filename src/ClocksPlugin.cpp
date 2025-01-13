#include "ClocksPlugin.hpp"

unique_ptr<StateStore> ClocksPlugin::state{};

ClocksPlugin::ClocksPlugin() {
  item_count = state->item_count();
  clocks.reserve(item_count);

  for (Index index = 0; index < item_count; index++) {
    clocks.push_back(ClockItem{index, *state});
  }
}

ClocksPlugin ClocksPlugin::instance;
ClocksPlugin& ClocksPlugin::Instance() { return instance; }

IPluginItem* ClocksPlugin::GetItem(int index) {
  if (index == item_count)
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
    case TMI_VERSION:
      return VERSION;
    default:
      return nullptr;
  }
}

ITMPlugin* TMPluginGetInstance() { return &ClocksPlugin::Instance(); }
