#include "ClocksConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_toggle.h>

#include <nowide/convert.hpp>
#include <thread>

#include "icons.hpp"
#include "imgui.hpp"

using namespace material_symbols;
using namespace std::chrono;

using nowide::narrow;
using nowide::widen;
using std::round;
using winrt::clock;

const short GAP = 5;
const short ICON_BUTTON_SIZE = 30;
const short BASE_SIZE = 20;

ClocksConfig::ClocksConfig(Configuration configuration)
    : configuration(configuration),
      original(configuration),
      formatter(StateStore::get_time_formatter(configuration.clock_type)) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto setup_resources = setup(window_handle, configuration.theme, BASE_SIZE)) {
      resources = std::move(*setup_resources);
      setup_resources.reset();

      locale_changed = true;

      current_theme = configuration.theme;
      debug_level = configuration.log_level == LogLevel::DEBUG;

      refresh_sample();

      gap = round(GAP * resources.scale_factor);
      icon_button_size = {ICON_BUTTON_SIZE * resources.scale_factor, ICON_BUTTON_SIZE * resources.scale_factor};
      button_space = icon_button_size.x + gap;
      button_padding = {gap, gap};

      while (keep_open(resources)) {
        if (is_minimized(resources)) continue;

        if (current_theme != configuration.theme) {
          current_theme = configuration.theme;
          set_theme(current_theme, &resources);
        }

        new_frame();

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
        ui_section_header();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{gap, 0});
        ImVec2 panels_size{panel_width, available_y() - (save_button_size.y + (gap * 2))};
        ui_section_clocks(panels_size);
        ui_section_options(panels_size);

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
  return {};
}

void ClocksConfig::ui_section_footer() {
  ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0, 0));
  ImGui::Separator();
  ImGui::PopStyleVar();

  move_x(end_x() - save_button_size.x - gap);
  move_y(current_y() + gap);
  ui_primary_button(save_label);
}

void ClocksConfig::ui_section_clocks(ImVec2& size) {
  ImGui::BeginChild("Clocks", size, ImGuiChildFlags_AlwaysUseWindowPadding);

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

void ClocksConfig::ui_add_clock_button() {
  move_x(panel_width - icon_button_size.x);
  ui_icon_button(MORE_TIME, 1.2);
}

void ClocksConfig::refresh_sample() {
  auto time = system_clock::now();
  auto winrt_time = clock::from_sys(time);

  auto selected_tz = configuration.clocks[selected_clock].timezone;
  auto local_days = sys_days{year_month_day{floor<days>(zoned_time{selected_tz, time}.get_local_time())}};
  auto timezone = widen(selected_tz);
  auto tz = locate_zone(DEFAULT_CLOCK.timezone);

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
    auto separator = panel_width > (480 * resources.scale_factor) ? '\t' : '\n';

    sample += separator + selected_label + " " + time_sample_before;
    sample += separator + selected_label + " " + time_sample_after;
  }

  ImGui::TextWrapped("%s", sample.data());

  ImGui::EndChild();
  ImGui::PopStyleVar();
}

void ClocksConfig::ui_section_header() {
  if (locale_changed) {
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
  }

  move_x(gap);
  with_font_scale(1.3, []() { ImGui::TextUnformatted(NEST_CLOCK_FARSIGHT_ANALOG.data()); });
  ImGui::SameLine();
  with_font_scale(1.3, []() { ImGui::TextUnformatted(l10n->ui.title.data()); });
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
