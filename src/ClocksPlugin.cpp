#include "ClocksPlugin.hpp"

// #include <chrono>

ClocksPlugin ClocksPlugin::m_instance;
ClocksPlugin::ClocksPlugin() {}
ClocksPlugin& ClocksPlugin::Instance() { return m_instance; }

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
