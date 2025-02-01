#include "shared.hpp"

string context(const string &context_id) { return "[" + context_id + "] "; }

void halt(el::Logger *logger, const string &message) {
  logger->fatal(message);
  logger->flush();
  std::abort();
};
