#pragma once

#include <cstdint>

typedef struct GlobalImageSize {
	uint32_t width;
	uint32_t height;
} GlobalImageSize;

#ifdef _GLOBALIMAGESIZE_LOCAL_
  #define _SCOPE_ 
#else
  #define _SCOPE_ extern
#endif

// This is used to pass
_SCOPE_ GlobalImageSize globalImageSize;
