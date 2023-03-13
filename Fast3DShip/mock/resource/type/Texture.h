#pragma once

namespace Ship {
enum class TextureType {
    Error = 0,
    RGBA32bpp = 1,
    RGBA16bpp = 2,
    Palette4bpp = 3,
    Palette8bpp = 4,
    Grayscale4bpp = 5,
    Grayscale8bpp = 6,
    GrayscaleAlpha4bpp = 7,
    GrayscaleAlpha8bpp = 8,
    GrayscaleAlpha16bpp = 9,
};
}
