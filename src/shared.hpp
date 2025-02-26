#pragma once

#include <easylogging++.h>

#include <string>

using std::string;
using std::string_view;

string context(const string& context_id);

string to_lower(const string_view& source);

void halt(el::Logger* logger, const string& message);
