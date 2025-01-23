#include <string>

using std::string;

struct I18N {
  struct StateStore {
    struct Contexts {
      string initialization;
    } contexts;
  } state_store;
};
