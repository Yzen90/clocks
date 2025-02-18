#pragma once

#include <optional>

#include "../StateStore.hpp"
#include "imgui.h"
#include "imgui.hpp"

using std::optional;

class ClocksConfig {
 private:
  Configuration configuration;
  Configuration original;

  Resources resources;

  string locale;
  bool locale_changed;
  float locale_space;

  bool is_changed = false;
  bool show_metrics = false;

  Theme current_theme;
  bool debug_level = false;

  float gap;
  ImVec2 icon_button_size;
  float button_space;
  ImVec2 button_padding;

  void ui_section_bottom();
  void ui_section_main();
  void ui_section_top();

  void ui_locale_select();
  void ui_theme_menu();
  void ui_graphics_metrics();

  bool ui_primary_button(
      const string& text, optional<float> font_scale = {}, optional<ImVec2> size = {}, optional<ImVec2> padding = {}
  );
  bool ui_icon_button(const string& icon, optional<float> font_scale = {});

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
