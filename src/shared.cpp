#include "shared.hpp"

string context(const string &context_id) { return "[" + context_id + "] "; }

string to_lower(const string_view &source) {
  string copy{source};
  std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) {
    return (c >= 'A' && c <= 'Z') ? c + 32 : c;
  });
  return copy;
}

void halt(el::Logger *logger, const string &message) {
  logger->fatal(message);
  logger->flush();
  std::abort();
};
