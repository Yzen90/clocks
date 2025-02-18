#include "ClocksConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <thread>

#include "icons.hpp"
#include "imgui.hpp"

using namespace material_symbols;
using std::round;

const short GAP = 5;
const short ICON_BUTTON_SIZE = 30;
const short BASE_SIZE = 20;

ClocksConfig::ClocksConfig(Configuration configuration) : configuration(configuration), original(configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto setup_resources = setup(window_handle, configuration.theme, BASE_SIZE)) {
      resources = std::move(*setup_resources);
      setup_resources.reset();

      locale_changed = true;

      current_theme = configuration.theme;
      debug_level = configuration.log_level == LogLevel::DEBUG;

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

        ui_section_top();
        ui_section_main();
        ui_section_bottom();

        ImGui::End();
        ImGui::PopStyleVar(2);

        render(resources);
      }

      cleanup(&resources);
    }
  }};

  ui.join();

  if (is_changed) return {configuration};

  load_locale(original.locale);
  return {};
}

void ClocksConfig::ui_section_bottom() {
  ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2(0, 0));
  ImGui::Separator();
  ImGui::PopStyleVar();

  ImGui::Dummy(ImVec2(1, gap));

  // TODO - Position according to text size
  move_x(available_x() - (99 * resources.scale_factor) - gap);
  ui_primary_button(SAVE_AS + " " + l10n->ui.actions.save + " ");
}

void ClocksConfig::ui_section_main() {
  float container_width = ImGui::GetContentRegionAvail().x * 0.5;
  float container_height = ImGui::GetContentRegionAvail().y - (40 * resources.scale_factor);
  ImGui::BeginChild("Clocks", ImVec2(container_width, container_height));

  ImGui::EndChild();
  ImGui::SameLine();

  ImGui::BeginChild(
      "Options", ImVec2(container_width, container_height), ImGuiChildFlags_None,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
  );

  ImGui::EndChild();
}

void ClocksConfig::ui_section_top() {
  if (locale_changed) {
    load_locale(configuration.locale);
    locale = LANGUAGE + " ";
    locale += locales[configuration.locale == Locale::Auto ? loaded_locale() : configuration.locale];
    locale += " ";
    locale_space = round(ImGui::CalcTextSize(locale.data()).x + (BASE_SIZE * resources.scale_factor) + (gap * 5));
    locale_changed = false;
  }

  short buttons = debug_level ? 2 : 1;
  float start_x = available_x() - locale_space - (button_space * buttons);

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
