#include "l10n.hpp"

#include <easylogging++.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.UserProfile.h>

#include <glaze/glaze.hpp>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

#include "../shared.hpp"
#include "locales.hpp"

using namespace winrt::Windows::System::UserProfile;
using std::string_view;

static el::Logger* logger = el::Loggers::getLogger("l10n");

unique_ptr<L10N> l10n;

const Locale DEFAULT_LOCALE = Locale::EN;

const std::map<Locale, string> LOCALE_NAMES = {{Locale::EN, "English"}, {Locale::ES, "Español"}};

Locale get_prefered_locale() {
  auto languages = GlobalizationPreferences::Languages();
  auto prefered = languages.Size();

  logger->verbose(0, "System prefered languages count: " + std::to_string(prefered));

  if (prefered > 0) {
    for (const auto language : languages) {
      if (language.starts_with(L"es")) return Locale::ES;
    }
  }

  return DEFAULT_LOCALE;
}

void load_locale(Locale locale) {
  string_view localization;

  if (locale == Locale::Auto) locale = get_prefered_locale();

  switch (locale) {
    case Locale::Auto:
      throw std::invalid_argument("Unexpected state @load_locale");
    case Locale::ES:
      localization = string_view{reinterpret_cast<char*>(locale_es_MX), locale_es_MX_len};
      break;
    case DEFAULT_LOCALE:
      localization = string_view{reinterpret_cast<char*>(locale_en_US), locale_en_US_len};
  }

  auto result = glz::read_json<L10N>(localization);

  if (result.has_value()) {
    l10n = std::make_unique<L10N>(result.value());
    logger->verbose(0, context(l10n->i18n.contexts.load) + l10n->i18n.messages.loaded);
  } else {
    logger->fatal("Unable to load localization.");
    logger->flush();
    std::abort();
  }
}
