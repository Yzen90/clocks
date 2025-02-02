#pragma once

#include <string>

using std::string;

struct L10N {
  struct StateStore {
    struct Contexts {
      string configuration;
      string initialization;
    } contexts;

    struct Messages {
      string config_file;
      string config_loaded;
      string config_saved;
      string eager_config_load;
      string eager_load_confirmed;
      string invalid_timezone;
      string no_eager_config_load;
      string too_many_clocks;
      string using_default_config;
      string warn_default_config;
      string warn_no_clocks;
      string warn_no_valid_clocks;
    } messages;

    string logger_id;
  } state_store;

  struct Localization {
    string logger_id;

    struct Contexts {
      string load;
    } contexts;

    struct Messages {
      string loaded;
    } messages;
  } l10n;

  struct Generic {
    string auto_option;
    string cause;
  } generic;
};
