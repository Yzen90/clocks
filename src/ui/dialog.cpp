#include "dialog.hpp"

#include <Shobjidl.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Popups.h>

#include <future>
#include <memory>

using namespace winrt::Windows::UI::Popups;

void show_dialog(std::wstring content, std::wstring title, void*& window_handle) {
  MessageDialog dialog(content, title);
  dialog.as<::IInitializeWithWindow>()->Initialize(reinterpret_cast<HWND>(window_handle));
  auto async = dialog.ShowAsync();

  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();

  async.Completed([promise]() { promise->set_value(); });

  future.get();
}
