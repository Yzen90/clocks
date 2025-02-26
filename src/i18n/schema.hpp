#pragma once

#include <string>

using std::string;

struct L10N {
  struct StateStore {
    struct Contexts {
      string configuration;
      string initialization;
    } contexts;

    string dummy_clock;

    struct Messages {
      string config_file;
      string config_loaded;
      string config_saved;
      string invalid_timezone;
      string too_many_clocks;
      string using_default_config;
      string warn_default_config;
      string warn_no_clocks;
      string warn_no_valid_clocks;
    } messages;
  } state_store;

  struct UI {
    struct Actions {
      string cancel;
      string save;
    } actions;

    struct Errors {
      string sdl_claim_window;
      string sdl_create_gpu_device;
      string sdl_create_window;
      string sdl_get_handle;
      string sdl_init;
      string sdl_set_parent;
    } errors;

    struct Messages {
      string config_closed;
      string config_opened;
      string setup_complete;
    } messages;

    struct Sections {
      struct Clocks {
        string filter;
        string label;
        string timezone;
      } clocks;

      struct Configuration {
        string clock_type_12h;
        string clock_type_24h;
        string clock_type_auto;
        string options;
        string sample;
        string show_day_difference;
      } configuration;

      struct Log {
        string title;
        struct Level {
          string debug;
          string error;
          string fatal;
          string info;
          string title;
          string trace;
          string verbose;
          string warning;
        } level;
      } log;
    } sections;

    struct Theme {
      string dark;
      string light;
      string title;
    } theme;

    string app_title;
    string title;
  } ui;

  struct Localization {
    struct Messages {
      string loaded;
      string loaded_system;
    } messages;
  } l10n;

  struct Generic {
    string auto_option;
    string cause;
  } generic;
};
