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
};
