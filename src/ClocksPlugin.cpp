#include "ClocksPlugin.hpp"

#include "config.hpp"
#include "ui/ClocksConfig.hpp"

ClocksPlugin::ClocksPlugin() : state(StateStore::instance()) {}

ClocksPlugin& ClocksPlugin::instance() {
  static ClocksPlugin instance;
  return instance;
}

IPluginItem* ClocksPlugin::GetItem(int index) {
  if (index < clocks.size())
    return &clocks[index];
  else
    return nullptr;
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

    if (state.first_time) {
      ClocksConfig window{state.get_configuration(), state.get_log_file()};

      auto no_parent = (void*)nullptr;

      auto configuration = window.open(no_parent);
      if (configuration) state.set_configuration(*configuration);
    }

    for (Index i; i < state.clock_count(); i++) clocks.push_back(ClockItem{i, state});
  }
}

ITMPlugin::OptionReturn ClocksPlugin::ShowOptionsDialog(void* parent) {
  ClocksConfig window{state.get_configuration(), state.get_log_file()};

  auto configuration = window.open(parent);

  if (configuration) {
    state.set_configuration(*configuration);
    return OptionReturn::OR_OPTION_CHANGED;

  } else {
    return OptionReturn::OR_OPTION_UNCHANGED;
  }
}

ITMPlugin* TMPluginGetInstance() { return &ClocksPlugin::instance(); }
