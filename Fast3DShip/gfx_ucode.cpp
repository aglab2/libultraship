#include "gfx_ucode.h"

#include "gfx_f3d.h"
#include "gfx_f3dex.h"
#include "gfx_f3dex2.h"
#include "gfx_interpreter.h"

#include "plugin.h"

#include <string.h>

UCodes gUCode;

void gfx_run_dl(void* cmd, uint32_t segAddr) {
    switch (gUCode) {
        case UCodes::F3D:
            return gfx_run_dl_F3D(cmd, segAddr);

        case UCodes::F3DEX:
            return gfx_run_dl_F3DEX(cmd, segAddr);

        case UCodes::F3DEX2:
            return gfx_run_dl_F3DEX2(cmd, segAddr);
    }
}

static void UnswapCopyWrap(const BYTE* src, uint32_t srcIdx, char* dest, uint32_t destIdx, uint32_t destMask,
                           uint32_t numBytes) {
    // copy leading bytes
    uint32_t leadingBytes = srcIdx & 3;
    if (leadingBytes != 0) {
        leadingBytes = 4 - leadingBytes;
        if ((uint32_t)leadingBytes > numBytes)
            leadingBytes = numBytes;
        numBytes -= leadingBytes;

        srcIdx ^= 3;
        for (uint32_t i = 0; i < leadingBytes; i++) {
            dest[destIdx & destMask] = src[srcIdx];
            ++destIdx;
            --srcIdx;
        }
        srcIdx += 5;
    }

    // copy dwords
    int numDWords = numBytes >> 2;
    while (numDWords--) {
        dest[(destIdx + 3) & destMask] = src[srcIdx++];
        dest[(destIdx + 2) & destMask] = src[srcIdx++];
        dest[(destIdx + 1) & destMask] = src[srcIdx++];
        dest[(destIdx + 0) & destMask] = src[srcIdx++];
        destIdx += 4;
    }

    // copy trailing bytes
    int trailingBytes = numBytes & 3;
    if (trailingBytes) {
        srcIdx ^= 3;
        for (int i = 0; i < trailingBytes; i++) {
            dest[destIdx & destMask] = src[srcIdx];
            ++destIdx;
            --srcIdx;
        }
    }
}

UCodes parseUcode(uint32_t ucStart, uint32_t ucDStart, uint32_t ucDSize) {
    auto rdram = Plugin::info().RDRAM;

    char uc_data[2048];
    UnswapCopyWrap(rdram, ucDStart & 0x1FFFFFFF, uc_data, 0, 0x7FF, 2048);
    char uc_str[256];

    for (int i = 0; i < 2046; ++i) {
        if ((uc_data[i] == 'R') && (uc_data[i + 1] == 'S') && (uc_data[i + 2] == 'P')) {
            int j = 0;
            while (uc_data[i + j] > 0x0A) {
                uc_str[j] = uc_data[i + j];
                j++;
            }

            uc_str[j] = 0x00;
            if (strncmp(&uc_str[4], "SW", 2) == 0) {
                return UCodes::F3D;
            } else if (strncmp(&uc_str[4], "Gfx", 3) == 0) {
                if (strncmp(&uc_str[14], "F3D", 3) == 0) {
                    if (uc_str[28] == '1' || strncmp(&uc_str[28], "0.95", 4) == 0 ||
                        strncmp(&uc_str[28], "0.96", 4) == 0)
                        return UCodes::F3DEX;
                    else if (uc_str[31] == '2') {
                        return UCodes::F3DEX2;
                    }
                }
            }

            break;
        }
    }

    abort();
}
