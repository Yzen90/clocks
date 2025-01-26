#pragma once

#include <string>

using std::string;

struct L10N {
  struct StateStore {
    struct Contexts {
      string initialization;
      string config_load;
      string config_save;
    } contexts;
    struct Messages {
      string eager_confirmed;
    } messages;
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
  struct Generic {
    string auto_option;
  } generic;
};
