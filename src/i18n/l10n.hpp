#pragma once

#include <map>
#include <memory>

#include "schema.hpp"

using std::unique_ptr;

extern unique_ptr<L10N> l10n;

enum class Locale { Auto, EN, ES, JA, ZH };
struct LocaleName {
  const string name;
  const string display_name;
};
typedef std::map<Locale, const LocaleName> LocaleNames;

extern LocaleNames locales;

void load_locale(Locale locale);
Locale loaded_locale();

void use_l10n(const L10N::StateStore::Contexts*& reference);
void use_l10n(const L10N::StateStore::Messages*& reference);
