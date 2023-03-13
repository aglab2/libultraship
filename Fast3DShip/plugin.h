#pragma once

#include <Windows.h>
#include "Gfx #1.3.h"

#include "config.h"

class Plugin {
  public:
    static const char* name() {
        return me().sName();
    }
    static void setInfo(GFX_INFO i) {
        return me().sSetInfo(i);
    }
    static GFX_INFO& info() {
        return me().sInfo();
    }
    static HWND hWnd() {
        return me().sHWnd();
    }
    static int statusBarHeight() {
        return me().sStatusBarHeight();
    }
    static const Config& config() {
        return me().config_;
    }

    static void resize(bool fs) {
        return me().sResize(fs);
    }

  private:
    const char* sName();
    void sSetInfo(GFX_INFO);
    GFX_INFO& sInfo();
    HWND sHWnd();
    int sStatusBarHeight();

    void sResize(bool fs);

    static Plugin& me() {
        static Plugin plugin;
        return plugin;
    }

    GFX_INFO gfxInfo_{};
    bool fullscreen_ = false;
    RECT statusRect_{};
    Config config_;
};