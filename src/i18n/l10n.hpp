#pragma once

#include <memory>
#include <string>

#include "locales.hpp"
#include "schema.hpp"

using std::string;
using std::unique_ptr;

extern unique_ptr<L10N> l10n;

extern void load_locale();
