#include "ClocksConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <thread>

#include "icons.hpp"
#include "imgui.hpp"

using namespace material_symbols;

const short GAP = 5;

ClocksConfig::ClocksConfig(Configuration configuration) : configuration(configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto setup_resources = setup(window_handle, configuration.theme)) {
      resources = std::move(*setup_resources);
      setup_resources.reset();

      current_theme = configuration.theme;
      debug_level = configuration.log_level == LogLevel::DEBUG;

      gap = GAP * resources.scale_factor;
      icon_button_size = ImVec2(32 * resources.scale_factor, 32 * resources.scale_factor);

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

        ui_top_buttons();

        ImGui::BeginChild("Main", ImVec2(0, ImGui::GetContentRegionAvail().y - (36 * resources.scale_factor)));

        for (int i = 0; i < 100; i++) ImGui::Text("%04d: scrollable region", i);

        ImGui::EndChild();

        ui_main_actions();

        ImGui::End();
        ImGui::PopStyleVar(2);

        render(resources);
      }

      cleanup(&resources);
    }
  }};

  ui.join();

  if (is_changed) return {configuration};
  return {};
}

void ClocksConfig::ui_main_actions() {
  ImGui::Separator();

  ImGui::Dummy(ImVec2(1, 1));

  // TODO - Position according to text size
  move_x(available_x() - (99 * resources.scale_factor) - gap);
  ui_primary_button(SAVE_AS + " " + l10n->ui.actions.save + " ", 1.1);
}

void ClocksConfig::ui_top_buttons() {
  short buttons = debug_level ? 2 : 1;
  float button_size = icon_button_size.x;
  float start_x = available_x() - (button_size * buttons) - (gap * buttons);

  move_x(start_x);
  if (debug_level) {
    ui_graphics_metrics();
    move_x(start_x + button_size + gap);
  }
  ui_theme_menu();
}

bool ClocksConfig::ui_primary_button(
    const string& text, optional<float> font_scale, optional<ImVec2> size, optional<ImVec2> padding
) {
  bool click;
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding ? *padding : ImVec2(5, 5));

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

void ClocksConfig::ui_theme_menu() {
  with_font_scale(1.1, [&]() {
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
  });
}

void ClocksConfig::ui_graphics_metrics() {
  if (ui_icon_button(BROWSE_ACTIVITY, 1.1)) show_metrics = !show_metrics;

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
