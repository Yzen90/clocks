#include "assets.hpp"

static constexpr const unsigned char SPLASH_IMAGE[] = {
#embed "splash.dib"
};

static constexpr const unsigned char LATIN_FONT[] = {
#embed "NotoSans-Regular.ttf"
};
static constexpr const size_t LATIN_SIZE = sizeof(LATIN_FONT);

static constexpr const unsigned char ICON_FONT[] = {
#embed "icons.ttf"
};
static constexpr const size_t ICON_SIZE = sizeof(ICON_FONT);

static constexpr const unsigned char SUPPORT_FONT[] = {
#embed "support.ttf"
};
static constexpr const size_t SUPPORT_SIZE = sizeof(SUPPORT_FONT);
