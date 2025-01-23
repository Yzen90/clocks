#include "l10n.hpp"

#include <easylogging++.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.UserProfile.h>

#include <cstdlib>
#include <glaze/glaze.hpp>
#include <memory>
#include <string_view>

#include "glaze/json/read.hpp"

using namespace winrt::Windows::System::UserProfile;

using el::Logger;
using el::Loggers;
using std::string_view;

static Logger* logger = Loggers::getLogger("l10n");

unique_ptr<I18N> l10n;

void load_locale() {
  string_view localization;

  auto languages = GlobalizationPreferences::Languages();
  if (languages.Size() > 0 && languages.GetAt(0).starts_with(L"es")) {
    localization = string_view{reinterpret_cast<char*>(locale_es_MX), locale_es_MX_len};
  } else {
    localization = string_view{reinterpret_cast<char*>(locale_en_US), locale_en_US_len};
  }

  auto result = glz::read_json<I18N>(localization);

  if (result.has_value()) {
    l10n = std::make_unique<I18N>(result.value());
    logger->verbose(0, "Localization loaded.");
  } else {
    logger->fatal("Unable to load localization.");
    logger->flush();
    std::abort();
  }
}
