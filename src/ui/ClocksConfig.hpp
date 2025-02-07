#pragma once

#include "../StateStore.hpp"

using std::optional;

class ClocksConfig {
 private:
  Configuration configuration;

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
