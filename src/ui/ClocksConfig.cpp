#include "ClocksConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imgui_toggle.h>

#include <nowide/convert.hpp>
#include <thread>

#include "icons.hpp"
#include "imgui.hpp"

using namespace material_symbols;
using namespace std::chrono;

using nowide::narrow;
using nowide::widen;
using std::to_string;
using winrt::clock;

const short MIN_WIDTH = 512;
const short MIN_HEIGHT = 384;
const short GAP = 5;
const short BASE_SIZE = 20;
const float SCALE_L = 1.24;
const short RESPONSIVE_BREAKPOINT = 480;

ClocksConfig::ClocksConfig(Configuration configuration)
    : configuration(configuration),
      original(configuration),
      formatter(StateStore::get_time_formatter(configuration.clock_type)) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto setup_resources = setup(window_handle, configuration.theme, BASE_SIZE, MIN_WIDTH, MIN_HEIGHT)) {
      resources = std::move(*setup_resources);
      setup_resources.reset();

      locale_changed = true;

      current_theme = configuration.theme;
      debug_level = configuration.log_level == LogLevel::DEBUG;

      refresh_sample();

      gap = ceil(GAP * resources.scale_factor);
      icon_button_width = (BASE_SIZE * resources.scale_factor) + (gap * 2);
      icon_button_size = {icon_button_width, icon_button_width};
      button_space = icon_button_width + gap;
      button_padding = {gap, gap};
      breakpoint = RESPONSIVE_BREAKPOINT * resources.scale_factor;
      timezone_select_size = {ceil(resources.real_min_width * 0.6f), ceil(resources.real_min_height * 0.4f)};

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
          refresh_clocks_state();
          frame_height = ImGui::GetFrameHeight();
          frame_padding = ImGui::GetStyle().FramePadding;
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

        panel_width = available_x() / 2;
        panel_large = panel_width > breakpoint;
        ui_section_header();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{gap, 0});
        ImVec2 panels_size{panel_width, available_y() - (save_button_size.y + (gap * 2))};
        ui_section_clocks(panels_size);
        ui_section_options(panels_size);

        ui_timezone_select();

        ui_section_footer();

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

  if (is_changed) return {configuration};

  load_locale(original.locale);
  return nullopt;
}

void ClocksConfig::ui_section_footer() {
  ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0, 0));
  ImGui::Separator();
  ImGui::PopStyleVar();

  move_x(end_x() - save_button_size.x - gap);
  move_y(current_y() + gap);

  ImGui::BeginDisabled(!is_changed);
  ui_primary_button(save_label);
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

  ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0, gap));
  ImGui::SeparatorText(l10n->ui.sections.configuration.options.data());
  ImGui::PopStyleVar();

  if (ImGui::Toggle(
          (" " + l10n->ui.sections.configuration.show_day_difference).data(), &configuration.show_day_difference,
          ImGuiToggleFlags_Animated
      )) {
    refresh_sample();
  }

  ImGui::EndChild();
}

void ClocksConfig::refresh_clocks_state() {
  auto count = configuration.clocks.size();
  clock_count = to_string(count);
  clock_count_size = ImGui::CalcTextSize(clock_count.data());
  clock_count_size.x += (gap * 2) + 1;
  clock_count_size.y += (gap * 2) + 1;

  can_remove = count > 1;
  can_add = count < ITEM_MAX;
}

void ClocksConfig::ui_clock_count() {
  move_x(panel_width - (clock_count_size.x + icon_button_width * 2 + gap * 2));
  ImGui::BeginDisabled();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, button_padding);
  ImGui::BeginChild("ClockCount", clock_count_size, ImGuiChildFlags_Borders);

  ImGui::TextUnformatted(clock_count.data());

  ImGui::EndChild();
  ImGui::PopStyleVar();

  ImGui::EndDisabled();
}

void ClocksConfig::ui_remove_clock_button() {
  move_x(panel_width - (icon_button_width * 2 + gap));

  ImGui::BeginDisabled(!can_remove);
  if (ui_icon_button(AUTO_DELETE, SCALE_L)) {
    configuration.clocks.erase(configuration.clocks.begin() + selected_clock);

    if (selected_clock == configuration.clocks.size()) selected_clock--;
    refresh_clocks_state();
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
  }
  ImGui::EndDisabled();
}

void ClocksConfig::ui_clock_entry(Clock& clock, const Index& index) {
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, button_padding);

  auto id = to_string(index);

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
      if (ui_primary_button(clock.timezone, nullopt, size)) open_timezone_select = true;

      if (!panel_large) ImGui::TableNextRow();

      ImGui::TableNextColumn();
      if (!panel_large) ImGui::Dummy(button_padding);
      ImGui::TextDisabled("%s", l10n->ui.sections.clocks.label.data());
      ImGui::SetNextItemWidth(size.x);
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, button_padding);
      ImGui::InputText("", &clock.label);
      ImGui::PopStyleVar();
      if (!panel_large) ImGui::Dummy({1, clock_entry_adjustment});

      ImGui::EndTable();
    }

    ImGui::EndTable();
  }
}

void ClocksConfig::load_timezones() {
  for (const auto& timezone : get_tzdb().zones) {
    timezones.push_back(timezone.name());
  }
}

void ClocksConfig::ui_timezone_select() {
  if (open_timezone_select) {
    open_timezone_select = false;

    if (timezones.empty()) load_timezones();

    ImGui::OpenPopup(timezone_select_title.data());
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, button_padding);
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, {0.5, 0.5});

  if (ImGui::BeginPopupModal(timezone_select_title.data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::GetStyle().GrabMinSize = 40 * resources.scale_factor;
    ImGui::BeginChild("Clocks", timezone_select_size, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders);

    for (auto& timezone : timezones) {
      ImGui::MenuItem(timezone.data());
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
}

void ClocksConfig::ui_section_header() {
  move_x(gap);
  with_font_scale(1.3, []() { ImGui::TextUnformatted(NEST_CLOCK_FARSIGHT_ANALOG.data()); });
  ImGui::SameLine();
  with_font_scale(1.3, []() { ImGui::TextUnformatted(l10n->ui.title.data()); });
  ImGui::SameLine();

  ui_clock_count();
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
    }
    if (selected) ImGui::SetItemDefaultFocus();

    for (const auto& [locale, name] : locales) {
      selected = configuration.locale == locale;
      if (ImGui::Selectable(name.data(), selected)) {
        configuration.locale = locale;
        locale_changed = true;
      }
      if (selected) ImGui::SetItemDefaultFocus();
    }

    ImGui::EndCombo();
  }
  ImGui::PopStyleVar();

  ImGui::SameLine();
}

void ClocksConfig::ui_theme_menu() {
  bool is_light = resources.light_theme;
  const string& icon = configuration.theme == Theme::Auto ? (is_light ? BRIGHTNESS_AUTO : NIGHT_SIGHT_AUTO)
                                                          : (is_light ? BRIGHTNESS_HIGH : DARK_MODE);
  if (ui_icon_button(icon)) ImGui::OpenPopup("theme_menu");

  if (ImGui::BeginPopup("theme_menu")) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 12));
    ImGui::TextDisabled("%s", l10n->ui.theme.title.data());

    if (ImGui::MenuItemEx(l10n->generic.auto_option.data(), ROUTINE.data())) {
      configuration.theme = Theme::Auto;
      set_theme(Theme::Auto, &resources);
    }

    if (ImGui::MenuItemEx(l10n->ui.theme.light.data(), BRIGHTNESS_HIGH.data())) {
      configuration.theme = Theme::Light;
      set_theme(Theme::Light, &resources);
    }

    if (ImGui::MenuItemEx(l10n->ui.theme.dark.data(), DARK_MODE.data())) {
      configuration.theme = Theme::Dark;
      set_theme(Theme::Dark, &resources);
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

bool ClocksConfig::ui_icon_button(const string& icon, optional<float> font_scale) {
  return ui_primary_button(icon, font_scale, icon_button_size, ImVec2(0, 0));
};
