#include "assets.hpp"

static constexpr const unsigned char SPLASH_IMAGE[] = {
#embed "splash.dib"
};

static constexpr  const unsigned char LATIN_FONT[] = {
#embed "NotoSans-Regular.ttf"
};
static constexpr const size_t LATIN_SIZE = sizeof(LATIN_FONT);
