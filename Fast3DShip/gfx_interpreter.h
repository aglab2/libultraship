#pragma once

#include <stdint.h>

enum class InstructionProcessResult {
    STOP,
    CONTINUE,
};

static inline void gfx_inc(void** gfx, uint32_t* segAddr) {
    *gfx = (void*)((char*)(*gfx) + 8);
    *segAddr += 8;
}

static inline void gfx_dec(void** gfx, uint32_t* segAddr) {
    *gfx = (void*)((char*)(*gfx) - 8);
    *segAddr -= 8;
}