#include "plugin.h"

const char* Plugin::sName() {
    return "LINK's Fast3DShip v0.1";
}

void Plugin::sSetInfo(GFX_INFO i) {
    gfxInfo_ = i;
    GetWindowRect(gfxInfo_.hStatusBar, &statusRect_);
}

GFX_INFO& Plugin::sInfo() {
    return gfxInfo_;
}

HWND Plugin::sHWnd() {
    return gfxInfo_.hWnd;
}

int Plugin::sStatusBarHeight() {
    return fullscreen_ ? 0 : statusRect_.bottom - statusRect_.top - 1;
}

void Plugin::sResize(bool fs) {
    fullscreen_ = fs;
    int width, height;
    if (fs) {
        width = config_.fullScreenWidth();
        height = config_.fullScreenHeight();
    } else {
        width = config_.width();
        height = config_.height();
    }
    auto hWnd = gfxInfo_.hWnd;
    RECT windowRect;

    GetClientRect(hWnd, &windowRect);

    auto offset = sStatusBarHeight();
    windowRect.right = windowRect.left + width - 1;
    windowRect.bottom = windowRect.top + height - 1 + offset;

    AdjustWindowRect(&windowRect, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != NULL);

    SetWindowPos(hWnd, NULL, 0, 0, windowRect.right - windowRect.left + 1, windowRect.bottom - windowRect.top + 2,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}