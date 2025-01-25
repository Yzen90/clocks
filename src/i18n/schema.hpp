#pragma once

#include <string>

using std::string;

struct L10N {
  struct StateStore {
    struct Contexts {
      string initialization;
    } contexts;
    string logger_id;
  } state_store;
  struct I18N {
    string logger_id;
    struct Contexts {
      string load;
    } contexts;
    struct Messages {
      string loaded;
    } messages;
  } i18n;
};
