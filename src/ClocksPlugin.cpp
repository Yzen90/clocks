#include "ClocksPlugin.hpp"

#include <easylogging++.cc>

#include "config.hpp"
#include "ui/ClocksConfig.hpp"

ClocksPlugin::ClocksPlugin() : state(StateStore::instance()), item_count(0) {}

ClocksPlugin& ClocksPlugin::instance() {
  static ClocksPlugin instance;
  return instance;
}

IPluginItem* ClocksPlugin::GetItem(int index) {
  if (index == item_count)
    return nullptr;
  else
    return &clocks[index];
}

void ClocksPlugin::DataRequired() { state.refresh(); }

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
    state.set_config_dir(data);
    sync();
  }
}

ITMPlugin::OptionReturn ClocksPlugin::ShowOptionsDialog(void* hParent) {
  ClocksConfig dialog{state.get_configuration()};

  auto configuration = dialog.open(hParent);

  if (configuration) {
    state.set_configuration(configuration.value());
    sync();
    return OptionReturn::OR_OPTION_CHANGED;

  } else {
    return OptionReturn::OR_OPTION_UNCHANGED;
  }
}

ITMPlugin* TMPluginGetInstance() { return &ClocksPlugin::instance(); }

void ClocksPlugin::sync() {
  auto count = state.item_count();

  if (count != item_count) {
    if (count > item_count) {
      clocks.reserve(count);
      for (Index index = item_count; index < count; index++) clocks.push_back(ClockItem{index, state});
    } else {
      clocks.erase(clocks.begin() + item_count, clocks.end());
    }

    item_count = count;
  }
}
