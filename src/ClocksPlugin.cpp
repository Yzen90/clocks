#include "ClocksPlugin.hpp"

#include "config.hpp"

unique_ptr<StateStore> ClocksPlugin::state = make_unique<StateStore>();
vector<ClockItem> ClocksPlugin::clocks;
ItemCount ClocksPlugin::item_count = 0;

ClocksPlugin::ClocksPlugin() {}

void ClocksPlugin::sync() {
  auto current_count = state->item_count();

  if (current_count != item_count) {
    if (current_count > item_count) {
      clocks.reserve(current_count);
      for (Index index = item_count; index < current_count; index++) clocks.push_back(ClockItem{index, *state});
    } else {
      clocks.erase(clocks.begin() + item_count, clocks.end());
    }

    item_count = current_count;
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

void ClocksPlugin::DataRequired() { state->refresh(); }

const wchar_t* ClocksPlugin::GetInfo(PluginInfoIndex index) {
  switch (index) {
    case TMI_NAME:
      return CLOCKS_NAME;
    case TMI_DESCRIPTION:
      return CLOCKS_DESCRIPTION;
    case TMI_AUTHOR:
      return CLOCKS_AUTHOR;
    case TMI_COPYRIGHT:
      return CLOCKS_C_SYMBOL CLOCKS_COPYRIGHT;
    case TMI_URL:
      return CLOCKS_URL;
    case TMI_VERSION:
      return CLOCKS_VERSION;
    default:
      return nullptr;
  }
}

void ClocksPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) {
  if (index == ITMPlugin::EI_CONFIG_DIR) {
    state->initialize(data);
    sync();
  }
}

ITMPlugin* TMPluginGetInstance() { return &ClocksPlugin::Instance(); }
