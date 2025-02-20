#include "l10n.hpp"

#include <easylogging++.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.UserProfile.h>
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++26-extensions"
#include <glaze/glaze.hpp>
#pragma clang diagnostic pop

#include <string>
#include <string_view>

#include "../shared.hpp"
#include "locales.hpp"
#include "schema.hpp"

using namespace winrt::Windows::System::UserProfile;

using std::invalid_argument;
using std::make_format_args;
using std::string_view;
using std::tie;
using std::tuple;
using std::vformat;

static el::Logger* logger;

unique_ptr<L10N> l10n;

static const L10N::StateStore::Contexts** ss_contexts;
static const L10N::StateStore::Messages** ss_messages;

const Locale DEFAULT_LOCALE = Locale::EN;

LocaleNames locales = {{Locale::EN, "English"}, {Locale::ES, "Español"}};

tuple<Locale, unsigned short> get_prefered_locale() {
  auto languages = GlobalizationPreferences::Languages();
  auto prefered = languages.Size();

  if (prefered > 0) {
    for (const auto language : languages) {
      if (language.starts_with(L"es")) return {Locale::ES, prefered};
    }
  }
  return {DEFAULT_LOCALE, prefered};
}

string_view get_localization(Locale locale) {
  switch (locale) {
    case Locale::Auto:
      throw invalid_argument("Unexpected Locale::Auto @ get_localization");
    case Locale::ES:
      return string_view{reinterpret_cast<const char*>(es_MX), es_MX_size};
      break;
    case DEFAULT_LOCALE:
      return string_view{reinterpret_cast<const char*>(en_US), en_US_size};
  }
}

static Locale locale_loaded = Locale::Auto;

Locale loaded_locale() { return locale_loaded; }

void load_locale(Locale locale) {
  if (logger == nullptr) logger = el::Loggers::getLogger("l10n");
  int prefered = -1;

  if (locale == Locale::Auto) tie(locale, prefered) = get_prefered_locale();
  if (locale_loaded == locale) return;

  auto localization = get_localization(locale);
  auto result = glz::read_json<L10N>(localization);

  if (result) {
    l10n = std::make_unique<L10N>(*result);

    if (ss_contexts) *ss_contexts = &l10n->state_store.contexts;
    if (ss_messages) *ss_messages = &l10n->state_store.messages;

    if (prefered != -1)
      logger->verbose(0, vformat(l10n->l10n.messages.loaded_system, make_format_args(prefered, locales[locale])));
    else
      logger->verbose(0, vformat(l10n->l10n.messages.loaded, make_format_args(locales[locale])));
  } else {
    halt(logger, "Unable to load localization. Cause: " + glz::format_error(result.error(), localization));
  }

  locale_loaded = locale;
}

void use_l10n(const L10N::StateStore::Contexts*& reference) {
  if (ss_contexts == nullptr)
    ss_contexts = &reference;
  else
    throw invalid_argument("Unexpected use of use_l10n for StateStore::Contexts, it should only be called once.");
};

void use_l10n(const L10N::StateStore::Messages*& reference) {
  if (ss_messages == nullptr)
    ss_messages = &reference;
  else
    throw invalid_argument("Unexpected use of use_l10n for StateStore::Messages, it should only be called once.");
};
