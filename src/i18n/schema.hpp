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
      string invalid_timezone;
      string too_many_clocks;
      string using_default_config;
      string warn_default_config;
      string warn_no_clocks;
      string warn_no_valid_clocks;
    } messages;

    string logger_id;
  } state_store;

  struct UI {
    struct Actions {
      string save;
    } actions;

    struct Errors {
      string sdl_claim_window;
      string sdl_create_gpu_device;
      string sdl_create_window;
      string sdl_init;
    } errors;

    struct Messages {
      string setup_complete;
    } messages;

    struct Sections {
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

    string title;
  } ui;

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
