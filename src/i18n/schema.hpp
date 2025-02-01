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
      string eager_config_load;
      string no_eager_config_load;
      string eager_load_confirmed;
      string warn_default_config;
      string warn_no_clocks;
      string using_default_config;
      string config_saved;
      string too_many_clocks;
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
    string cause;
  } generic;
};
