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

  float gap;

  bool ui_primary_button(const string& text, float font_scale = 1, optional<ImVec2> size = {});

  void ui_main_actions();

  void ui_top_buttons();
  void ui_theme_menu();
  void ui_graphics_metrics();

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
