#include "ClocksConfig.hpp"

#include "bootstrap.hpp"

ClocksConfig::ClocksConfig(Configuration configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  bootstrap(window_handle);
  return {};
}
