#include "ClocksConfig.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <thread>

#include "icons.hpp"
#include "imgui.hpp"

ClocksConfig::ClocksConfig(Configuration configuration) : configuration(configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto resources = setup(window_handle, configuration.theme)) {
      using namespace material_symbols;

      current_theme = configuration.theme;

      while (keep_open(*resources)) {
        if (is_minimized(*resources)) continue;

        if (current_theme != configuration.theme) {
          current_theme = configuration.theme;
          set_theme(current_theme, &*resources);
        }

        new_frame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 1);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(resources->io->DisplaySize);

        ImGui::Begin(
            "ClocksConfig", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse
        );

        with_font_scale(1.1, [&]() {
          ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 30);

          bool is_light = resources->light_theme;
          bool show_menu = ImGui::Button(
              configuration.theme == Theme::Auto ? (is_light ? BRIGHTNESS_AUTO.data() : NIGHT_SIGHT_AUTO.data())
                                                 : (is_light ? BRIGHTNESS_HIGH.data() : DARK_MODE.data())
          );
          ImGui::PopStyleVar();

          if (show_menu) ImGui::OpenPopup("theme_menu");

          if (ImGui::BeginPopup("theme_menu")) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 12));
            ImGui::TextDisabled("%s", l10n->ui.theme.title.data());

            if (ImGui::MenuItemEx(l10n->generic.auto_option.data(), ROUTINE.data())) {
              configuration.theme = Theme::Auto;
              set_theme(Theme::Auto, &*resources);
            }

            if (ImGui::MenuItemEx(l10n->ui.theme.light.data(), BRIGHTNESS_HIGH.data())) {
              configuration.theme = Theme::Light;
              set_theme(Theme::Light, &*resources);
            }

            if (ImGui::MenuItemEx(l10n->ui.theme.dark.data(), DARK_MODE.data())) {
              configuration.theme = Theme::Dark;
              set_theme(Theme::Dark, &*resources);
            }
            ImGui::Spacing();
            ImGui::PopStyleVar();
            ImGui::EndPopup();
          }
        });

        ImGui::Text("%s", resources->driver.data());
        ImGui::Text("%.1f fps", resources->io->Framerate);
        ImGui::Text("%.0fx%.0f", resources->io->DisplaySize.x, resources->io->DisplaySize.y);
        ImGui::Text("%ddpi %d%%", resources->dpi, resources->scale);

        ImGui::Text("%s", (l10n->ui.title + material_symbols::BRIGHTNESS_AUTO).data());

        ImGui::End();
        ImGui::PopStyleVar(2);

        render(*resources);
      }

      cleanup(*resources);
    }
  }};

  ui.join();

  if (configuration_changed) return {configuration};
  return {};
}
