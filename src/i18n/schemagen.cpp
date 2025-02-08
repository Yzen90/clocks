#include <fstream>
#include <glaze/json/schema.hpp>
#include <iostream>

#include "schema.hpp"

int main() {
  const auto schema = glz::write_json_schema<L10N, glz::opts{.prettify = true}>();

  if (schema) {
    std::ofstream{"l10n.schema.json"} << *schema;
    return 0;
  } else {
    std::cerr << glz::format_error(schema.error()) << std::endl;
    return 1;
  }
}
