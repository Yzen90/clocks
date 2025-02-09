#pragma clang diagnostic ignored "-Wconstant-conversion"

#include "splash.h"

static const unsigned char splash_image[] = {
  #embed "../../assets/splash.png"
};

const long long splash_size = sizeof(splash_image);
