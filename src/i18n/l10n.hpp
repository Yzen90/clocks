#pragma once

#include <memory>

#include "schema.hpp"

using std::unique_ptr;

extern unique_ptr<L10N> l10n;

enum class Locale { Auto, EN, ES };

void load_locale(Locale locale);

void use_l10n(const L10N::StateStore::Contexts*& reference);
void use_l10n(const L10N::StateStore::Messages*& reference);
