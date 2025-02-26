#pragma once

#include "../StateStore.hpp"
#include "imgui.h"
#include "imgui.hpp"

using std::map;
using std::nullopt;
using std::optional;
using std::string_view;
using winrt::Windows::Globalization::DateTimeFormatting::DateTimeFormatter;

class ClocksConfig {
 private:
  Configuration configuration;
  Configuration original;
  string log_file;

  Resources resources;
  Logger* logger = Loggers::getLogger("ClocksConfig");

  bool was_opened = false;
  bool is_changed = false;
  bool save = false;
  bool show_metrics = false;
  bool debug_level = false;
  map<LogLevel, string_view> log_levels;
  string selected_log_level;

  string locale;
  bool locale_changed;
  float locale_space;
  string save_label;
  ImVec2 save_button_size;

  DateTimeFormatter formatter;
  string time_sample;
  string time_sample_before;
  string time_sample_after;
  string selected_clock_type;
  map<ClockType, string_view> clock_types;

  int selected_clock = 0;
  string clock_count;
  ImVec2 clock_count_size;
  float clock_count_position;
  bool moved_up = false;
  bool moved_down = false;
  bool can_remove;
  bool can_add;
  bool clock_added = false;
  bool open_timezone_select = false;
  string timezone_select_title;
  ImVec2 timezone_select_size;
  string cancel_label;
  ImVec2 cancel_button_size;

  string timezone_filter;
  vector<string_view> timezones;
  vector<string_view> timezones_filtered;

  Theme current_theme;
  struct ThemeItem {
    string_view label;
    string_view icon;
  };
  map<Theme, ThemeItem> theme_list;

  float gap;
  float frame_height;
  ImVec2 frame_padding;
  float icon_button_width;
  ImVec2 icon_button_size;
  ImVec2 icon_button_size_split;
  float button_space;
  ImVec2 button_padding;
  float panel_width;
  float breakpoint;
  bool panel_large;
  float clock_entry_adjustment;

  void ui_section_header();
  void ui_section_clocks(ImVec2& size);
  void ui_section_options(ImVec2& size);
  void ui_section_footer();

  void change_locale();
  void ui_locale_select();
  void refresh_theme_list();
  void ui_theme_menu();
  void ui_graphics_metrics();

  void refresh_clocks_state();
  void ui_clock_count();
  void ui_move_clock_buttons();
  void ui_remove_clock_button();
  void ui_add_clock_button();

  void ui_clock_entry(Clock& clock, const Index& index);
  void load_timezones();
  static int filter_timezones(ImGuiInputTextCallbackData* data);
  void ui_timezone_select();

  void refresh_sample();
  void ui_clock_sample();
  void refresh_clock_type();
  void refresh_clock_types();
  void ui_clock_type_select();
  void refresh_log_levels();
  void refresh_log_level();
  void ui_log_level_select();

  void refresh_changed();

  bool ui_primary_button(
      const string& text, optional<float> font_scale = nullopt, optional<ImVec2> size = nullopt,
      optional<ImVec2> padding = nullopt
  );
  bool ui_icon_button(
      const string& icon, optional<float> font_scale = nullopt, optional<ImVec2> size = nullopt,
      optional<ImVec2> padding = nullopt
  );

 public:
  ClocksConfig(Configuration configuration, string log_file);

  optional<Configuration> open(void*& window_handle);
};
