#include "../StateStore.hpp"

using std::optional;

class ClocksConfig {
 private:
 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open();
};
