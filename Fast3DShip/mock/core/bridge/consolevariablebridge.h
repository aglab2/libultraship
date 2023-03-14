#pragma once

#include <string_view>

#include <stdint.h>
#include <stdlib.h>

#include "../../../../Fast3DShip/plugin.h"

static inline int32_t CVarGetInteger(std::string_view name, int32_t defaultValue) {
    if (name == "gMSAAValue") {
        return Plugin::config().msaa;
    } else if (name == "gDirtPathFix") {
        return (int) Plugin::config().depthBiasType;
    } else {
        abort();
    }
}

static inline float CVarGetFloat(const char* name, float defaultValue) {
    if (name == "gInternalResolution") {
        return defaultValue;
    } else {
        abort();
    }
}

static inline void CVarSetString(const char* name, const char* value) {
}
static inline void CVarSetInteger(const char* name, int32_t value) {
}
static inline void CVarSave() {
}
