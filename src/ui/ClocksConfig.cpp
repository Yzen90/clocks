#include "ClocksConfig.hpp"

#include <imgui.h>

#include <thread>

#include "imgui.hpp"

ClocksConfig::ClocksConfig(Configuration configuration) : configuration(configuration) {}

optional<Configuration> ClocksConfig::open(void*& window_handle) {
  std::thread ui{[&]() {
    if (auto resources = setup(window_handle, configuration.theme)) {
      while (keep_open(*resources)) {
        if (is_minimized(*resources)) continue;

        new_frame();

        ImGui::ShowDemoWindow();

        render(*resources);
      }

      cleanup(*resources);
    }
  }};

  ui.join();

  return {};
}
