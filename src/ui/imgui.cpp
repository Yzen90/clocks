#include "imgui.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
#include <easylogging++.h>
#include <misc/freetype/imgui_freetype.h>
#include <winrt/Windows.Storage.h>

#include "../i18n/l10n.hpp"
#include "assets.hpp"
#include "dialogs.hpp"

using namespace winrt::Windows::Storage;

using std::make_format_args;
using std::nullopt;
using std::vformat;
using std::filesystem::exists;

static el::Logger* logger;
static HWND splash;

optional<Resources> setup(void*& window_handle, Theme theme, const short base_size, short min_width, short min_height) {
  if (logger == nullptr) logger = el::Loggers::getLogger("imgui");
  Resources resources{.parent = static_cast<HWND>(window_handle)};

  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

  short dpi;
  {
    dpi = GetDpiForWindow(resources.parent);
    if (dpi == 0) {
      HDC hdc = GetDC(nullptr);
      dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX));
      ReleaseDC(nullptr, hdc);
    }
  }
  float scale = dpi / 96.0;
  min_width = min_width * scale;
  min_height = min_height * scale;
  resources.real_min_width = min_width;
  resources.real_min_height = min_height;

  EnableWindow(resources.parent, FALSE);
  if (show_splash(scale, resources.parent)) flags |= SDL_WINDOW_HIDDEN;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    logger->error(l10n->ui.errors.sdl_init + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_init + " " + SDL_GetError(), l10n->ui.app_title, window_handle);
    SDL_Quit();
    return nullopt;
  }

  resources.window = SDL_CreateWindow(l10n->ui.app_title.data(), min_width, min_height, flags);
  if (!resources.window) {
    logger->error(l10n->ui.errors.sdl_create_window + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_create_window + " " + SDL_GetError(), l10n->ui.app_title, window_handle);
    cleanup(&resources);
    return nullopt;
  }

  SDL_SetWindowMinimumSize(resources.window, min_width, min_height);

  if (window_handle) {
    HWND handle = static_cast<HWND>(
        SDL_GetPointerProperty(SDL_GetWindowProperties(resources.window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL)
    );
    if (handle) {
      SetLastError(0);
      SetWindowLongPtr(handle, GWLP_HWNDPARENT, (LONG_PTR)window_handle);
      if (GetLastError()) logger->warn(l10n->ui.errors.sdl_set_parent);
    } else {
      logger->warn(l10n->ui.errors.sdl_get_handle);
    }
  }

  // ANCHOR - SDL GPU initialization

#ifdef NOT_RELEASE_MODE
  bool debug_mode = true;
#else
  bool debug_mode = false;
#endif

  resources.gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, debug_mode, nullptr);
  if (!resources.gpu) {
    logger->error(l10n->ui.errors.sdl_create_gpu_device + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_create_gpu_device + " " + SDL_GetError(), l10n->ui.app_title, window_handle);
    cleanup(&resources);
    return nullopt;
  }

  if (!SDL_ClaimWindowForGPUDevice(resources.gpu, resources.window)) {
    logger->error(l10n->ui.errors.sdl_claim_window + " " + SDL_GetError());
    error_message(l10n->ui.errors.sdl_claim_window + " " + SDL_GetError(), l10n->ui.app_title, window_handle);
    cleanup(&resources);
    return nullopt;
  }
  SDL_SetGPUSwapchainParameters(
      resources.gpu, resources.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
      SDL_WindowSupportsGPUPresentMode(resources.gpu, resources.window, SDL_GPU_PRESENTMODE_MAILBOX)
          ? SDL_GPU_PRESENTMODE_MAILBOX
          : SDL_GPU_PRESENTMODE_VSYNC
  );

  // ANCHOR - ImGui initialization

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
  static constexpr const ImWchar bmp_emoji_range[] = {0x00001, 0x1FFFF, 0};

  ImFontConfig font_config;
  font_config.FontDataOwnedByAtlas = false;
  io.Fonts->AddFontFromMemoryTTF(
      const_cast<unsigned char*>(LATIN_FONT), LATIN_SIZE, base_size * 2, &font_config, bmp_emoji_range
  );

  static const path emoji_font = path{static_cast<wstring>(SystemDataPaths::GetDefault().Fonts())} / "seguiemj.ttf";
  if (exists(emoji_font)) {
    ImFontConfig emoji_config;
    emoji_config.MergeMode = true;
    emoji_config.GlyphOffset = ImVec2{-3, 0};
    emoji_config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LoadColor;
    io.Fonts->AddFontFromFileTTF(emoji_font.string().data(), 32, &emoji_config, bmp_emoji_range);
  }

  font_config.MergeMode = true;
  font_config.GlyphOffset = ImVec2{0, 8};
  io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(ICON_FONT), ICON_SIZE, 48, &font_config, bmp_emoji_range);

  resources.scale_factor = scale;
  resources.real_scale = 0.5 * resources.scale_factor;
  io.FontGlobalScale = resources.real_scale;
  ImGui::GetStyle().ScaleAllSizes(resources.real_scale);
  resources.dpi = dpi;
  resources.scale = resources.scale_factor * 100;

  resources.io = &io;

  set_theme(theme, &resources);

  // ANCHOR - ImGui backend initialization

  ImGui_ImplSDL3_InitForSDLGPU(resources.window);

  ImGui_ImplSDLGPU3_InitInfo init_info{
      resources.gpu, SDL_GetGPUSwapchainTextureFormat(resources.gpu, resources.window), SDL_GPU_SAMPLECOUNT_1
  };
  ImGui_ImplSDLGPU3_Init(&init_info);

  resources.driver = SDL_GetGPUDeviceDriver(resources.gpu);
  logger->verbose(
      0, vformat(l10n->ui.messages.setup_complete, make_format_args(resources.driver, resources.scale_factor))
  );
  return {resources};
}

void set_theme(Theme theme, Resources* resources) {
  if ((theme == Theme::Auto && apps_use_light_theme()) || theme == Theme::Light) {
    ImGui::StyleColorsLight();
    resources->light_theme = true;
  } else {
    ImGui::StyleColorsDark();
    resources->light_theme = false;
  }
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

  if (splash) {
    DestroyWindow(splash);
    splash = nullptr;
    SDL_ShowWindow(resources.window);
  }
}

bool is_minimized(const Resources& resources) {
  if (SDL_GetWindowFlags(resources.window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return true;
  }
  return false;
}

void cleanup(Resources* resources) {
  if (resources->gpu) {
    SDL_WaitForGPUIdle(resources->gpu);

    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(resources->gpu, resources->window);
    SDL_DestroyGPUDevice(resources->gpu);
  }
  if (resources->window) SDL_DestroyWindow(resources->window);

  EnableWindow(resources->parent, TRUE);
  SetForegroundWindow(resources->parent);
  SetFocus(resources->parent);
  *resources = Resources{};
  SDL_Quit();
}

// ANCHOR - UI

void with_font_scale(float scale, function<void()> imgui_ops, float reset_scale) {
  ImGui::SetWindowFontScale(scale);
  imgui_ops();
  ImGui::SetWindowFontScale(reset_scale);
}

float current_x() { return ImGui::GetCursorPosX(); };
float available_x() { return ImGui::GetContentRegionAvail().x; };
float end_x() { return ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x; };
void move_x(float distance) { ImGui::SetCursorPosX(distance); }

float current_y() { return ImGui::GetCursorPosY(); };
float available_y() { return ImGui::GetContentRegionAvail().y; };
float end_y() { return ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y; };
void move_y(float distance) { ImGui::SetCursorPosY(distance); }

// ANCHOR - Internal

bool apps_use_light_theme() {
  HKEY key;

  LONG result = RegOpenKeyExW(
      HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &key
  );

  if (result == ERROR_SUCCESS) {
    DWORD value = 1;
    DWORD dataSize = sizeof(value);

    result = RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &dataSize);
    RegCloseKey(key);

    if (result == ERROR_SUCCESS) return (value == 1);
  }

  return true;
}

static const auto MODULE = GetModuleHandle(NULL);
static const auto SPLASH_CLASS = []() {
  const auto classname = L"ClocksSplash";

  WNDCLASSEX splash_class = {0};
  splash_class.cbSize = sizeof(WNDCLASSEX);
  splash_class.lpfnWndProc = DefWindowProc;
  splash_class.hInstance = MODULE;
  splash_class.hCursor = LoadCursor(MODULE, IDC_WAIT);
  splash_class.lpszClassName = classname;
  RegisterClassEx(&splash_class);

  return classname;
}();

// ANCHOR - Internal: Splash screen

const unsigned int COLORS = 256, SPLASH_SIZE = 256;

const size_t INFO_POS = sizeof(BITMAPFILEHEADER);
const BITMAPINFOHEADER* SPLASH_INFO = reinterpret_cast<const BITMAPINFOHEADER*>(SPLASH_IMAGE + INFO_POS);

const size_t COLOR_POS = INFO_POS + sizeof(BITMAPINFOHEADER);
const RGBQUAD* SPLASH_COLORS = reinterpret_cast<const RGBQUAD*>(SPLASH_IMAGE + COLOR_POS);

HPALETTE splash_palette() {
  struct {
    WORD palVersion = 0x300;
    WORD palNumEntries = COLORS;
    PALETTEENTRY palPalEntry[COLORS];
  } log_palette;

  for (int i = 0; i < COLORS; ++i) {
    log_palette.palPalEntry[i].peRed = SPLASH_COLORS[i].rgbRed;
    log_palette.palPalEntry[i].peGreen = SPLASH_COLORS[i].rgbGreen;
    log_palette.palPalEntry[i].peBlue = SPLASH_COLORS[i].rgbBlue;
    log_palette.palPalEntry[i].peFlags = 0;
  }

  return CreatePalette(reinterpret_cast<const LOGPALETTE*>(&log_palette));
};

bool show_splash(float scale, HWND parent) {
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  short splash_size = 256 * scale;

  splash = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"ClocksSplash", NULL, WS_POPUP | WS_VISIBLE, (screenWidth - splash_size) / 2,
      (screenHeight - splash_size) / 2, splash_size, splash_size, parent, NULL, MODULE, NULL
  );

  if (splash) {
    const HPALETTE palette = splash_palette();
    bool stretch = splash_size != SPLASH_SIZE;

    HDC splash_context = GetDC(splash);
    SelectPalette(splash_context, palette, TRUE);
    RealizePalette(splash_context);
    SetStretchBltMode(splash_context, stretch ? HALFTONE : COLORONCOLOR);
    if (stretch) SetBrushOrgEx(splash_context, 0, 0, nullptr);

    StretchDIBits(
        splash_context, 0, 0, splash_size, splash_size, 0, 0, SPLASH_SIZE, SPLASH_SIZE,
        SPLASH_IMAGE + COLOR_POS + (sizeof(RGBQUAD) * COLORS), reinterpret_cast<const BITMAPINFO*>(SPLASH_INFO),
        DIB_RGB_COLORS, SRCCOPY
    );

    DeleteObject(palette);
    ReleaseDC(splash, splash_context);
    return true;
  }
  return false;
}
