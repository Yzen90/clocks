#pragma once

#include <optional>

#include "../StateStore.hpp"
#include "imgui.h"
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
  ImVec2 icon_button_size;

  bool ui_primary_button(
      const string& text, optional<float> font_scale = {}, optional<ImVec2> size = {}, optional<ImVec2> padding = {}
  );
  bool ui_icon_button(const string& icon, optional<float> font_scale = {});

  void ui_main_actions();

  void ui_top_buttons();
  void ui_theme_menu();
  void ui_graphics_metrics();

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
