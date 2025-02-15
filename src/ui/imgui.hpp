#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>

#include <functional>
#include <optional>

#include "../StateStore.hpp"

using std::function;
using std::optional;

struct Resources {
  SDL_Window* window;
  SDL_GPUDevice* gpu;
  ImGuiIO* io;
  ImGuiStyle* style;
  string driver;
  int dpi;
  int scale;
  bool light_theme;
};

optional<Resources> setup(void*& window_handle, Theme theme);

void set_theme(Theme theme, Resources* resources);

bool keep_open(const Resources& resources);

bool is_minimized(const Resources& resources);

void new_frame();

void render(const Resources& resources);

void cleanup(const Resources& resources);

void with_font_scale(float scale, function<void()> imgui_ops);

static bool apps_use_light_theme();
static bool show_splash();
