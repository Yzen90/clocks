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

bool ClocksPlugin::open_configuration(void*& window_handle) {
  ClocksConfig window{state.get_configuration(), state.get_log_file()};

  auto configuration = window.open(window_handle);
  if (configuration) {
    state.set_configuration(*configuration);
    sync();
    return true;
  } else {
    return false;
  }
}

void ClocksPlugin::sync() {
  auto count = state.clock_count();
  auto current = clocks.size();

  if (count != current) {
    if (count > current)
      for (Index index = current; index < count; index++) clocks.push_back(ClockItem{index, state});
    /*
    else clocks.erase(clocks.begin() + count, clocks.end());

    TrafficMonitor does not call GetItem after ShowOptionsDialog
    so cannot remove items as that should invalidate the pointers
    to the removed items. If this changes in the future, deque<ClockItem> clocks
    can be changed back to vector<ClockItem> clocks.
    */
  }
}

void ClocksPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) {
  if (index == ITMPlugin::EI_CONFIG_DIR) {
    state.set_config_dir(data);
    sync();

    if (state.first_time) {
      auto parent = static_cast<void*>(GetActiveWindow());
      open_configuration(parent);
    }
  }
}

ITMPlugin::OptionReturn ClocksPlugin::ShowOptionsDialog(void* parent) {
  if (open_configuration(parent))
    return OptionReturn::OR_OPTION_CHANGED;
  else
    return OptionReturn::OR_OPTION_UNCHANGED;
}

ITMPlugin* TMPluginGetInstance() { return &ClocksPlugin::instance(); }
