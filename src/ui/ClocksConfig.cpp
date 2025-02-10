#include "ClocksConfig.hpp"

#include <imgui.h>

#include <thread>

#include "imgui.hpp"

ClocksConfig::ClocksConfig(Configuration configuration) : configuration(configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto resources = setup(window_handle, configuration.theme)) {
      current_theme = configuration.theme;

      while (keep_open(*resources)) {
        if (is_minimized(*resources)) continue;

        if (current_theme != configuration.theme) {
          current_theme = configuration.theme;
          set_theme(current_theme);
        }

        new_frame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(resources->io->DisplaySize);

        ImGui::Begin(
            "ClocksConfig", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse
        );

        ImGui::Text("%s", resources->driver.data());
        ImGui::Text("%.1f fps", resources->io->Framerate);
        ImGui::Text("%.0fx%.0f", resources->io->DisplaySize.x, resources->io->DisplaySize.y);
        ImGui::Text("%ddpi %d%%", resources->dpi, resources->scale);

        ImGui::End();
        ImGui::PopStyleVar();

        render(*resources);
      }

      cleanup(*resources);
    }
  }};

  ui.join();

  if (configuration_changed) return {configuration};
  return {};
}
