#include "game.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ASSERT GAME_ASSERT
#define STBI_MALLOC AllocPtr
#define STBI_REALLOC ReallocPtr
#define STBI_FREE SafeDisposePtr

#include "stb_image.h"
