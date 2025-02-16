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
  string driver;
  int dpi;
  int scale;
  float scale_factor;
  float real_scale;
  bool light_theme;
};

void set_theme(Theme theme, Resources* resources);

optional<Resources> setup(void*& window_handle, Theme theme);
bool keep_open(const Resources& resources);
bool is_minimized(const Resources& resources);

void new_frame();
void render(const Resources& resources);
void cleanup(Resources* resources);

void with_font_scale(float scale, function<void()> imgui_ops);
float available_x();
void move_x(float distance);

static bool apps_use_light_theme();
static bool show_splash();
