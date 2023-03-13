#pragma once

#include <stdint.h>

enum class UCodes {
    F3D,
    F3DEX,
    F3DEX2,
};

extern UCodes gUCode;

UCodes parseUcode(uint32_t ucStart, uint32_t ucDStart, uint32_t ucDSize);

void gfx_run_dl(void* cmd, uint32_t segAddr);
