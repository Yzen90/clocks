#pragma once

#include <easylogging++.h>

#include <string>

using std::string;

string context(const string& context_id);

void halt(el::Logger* logger, const string& message);
