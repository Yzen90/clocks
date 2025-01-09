#include "ClocksPlugin.hpp"

// #include <chrono>

ClocksPlugin ClocksPlugin::m_instance;
ClocksPlugin::ClocksPlugin() {}
ClocksPlugin& ClocksPlugin::Instance() { return m_instance; }

const wchar_t* ClocksPlugin::GetInfo(PluginInfoIndex index) {
  switch (index) {
    case TMI_NAME:
      return (wchar_t*)NAME;
    case TMI_DESCRIPTION:
      return (wchar_t*)DESCRIPTION;
    case TMI_AUTHOR:
      return (wchar_t*)AUTHOR;
    case TMI_COPYRIGHT:
      return (wchar_t*)COPYRIGHT;
    case TMI_URL:
      return (wchar_t*)URL;
      break;
    case TMI_VERSION:
      return (wchar_t*)VERSION;
    default:
      break;
  }

  return nullptr;
}
