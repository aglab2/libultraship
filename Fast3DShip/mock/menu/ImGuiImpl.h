#pragma once

#include <imgui.h>

namespace SohImGui {

enum class Backend {
    DX11,
};

typedef union {
    struct {
        void* Handle;
        int Msg;
        int Param1;
        int Param2;
    } Win32;
} EventImpl;

struct WindowImpl {
    Backend backend;
    struct {
        void* Window;
        void* DeviceContext;
        void* Device;
    } Dx11;
};

static inline void Init(WindowImpl) {
}
static inline void Update(EventImpl) {
}
static inline void DrawMainMenuAndCalculateGameSize() {
}
static inline void DrawFramebufferAndGameInput() {
}
static inline void CancelFrame() {
}
static inline void Render() {
}

} // namespace SohImGui
