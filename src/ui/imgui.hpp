#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>

#include <optional>

#include "../StateStore.hpp"

using std::optional;

struct Resources {
  SDL_Window* window;
  SDL_GPUDevice* gpu;
  ImGuiIO* io;
  string driver;
};

optional<Resources> setup(void*& window_handle, Theme theme);

void set_theme(Theme theme);

bool keep_open(const Resources& resources);

bool is_minimized(const Resources& resources);

void new_frame();

void render(const Resources& resources);

void cleanup(const Resources& resources);

static bool show_splash();
static bool apps_use_light_theme();
