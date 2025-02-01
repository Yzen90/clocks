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

template <>
struct glz::meta<Locale> {
  using enum Locale;
  static constexpr auto value = enumerate(Auto, EN, ES);
};

static el::Logger* logger;

unique_ptr<L10N> l10n;

static const L10N::StateStore::Contexts** ss_contexts;
static const L10N::StateStore::Messages** ss_messages;

const Locale DEFAULT_LOCALE = Locale::EN;

typedef std::map<Locale, string> LocaleNames;
const LocaleNames& locale_names() {
  static const LocaleNames locale_names{{Locale::EN, "English"}, {Locale::ES, "Español"}};
  return locale_names;
};

Locale get_prefered_locale() {
  auto languages = GlobalizationPreferences::Languages();
  auto prefered = languages.Size();

  logger->verbose(0, "System prefered languages: " + std::to_string(prefered));

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

static Locale* current_locale;

void load_locale(Locale locale) {
  if (current_locale != nullptr && *current_locale == locale) return;

  if (logger == nullptr) logger = el::Loggers::getLogger("l10n");

  if (locale == Locale::Auto) locale = get_prefered_locale();

  auto localization = get_localization(locale);
  auto result = glz::read_json<L10N>(localization);

  if (result.has_value()) {
    l10n = std::make_unique<L10N>(result.value());

    if (ss_contexts) *ss_contexts = &l10n->state_store.contexts;
    if (ss_messages) *ss_messages = &l10n->state_store.messages;

    logger->verbose(
        0, context(l10n->i18n.contexts.load) + l10n->i18n.messages.loaded + " " + locale_names().at(locale)
    );
  } else {
    halt(logger, "Unable to load localization. Cause: " + glz::format_error(result.error(), localization));
  }

  current_locale = &locale;
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
