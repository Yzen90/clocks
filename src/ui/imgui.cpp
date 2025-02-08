#include "imgui.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <easylogging++.h>
#include <imgui.h>

#include "../i18n/l10n.hpp"
#include "SDL3/SDL_gpu.h"
#include "dialogs.hpp"

const int MIN_WIDTH = 640;
const int MIN_HEIGHT = 480;

bool apps_use_light_theme() { return false; }

optional<Resources> setup(void*& window_handle, Theme theme) {
  el::Logger* logger = el::Loggers::getLogger("ui");
  Resources resources;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    logger->error(l10n->ui.errors.sdl_init + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_init + " " + SDL_GetError(), l10n->ui.title, window_handle);
    SDL_Quit();
    return {};
  }

  resources.window = SDL_CreateWindow(
      l10n->ui.title.data(), MIN_WIDTH, MIN_HEIGHT,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_UTILITY
  );
  if (!resources.window) {
    logger->error(l10n->ui.errors.sdl_create_window + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_create_window + " " + SDL_GetError(), l10n->ui.title, window_handle);
    cleanup(resources);
    return {};
  }

  if (!SDL_SetWindowMinimumSize(resources.window, MIN_WIDTH, MIN_HEIGHT)) {
  }

#ifdef NOT_RELEASE_MODE
  bool debug_mode = true;
#else
  bool debug_mode = false;
#endif

  resources.gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, debug_mode, nullptr);
  if (!resources.gpu) {
    logger->error(l10n->ui.errors.sdl_create_gpu_device + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_create_gpu_device + " " + SDL_GetError(), l10n->ui.title, window_handle);
    cleanup(resources);
    return {};
  }

  if (!SDL_ClaimWindowForGPUDevice(resources.gpu, resources.window)) {
    logger->error(l10n->ui.errors.sdl_claim_window + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_claim_window + " " + SDL_GetError(), l10n->ui.title, window_handle);
    cleanup(resources);
    return {};
  }
  SDL_SetGPUSwapchainParameters(
      resources.gpu, resources.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
      SDL_WindowSupportsGPUPresentMode(resources.gpu, resources.window, SDL_GPU_PRESENTMODE_MAILBOX)
          ? SDL_GPU_PRESENTMODE_MAILBOX
          : SDL_GPU_PRESENTMODE_VSYNC
  );

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  set_theme(theme);

  ImGui_ImplSDL3_InitForSDLGPU(resources.window);

  ImGui_ImplSDLGPU3_InitInfo init_info{
      resources.gpu, SDL_GetGPUSwapchainTextureFormat(resources.gpu, resources.window), SDL_GPU_SAMPLECOUNT_1
  };
  ImGui_ImplSDLGPU3_Init(&init_info);

  logger->verbose(0, l10n->ui.messages.setup_complete + " " + SDL_GetGPUDeviceDriver(resources.gpu));
  return {resources};
}

void set_theme(Theme theme) {
  if ((theme == Theme::Auto && apps_use_light_theme()) || theme == Theme::Light)
    ImGui::StyleColorsLight();
  else
    ImGui::StyleColorsDark();
}

bool keep_open(const Resources& resources) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_QUIT ||
        (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(resources.window)))
      return false;
  }

  return true;
}

void new_frame() {
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void render(const Resources& resources) {
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();
  const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

  SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(resources.gpu);

  SDL_GPUTexture* swapchain_texture;
  SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, resources.window, &swapchain_texture, nullptr, nullptr);

  if (swapchain_texture != nullptr && !is_minimized) {
    Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);  // Mandatory!

    SDL_GPUColorTargetInfo target_info = {};
    target_info.texture = swapchain_texture;
    target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    target_info.store_op = SDL_GPU_STOREOP_STORE;
    target_info.mip_level = 0;
    target_info.layer_or_depth_plane = 0;
    target_info.cycle = false;
    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

    SDL_EndGPURenderPass(render_pass);
  }

  SDL_SubmitGPUCommandBuffer(command_buffer);
}

bool is_minimized(const Resources& resources) {
  if (SDL_GetWindowFlags(resources.window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return true;
  }

  return false;
}

void cleanup(const Resources& resources) {
  if (resources.gpu) {
    SDL_WaitForGPUIdle(resources.gpu);

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(resources.gpu, resources.window);
    SDL_DestroyGPUDevice(resources.gpu);
  }
  if (resources.window) SDL_DestroyWindow(resources.window);
  SDL_Quit();
}
