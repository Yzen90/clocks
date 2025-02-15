#pragma once

#include "../StateStore.hpp"
#include "imgui.hpp"

using std::optional;

class ClocksConfig {
 private:
  Configuration configuration;

  Theme current_theme;
  Resources resources;

  bool is_changed = false;
  bool debug_level = false;
  bool show_metrics = false;

  bool ui_primary_button(const string& text);
  void ui_top_buttons_spacing(int button_count);

  void ui_theme_menu();
  void ui_graphics_metrics();
  void ui_main_actions();

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
