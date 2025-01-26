#pragma once

#include <memory>

#include "schema.hpp"

using std::unique_ptr;

extern unique_ptr<L10N> l10n;

enum class Locale { Auto, ES, EN };

void load_locale(Locale locale = Locale::Auto);

void use_l10n(L10N::StateStore::Contexts*& reference);
void use_l10n(L10N::StateStore::Messages*& reference);
