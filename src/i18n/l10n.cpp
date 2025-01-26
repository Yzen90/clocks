#include "l10n.hpp"

#include <easylogging++.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.UserProfile.h>

#include <glaze/glaze.hpp>
#include <map>
#include <string>
#include <string_view>

#include "../shared.hpp"
#include "locales.hpp"
#include "schema.hpp"

using namespace winrt::Windows::System::UserProfile;

using std::invalid_argument;
using std::string_view;

static el::Logger* logger = el::Loggers::getLogger("l10n");

unique_ptr<L10N> l10n;

static L10N::StateStore::Contexts** ss_contexts;
static L10N::StateStore::Messages** ss_messages;

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

string_view get_localization(Locale locale) {
  switch (locale) {
    case Locale::Auto:
      throw invalid_argument("Unexpected Locale::Auto @ get_localization");
    case Locale::ES:
      return string_view{reinterpret_cast<char*>(locale_es_MX), locale_es_MX_len};
      break;
    case DEFAULT_LOCALE:
      return string_view{reinterpret_cast<char*>(locale_en_US), locale_en_US_len};
  }
}

void load_locale(Locale locale) {
  if (locale == Locale::Auto) locale = get_prefered_locale();

  auto localization = get_localization(locale);
  auto result = glz::read_json<L10N>(localization);

  if (result.has_value()) {
    l10n = std::make_unique<L10N>(result.value());

    if (ss_contexts) *ss_contexts = &l10n->state_store.contexts;
    if (ss_messages) *ss_messages = &l10n->state_store.messages;

    logger->verbose(0, context(l10n->i18n.contexts.load) + l10n->i18n.messages.loaded);
  } else {
    halt(logger, "Unable to load localization. Cause: " + glz::format_error(result.error(), localization));
  }
}

void use_l10n(L10N::StateStore::Contexts*& reference) {
  if (ss_contexts == nullptr)
    ss_contexts = &reference;
  else
    throw invalid_argument("Unexpected use of use_l10n for StateStore::Contexts, it should only be called once.");
};

void use_l10n(L10N::StateStore::Messages*& reference) {
  if (ss_messages == nullptr)
    ss_messages = &reference;
  else
    throw invalid_argument("Unexpected use of use_l10n for StateStore::Messages, it should only be called once.");
};
