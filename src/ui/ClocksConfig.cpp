#include "ClocksConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imgui_toggle.h>

#include <nowide/convert.hpp>
#include <rapidfuzz/fuzz.hpp>
#include <thread>

#include "../shared.hpp"
#include "icons.hpp"
#include "imgui.hpp"

using namespace material_symbols;
using namespace std::chrono;

using nowide::narrow;
using nowide::widen;
using rapidfuzz::fuzz::CachedPartialTokenSetRatio;
using std::to_string;
using winrt::clock;

const short MIN_WIDTH = 512;
const short MIN_HEIGHT = 384;
const short GAP = 5;
const short BASE_SIZE = 20;
const float SCALE_L = 1.24;
const short RESPONSIVE_BREAKPOINT = 480;

ClocksConfig::ClocksConfig(Configuration configuration, string log_file)
    : configuration(configuration),
      original(configuration),
      log_file(log_file),
      formatter(StateStore::get_time_formatter(configuration.clock_type)) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto setup_resources =
            setup(window_handle, configuration.theme, BASE_SIZE, MIN_WIDTH, MIN_HEIGHT, configuration.locale)) {
      resources = std::move(*setup_resources);
      setup_resources.reset();

      logger->info(l10n->ui.messages.config_opened);

      was_opened = true;
      locale_changed = true;
      current_theme = configuration.theme;

      gap = ceil(GAP * resources.scale_factor);
      icon_button_width = (BASE_SIZE * resources.scale_factor) + (gap * 2);
      icon_button_size = {icon_button_width, icon_button_width};

      button_space = icon_button_width + gap;
      button_padding = {gap, gap};
      breakpoint = RESPONSIVE_BREAKPOINT * resources.scale_factor;
      timezone_select_size = {ceil(resources.real_min_width * 0.6f), ceil(resources.real_min_height * 0.5f)};

      bool first_frame = true;

      while (keep_open(resources)) {
        if (is_minimized(resources)) continue;

        if (current_theme != configuration.theme) {
          current_theme = configuration.theme;
          set_theme(current_theme, &resources);
        }

        new_frame();

        if (first_frame) {
          first_frame = false;

          refresh_sample();
          refresh_clocks_state();
          frame_height = ImGui::GetFrameHeight();
          frame_padding = ImGui::GetStyle().FramePadding;
          icon_button_size_split = {icon_button_width, floor((icon_button_width - frame_padding.y) / 2)};
          clock_entry_adjustment = frame_padding.y * 3;
        }

        if (locale_changed) change_locale();

        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 12 * resources.scale_factor);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2 * resources.scale_factor);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, gap));
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(resources.io->DisplaySize);

        ImGui::Begin(
            "ClocksConfig", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse
        );
        ImGui::PopStyleVar(2);

        panel_width = floor(available_x() / 2);
        panel_large = panel_width > breakpoint;

        ui_section_header();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{gap, 0});
        ImVec2 panels_size{panel_width, available_y() - (save_button_size.y + (gap * 2))};
        ui_section_clocks(panels_size);
        ui_section_options(panels_size);

        ui_timezone_select();

        ui_section_footer();
        if (save) break;

        ImGui::End();
        ImGui::PopStyleVar(3);

        render(resources);
      }

      cleanup(&resources);
    }
  }};

  HANDLE ui_thread = ui.native_handle();
  DWORD result;
  while (true) {
    result = MsgWaitForMultipleObjects(1, &ui_thread, FALSE, INFINITE, QS_ALLINPUT);

    if (result == WAIT_OBJECT_0) {
      break;

    } else if (result == WAIT_OBJECT_0 + 1) {
      MSG message;
      while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
      }
    } else {
      throw "UI thread wait error.";
    }
  }
  ui.join();

  if (save) return {configuration};

  load_locale(original.locale);
  if (was_opened) logger->info(l10n->ui.messages.config_closed);
  return nullopt;
}

void ClocksConfig::refresh_changed() {
  if (configuration.clock_type != original.clock_type ||
      configuration.show_day_difference != original.show_day_difference || configuration.locale != original.locale ||
      configuration.theme != original.theme || configuration.log_level != original.log_level) {
    is_changed = true;
    return;
  }

  if (configuration.clocks.size() != original.clocks.size()) {
    is_changed = true;
    return;
  }

  auto& current = configuration.clocks;
  auto& previous = original.clocks;

  for (Index i = 0; i < current.size(); i++) {
    if (current[i].label != previous[i].label || current[i].timezone != previous[i].timezone) {
      is_changed = true;
      return;
    }
  }

  is_changed = false;
}

void ClocksConfig::ui_section_footer() {
  ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0, 0));
  ImGui::Separator();
  ImGui::PopStyleVar();

  move_x(end_x() - save_button_size.x - gap);
  move_y(current_y() + gap);

  ImGui::BeginDisabled(!is_changed);
  if (ui_primary_button(save_label)) save = true;
  ImGui::EndDisabled();
}

void ClocksConfig::ui_section_clocks(ImVec2& size) {
  ImGui::BeginChild("Clocks", size, ImGuiChildFlags_AlwaysUseWindowPadding);

  Index index = 0;
  auto last = configuration.clocks.size() - 1;

  for (auto& clock : configuration.clocks) {
    ui_clock_entry(clock, index);
    if (index < last) ImGui::Separator();
    index++;
  }

  if (clock_added) {
    clock_added = false;
    ImGui::SetScrollHereY();
  }

  ImGui::EndChild();
}

void ClocksConfig::ui_section_options(ImVec2& size) {
  ImGui::SameLine();
  move_x(size.x);

  ImGui::BeginChild(
      "Options", size, ImGuiChildFlags_AlwaysUseWindowPadding,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
  );

  ui_clock_sample();

  ImGui::Dummy(button_padding);
  ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0, gap));
  ImGui::SeparatorText(l10n->ui.sections.configuration.options.data());

  if (ImGui::Toggle(
          (" " + l10n->ui.sections.configuration.show_day_difference).data(), &configuration.show_day_difference,
          ImGuiToggleFlags_Animated
      )) {
    refresh_sample();
    refresh_changed();
  }

  ImGui::Dummy(button_padding);
  ui_clock_type_select();

  ImGui::Dummy(button_padding);
  ImGui::SeparatorText(l10n->ui.sections.log.title.data());
  ImGui::PopStyleVar();

  ui_log_level_select();

  ImGui::EndChild();
}

void ClocksConfig::refresh_clocks_state() {
  auto count = configuration.clocks.size();
  clock_count = to_string(count);
  clock_count_size = ImGui::CalcTextSize(clock_count.data());
  clock_count_size.x += (gap * 2) + 1;
  clock_count_size.y += (gap * 2);

  clock_count_position = clock_count_size.x + icon_button_width * 3 + gap * 3;

  can_remove = count > 1;
  can_add = count < ITEM_MAX;
}

void ClocksConfig::ui_clock_count() {
  move_x(panel_width - clock_count_position);
  ImGui::BeginDisabled();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, button_padding);
  ImGui::BeginChild("ClockCount", clock_count_size, ImGuiChildFlags_Borders);

  ImGui::TextUnformatted(clock_count.data());

  ImGui::EndChild();
  ImGui::PopStyleVar();

  ImGui::EndDisabled();
}

void ClocksConfig::ui_move_clock_buttons() {
  move_x(panel_width - (icon_button_width * 3 + gap * 2));

  ImGui::BeginChild("MoveClock", icon_button_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

  ImGui::BeginDisabled(selected_clock == 0);
  if (ui_icon_button(ARROW_DROP_UP, nullopt, icon_button_size_split, ImVec2{0, -gap})) {
    auto& clocks = configuration.clocks;
    std::swap(clocks[selected_clock], clocks[selected_clock - 1]);
    selected_clock--;
    moved_up = true;
    refresh_changed();
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(selected_clock == (configuration.clocks.size() - 1));
  if (ui_icon_button(ARROW_DROP_DOWN, nullopt, icon_button_size_split, ImVec2{0, -gap})) {
    auto& clocks = configuration.clocks;
    std::swap(clocks[selected_clock], clocks[selected_clock + 1]);
    selected_clock++;
    moved_down = true;
    refresh_changed();
  }
  ImGui::EndDisabled();

  ImGui::EndChild();
}

void ClocksConfig::ui_remove_clock_button() {
  move_x(panel_width - (icon_button_width * 2 + gap));

  ImGui::BeginDisabled(!can_remove);
  if (ui_icon_button(AUTO_DELETE, SCALE_L)) {
    configuration.clocks.erase(configuration.clocks.begin() + selected_clock);

    if (selected_clock == configuration.clocks.size()) selected_clock--;
    refresh_clocks_state();
    refresh_changed();
  }
  ImGui::EndDisabled();
}

void ClocksConfig::ui_add_clock_button() {
  move_x(panel_width - icon_button_width);

  ImGui::BeginDisabled(!can_add);
  if (ui_icon_button(MORE_TIME, SCALE_L)) {
    configuration.clocks.push_back(DEFAULT_CLOCK);
    refresh_clocks_state();
    selected_clock = configuration.clocks.size() - 1;
    clock_added = true;
    refresh_changed();
  }
  ImGui::EndDisabled();
}

void ClocksConfig::ui_clock_entry(Clock& clock, const Index& index) {
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, button_padding);

  auto id = to_string(index);

  if (moved_up && index == selected_clock) {
    moved_up = false;
    ImGui::SetScrollHereY();
  }

  if (ImGui::BeginTable(("ClockContainer" + id).data(), 2)) {
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, frame_height);

    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::Dummy(panel_large ? button_padding : frame_padding);
    if (ImGui::RadioButton(("##Clock" + id).data(), &selected_clock, index)) refresh_sample();

    ImGui::TableNextColumn();
    ImGui::PopStyleVar();
    if (ImGui::BeginTable(("ClockParams" + id).data(), panel_large ? 2 : 1)) {
      ImGui::TableNextRow();

      ImGui::TableNextColumn();
      if (panel_large) ImGui::Dummy({1, clock_entry_adjustment});
      ImGui::TextDisabled("%s", l10n->ui.sections.clocks.timezone.data());
      ImVec2 size{available_x(), 0};
      if (ui_primary_button(clock.timezone, nullopt, size)) {
        selected_clock = index;
        open_timezone_select = true;
      }

      if (!panel_large) ImGui::TableNextRow();

      ImGui::TableNextColumn();
      if (!panel_large) ImGui::Dummy(button_padding);
      ImGui::TextDisabled("%s", l10n->ui.sections.clocks.label.data());

      ImGui::SetNextItemWidth(size.x);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, button_padding);
      ImGui::InputText("", &clock.label, ImGuiInputTextFlags_EnterReturnsTrue);
      if (ImGui::IsItemActivated()) selected_clock = index;
      if (ImGui::IsItemDeactivated()) refresh_changed();

      ImGui::PopStyleVar();
      if (!panel_large) ImGui::Dummy({1, clock_entry_adjustment});

      ImGui::EndTable();
    }

    ImGui::EndTable();
  }

  if (moved_down && index == selected_clock) {
    moved_down = false;
    ImGui::SetScrollHereY();
  }
}

void ClocksConfig::load_timezones() {
  for (const auto& tz : get_tzdb().zones) {
    auto timezone = tz.name();

    if (timezone.rfind("SystemV", 0) != 0) timezones.push_back(timezone);
  }
}

int ClocksConfig::filter_timezones(ImGuiInputTextCallbackData* event) {
  if (event->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
    auto instance = static_cast<ClocksConfig*>(event->UserData);

    auto& timezones = instance->timezones;
    auto& timezones_filtered = instance->timezones_filtered;

    string_view buffer{event->Buf, static_cast<size_t>(event->BufTextLen)};

    if (buffer.empty()) {
      timezones_filtered = timezones;
      return 0;
    }

    timezones_filtered.clear();
    auto filter = CachedPartialTokenSetRatio{buffer};

    for (auto& timezone : timezones) {
      auto tz = to_lower(timezone);
      std::replace(tz.begin(), tz.end(), '_', ' ');
      std::replace(tz.begin(), tz.end(), '/', ' ');

      if (filter.similarity(tz) == 100) timezones_filtered.push_back(timezone);
    }
  }

  return 0;
}

void ClocksConfig::ui_timezone_select() {
  if (open_timezone_select) {
    open_timezone_select = false;

    if (timezones.empty()) load_timezones();
    timezones_filtered = timezones;
    timezone_filter = "";

    ImGui::OpenPopup(timezone_select_title.data());
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, button_padding);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, {0.5, 0.5});

  if (ImGui::BeginPopupModal(timezone_select_title.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::SetNextItemWidth(timezone_select_size.x);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, button_padding);

    ImGui::InputTextWithHint(
        "##Filter", l10n->ui.sections.clocks.filter.data(), &timezone_filter, ImGuiInputTextFlags_CallbackEdit,
        ClocksConfig::filter_timezones, this
    );
    ImGui::PopStyleVar();

    ImGui::GetStyle().GrabMinSize = 40 * resources.scale_factor;
    ImGui::BeginChild("Clocks", timezone_select_size, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders);

    for (auto& timezone : timezones_filtered) {
      if (ImGui::MenuItem(timezone.data())) {
        ImGui::CloseCurrentPopup();

        auto& selected = configuration.clocks[selected_clock];
        selected.timezone = timezone;

        auto separator = timezone.find_last_of('/');
        selected.label = separator != string::npos ? timezone.substr(separator + 1) : timezone;

        std::replace(selected.label.begin(), selected.label.end(), '_', ' ');
        selected.label += ":";

        refresh_sample();
        refresh_changed();
      }
    }

    ImGui::EndChild();

    ImGui::Dummy(frame_padding);

    move_x(end_x() - cancel_button_size.x);
    if (ui_primary_button(l10n->ui.actions.cancel, nullopt, cancel_button_size)) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();
}

void ClocksConfig::refresh_sample() {
  auto time = system_clock::now();
  auto winrt_time = clock::from_sys(time);

  auto selected_tz = configuration.clocks[selected_clock].timezone;
  auto local_days = sys_days{year_month_day{floor<days>(zoned_time{selected_tz, time}.get_local_time())}};
  auto timezone = widen(selected_tz);
  auto tz = locate_zone(selected_tz);

  time_sample = narrow(StateStore::get_time(tz, timezone, configuration, formatter, time, winrt_time, local_days));
  if (configuration.show_day_difference) {
    time_sample_after =
        narrow(StateStore::get_time(tz, timezone, configuration, formatter, time, winrt_time, local_days - days(1)));
    time_sample_before =
        narrow(StateStore::get_time(tz, timezone, configuration, formatter, time, winrt_time, local_days + days(1)));
  }
}

void ClocksConfig::ui_clock_sample() {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, button_padding);
  ImGui::BeginChild("Sample", {available_x(), 0}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

  ImGui::TextDisabled("%s", l10n->ui.sections.configuration.sample.data());

  auto selected_label = configuration.clocks[selected_clock].label;

  string sample = selected_label + " " + time_sample;
  if (configuration.show_day_difference) {
    auto separator = panel_large ? '\t' : '\n';

    sample += separator + selected_label + " " + time_sample_before;
    sample += separator + selected_label + " " + time_sample_after;
  }

  ImGui::TextWrapped("%s", sample.data());

  ImGui::EndChild();
  ImGui::PopStyleVar();
}

void ClocksConfig::refresh_clock_type() {
  selected_clock_type =
      SEARCH_ACTIVITY + " " +
      (configuration.clock_type == ClockType::FormatAuto
           ? l10n->ui.sections.configuration.clock_type_auto
           : (configuration.clock_type == ClockType::Format24h ? l10n->ui.sections.configuration.clock_type_24h
                                                               : l10n->ui.sections.configuration.clock_type_12h));
}

void ClocksConfig::refresh_clock_types() {
  clock_types = {
      {ClockType::FormatAuto, l10n->ui.sections.configuration.clock_type_auto},
      {ClockType::Format12h, l10n->ui.sections.configuration.clock_type_12h},
      {ClockType::Format24h, l10n->ui.sections.configuration.clock_type_24h},
  };
}

void ClocksConfig::ui_clock_type_select() {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, button_padding);
  ImGui::SetNextItemWidth(panel_large ? breakpoint / 2 : available_x());
  if (ImGui::BeginCombo("##ClockType", selected_clock_type.data(), ImGuiComboFlags_None)) {
    bool selected;

    for (const auto& [type, label] : clock_types) {
      selected = configuration.clock_type == type;

      if (ImGui::Selectable(label.data(), selected)) {
        configuration.clock_type = type;
        refresh_clock_type();
        formatter = StateStore::get_time_formatter(configuration.clock_type);
        refresh_sample();
        refresh_changed();
      }
      if (selected) ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  ImGui::PopStyleVar();
}

void ClocksConfig::refresh_log_levels() {
  log_levels = {{LogLevel::FATAL, {l10n->ui.sections.log.level.fatal}},
                {LogLevel::ERROR, {l10n->ui.sections.log.level.error}},
                {LogLevel::WARNING, {l10n->ui.sections.log.level.warning}},
                {LogLevel::INFO, {l10n->ui.sections.log.level.info}},
                {LogLevel::VERBOSE, {l10n->ui.sections.log.level.verbose}},
                {LogLevel::DEBUG, {l10n->ui.sections.log.level.debug}}};
}

void ClocksConfig::refresh_log_level() {
  selected_log_level = DVR + " " + l10n->ui.sections.log.level.title + ": ";
  selected_log_level.append(log_levels[configuration.log_level]);

  debug_level = configuration.log_level == LogLevel::DEBUG;
}

void ClocksConfig::ui_log_level_select() {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, button_padding);
  ImGui::SetNextItemWidth(panel_large ? breakpoint / 2 : (available_x() - (icon_button_width + gap)));
  if (ImGui::BeginCombo("##LogLevel", selected_log_level.data())) {
    bool selected = false;

    for (auto& [level, label] : log_levels) {
      selected = configuration.log_level == level;
      if (ImGui::Selectable(label.data(), selected)) {
        configuration.log_level = level;
        refresh_log_level();
        refresh_changed();
      }
      if (selected) ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  ImGui::PopStyleVar();

  ImGui::SameLine();
  if (!panel_large) move_x(end_x() - icon_button_width);
  if (ui_icon_button(OPEN_IN_NEW))
    ImGui::GetPlatformIO().Platform_OpenInShellFn(ImGui::GetCurrentContext(), log_file.data());
}

void ClocksConfig::change_locale() {
  load_locale(configuration.locale);

  locale = LANGUAGE + " ";
  locale += locales[configuration.locale == Locale::Auto ? loaded_locale() : configuration.locale];
  locale += " ";
  locale_space = round(ImGui::CalcTextSize(locale.data()).x + (BASE_SIZE * resources.scale_factor) + (gap * 5));
  locale_changed = false;

  save_label = SAVE_AS + " " + l10n->ui.actions.save + " ";

  save_button_size = ImGui::CalcTextSize(save_label.data());
  save_button_size.x += gap * 2;
  save_button_size.y += gap * 2;

  timezone_select_title = l10n->ui.sections.clocks.timezone;
  cancel_label = " " + l10n->ui.actions.cancel + " ";
  cancel_button_size = ImGui::CalcTextSize(cancel_label.data());
  cancel_button_size.x += gap * 2;
  cancel_button_size.y += gap * 2;

  refresh_theme_list();
  refresh_clock_type();
  refresh_clock_types();
  refresh_log_levels();
  refresh_log_level();
}

void ClocksConfig::ui_section_header() {
  move_x(gap);
  with_font_scale(1.3, []() { ImGui::TextUnformatted(NEST_CLOCK_FARSIGHT_ANALOG.data()); });
  ImGui::SameLine();
  with_font_scale(1.3, []() { ImGui::TextUnformatted(l10n->ui.title.data()); });
  ImGui::SameLine();

  ui_clock_count();
  ImGui::SameLine();
  ui_move_clock_buttons();
  ImGui::SameLine();
  ui_remove_clock_button();
  ImGui::SameLine();
  ui_add_clock_button();
  ImGui::SameLine();

  short buttons = debug_level ? 2 : 1;
  float start_x = end_x() - locale_space - (button_space * buttons);

  move_x(start_x);
  ui_locale_select();

  move_x(start_x + locale_space);
  if (debug_level) {
    ui_graphics_metrics();
    move_x(start_x + locale_space + button_space);
  }
  ui_theme_menu();
}

void ClocksConfig::ui_locale_select() {
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, button_padding);
  if (ImGui::BeginCombo("##Locale", locale.data(), ImGuiComboFlags_WidthFitPreview)) {
    bool selected = configuration.locale == Locale::Auto;
    if (ImGui::Selectable(l10n->generic.auto_option.data(), selected)) {
      configuration.locale = Locale::Auto;
      locale_changed = true;
      refresh_changed();
    }
    if (selected) ImGui::SetItemDefaultFocus();

    for (const auto& [locale, name] : locales) {
      selected = configuration.locale == locale;
      if (ImGui::Selectable(name.data(), selected)) {
        configuration.locale = locale;
        locale_changed = true;
        refresh_changed();
      }
      if (selected) ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  ImGui::PopStyleVar();

  ImGui::SameLine();
}

void ClocksConfig::refresh_theme_list() {
  theme_list = {
      {Theme::Auto, {{l10n->generic.auto_option}, {ROUTINE}}},
      {Theme::Light, {{l10n->ui.theme.light}, {BRIGHTNESS_HIGH}}},
      {Theme::Dark, {{l10n->ui.theme.dark}, {DARK_MODE}}},
  };
}

void ClocksConfig::ui_theme_menu() {
  bool is_light = resources.light_theme;
  const string& icon = configuration.theme == Theme::Auto ? (is_light ? BRIGHTNESS_AUTO : NIGHT_SIGHT_AUTO)
                                                          : (is_light ? BRIGHTNESS_HIGH : DARK_MODE);
  if (ui_icon_button(icon)) ImGui::OpenPopup("ThemeMenu");

  if (ImGui::BeginPopup("ThemeMenu")) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 12));
    ImGui::TextDisabled("%s", l10n->ui.theme.title.data());

    for (const auto& [theme, item] : theme_list) {
      if (ImGui::MenuItemEx(item.label.data(), item.icon.data(), nullptr, configuration.theme == theme)) {
        configuration.theme = theme;
        set_theme(theme, &resources);
        refresh_changed();
      }
    }

    ImGui::Spacing();
    ImGui::PopStyleVar();
    ImGui::EndPopup();
  }
}

void ClocksConfig::ui_graphics_metrics() {
  if (ui_icon_button(BROWSE_ACTIVITY)) show_metrics = !show_metrics;

  if (show_metrics) {
    ImGui::Begin(
        (BROWSE_ACTIVITY + "🕜").data(), &show_metrics,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus
    );

    ImGui::Text("%s", resources.driver.data());
    ImGui::Text("%.1f fps", resources.io->Framerate);
    ImGui::Text("%.0fx%.0f", resources.io->DisplaySize.x, resources.io->DisplaySize.y);
    ImGui::Text("%ddpi %d%%", resources.dpi, resources.scale);

    ImGui::End();

#ifdef NOT_RELEASE_MODE
    ImGui::ShowMetricsWindow();
#endif
  }

  ImGui::SameLine();
}

bool ClocksConfig::ui_primary_button(
    const string& text, optional<float> font_scale, optional<ImVec2> size, optional<ImVec2> padding
) {
  bool click;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding ? *padding : button_padding);

  auto show_button = [&]() { click = size ? ImGui::Button(text.data(), *size) : ImGui::Button(text.data()); };

  if (font_scale)
    with_font_scale(*font_scale, show_button);
  else
    show_button();

  ImGui::PopStyleVar();
  return click;
};

bool ClocksConfig::ui_icon_button(
    const string& icon, optional<float> font_scale, optional<ImVec2> size, optional<ImVec2> padding
) {
  return ui_primary_button(icon, font_scale, size ? size : icon_button_size, padding ? padding : ImVec2(0, 0));
};
