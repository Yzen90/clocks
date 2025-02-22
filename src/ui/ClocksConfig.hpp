#pragma once

#include <optional>

#include "../StateStore.hpp"
#include "imgui.h"
#include "imgui.hpp"

using std::optional;
using winrt::Windows::Globalization::DateTimeFormatting::DateTimeFormatter;

class ClocksConfig {
 private:
  Configuration configuration;
  Configuration original;

  Resources resources;
  Logger* logger = Loggers::getLogger("ClocksConfig");

  bool is_changed = false;
  bool show_metrics = false;

  string locale;
  bool locale_changed;
  float locale_space;
  string save_label;
  ImVec2 save_button_size;

  DateTimeFormatter formatter;
  string time_sample;
  string time_sample_before;
  string time_sample_after;

  Index selected_clock = 0;
  Theme current_theme;
  bool debug_level = false;

  float gap;
  float icon_button_width;
  ImVec2 icon_button_size;
  float button_space;
  ImVec2 button_padding;
  float panel_width;
  float breakpoint;
  bool panel_large;

  void ui_section_header();
  void ui_section_clocks(ImVec2& size);
  void ui_section_options(ImVec2& size);
  void ui_section_footer();

  void ui_locale_select();
  void ui_theme_menu();
  void ui_graphics_metrics();

  void ui_add_clock_button();
  void ui_clock_entry(Clock& clock);
  void ui_clock_sample();

  bool ui_primary_button(
      const string& text, optional<float> font_scale = {}, optional<ImVec2> size = {}, optional<ImVec2> padding = {}
  );
  bool ui_icon_button(const string& icon, optional<float> font_scale = {});

  void refresh_sample();

 public:
  ClocksConfig(Configuration configuration);

  optional<Configuration> open(void*& window_handle);
};
