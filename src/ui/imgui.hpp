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
  HWND parent;
  ImGuiIO* io;
  string driver;
  int dpi;
  int scale;
  float scale_factor;
  float real_scale;
  bool light_theme;
  short real_min_width;
  short real_min_height;
};

void load_fonts(ImGuiIO& io, const Locale locale, const short base_size, bool create_texture = true);
void set_theme(Theme theme, Resources* resources);

optional<Resources> setup(
    void*& window_handle, Theme theme, const short base_size, short min_width, short min_height, Locale locale
);
bool keep_open(const Resources& resources);
bool is_minimized(const Resources& resources);

void new_frame();
void render(const Resources& resources);
void cleanup(Resources* resources);

void with_font_scale(float scale, function<void()> imgui_ops, float reset_scale = 1);
float current_x();
float available_x();
float end_x();
void move_x(float distance);
float current_y();
float available_y();
float end_y();
void move_y(float distance);

static bool apps_use_light_theme();
static bool show_splash(float scale, HWND parent);
