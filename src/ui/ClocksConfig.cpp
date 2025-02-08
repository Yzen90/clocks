#include "ClocksConfig.hpp"

#include <imgui.h>

#include <thread>

#include "imgui.hpp"

ClocksConfig::ClocksConfig(Configuration configuration) : configuration(configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto resources = setup(window_handle, configuration.theme)) {
      ImGuiIO& io = ImGui::GetIO();
      current_theme = configuration.theme;

      static float f = 0.0f;

      while (keep_open(*resources)) {
        if (is_minimized(*resources)) continue;

        if (current_theme != configuration.theme) {
          current_theme = configuration.theme;
          set_theme(current_theme);
        }

        new_frame();

        ImGui::Begin(l10n->ui.title.data());
        ImGui::Text("avg %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        render(*resources);
      }

      cleanup(*resources);
    }
  }};

  ui.join();

  if (configuration_changed) return {configuration};
  return {};
}
