#include "assets.hpp"

static constexpr const unsigned char splash_image[] = {
#embed "splash.png"
};

static constexpr const size_t splash_size = sizeof(splash_image);

static constexpr  const unsigned char latin_font[] = {
#embed "latin.ttf"
};
static constexpr const size_t latin_size = sizeof(latin_font);
