#include "bootstrap.hpp"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <imgui.h>

#include "dialog.hpp"

void bootstrap(void*& window_handle) { show_dialog(L"COLABOLA MESSAGE", L"COLABOLA", window_handle); }
