#pragma once

#include "../StateStore.hpp"

using std::optional;

class ClocksConfig {
 private:
  Configuration configuration;
  Theme current_theme;
  bool configuration_changed = false;

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
