#pragma once

#include <SDL3/SDL.h>

#include <optional>

using std::optional;

struct Resources {
  SDL_Window* window;
  SDL_GPUDevice* gpu;
};

#include "../StateStore.hpp"

optional<Resources> setup(void*& window_handle, Theme theme);

void set_theme(Theme theme);

bool keep_open(const Resources& resources);

bool is_minimized(const Resources& resources);

void new_frame();

void render(const Resources& resources);

void cleanup(const Resources& resources);
