#include "dwnd.h"
#include "plugin.h"

#include "GL/glew.h"
#include "gfx_screen_config.h"

#include "plugin.h"

static HGLRC hRC;
static HDC hDC;

static void dwnd_init(const char* game_name, const char* gfx_api_name, bool start_in_fullscreen, uint32_t width,
                      uint32_t height) {
    auto pluginNameW = Plugin::name();
    auto& gfxInfo = Plugin::info();

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
        1,                             // version number
        PFD_DRAW_TO_WINDOW |           // support window
            PFD_SUPPORT_OPENGL |       // support OpenGL
            PFD_DOUBLEBUFFER,          // double buffered
        PFD_TYPE_RGBA,                 // RGBA type
        32,                            // color depth
        0,
        0,
        0,
        0,
        0,
        0, // color bits ignored
        0, // no alpha buffer
        0, // shift bit ignored
        0, // no accumulation buffer
        0,
        0,
        0,
        0,              // accum bits ignored
        32,             // z-buffer
        0,              // no stencil buffer
        0,              // no auxiliary buffer
        PFD_MAIN_PLANE, // main layer
        0,              // reserved
        0,
        0,
        0 // layer masks ignored
    };

    if (gfxInfo.hWnd == NULL)
        gfxInfo.hWnd = GetActiveWindow();

    if ((hDC = GetDC(gfxInfo.hWnd)) == NULL) {
        MessageBox(gfxInfo.hWnd, "Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK);
        return;
    }

    int pixelFormat;
    if ((pixelFormat = ChoosePixelFormat(hDC, &pfd)) == 0) {
        MessageBox(gfxInfo.hWnd, "Unable to find a suitable pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
        return;
    }

    if ((SetPixelFormat(hDC, pixelFormat, &pfd)) == FALSE) {
        MessageBox(gfxInfo.hWnd, "Error while setting pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
        return;
    }

    if ((hRC = wglCreateContext(hDC)) == NULL) {
        MessageBox(gfxInfo.hWnd, "Error while creating OpenGL context!", pluginNameW, MB_ICONERROR | MB_OK);
        RomClosed();
        return;
    }

    if (!wglMakeCurrent(hDC, hRC)) {
        MessageBox(gfxInfo.hWnd, "Failed to activate OpenGL 3.3 rendering context.", pluginNameW, MB_ICONERROR | MB_OK);
        return;
    }

    glewInit();

    auto vsync = Plugin::config().vsyncMode();
    if (vsync == VsyncMode::AUTOMATIC) {
        wglSwapIntervalEXT(-1);
    } else {
        wglSwapIntervalEXT((int)vsync);
    }

    Plugin::resize(false);
}

void dwnd_close(void) {
    auto& gfxInfo = Plugin::info();

    wglMakeCurrent(NULL, NULL);

    if (hRC != NULL) {
        wglDeleteContext(hRC);
        hRC = NULL;
    }

    if (hDC != NULL) {
        ReleaseDC(gfxInfo.hWnd, hDC);
        hDC = NULL;
    }
}

void dwnd_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode),
                                 void (*on_all_keys_up)(void)) {
}

void dwnd_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
}

void dwnd_set_fullscreen(bool enable) {
}

void dwnd_get_active_window_refresh_rate(uint32_t* refresh_rate) {
}

void dwnd_set_cursor_visibility(bool visible) {
}

void dwnd_main_loop(void (*run_one_game_iter)(void)) {
}

void dwnd_get_dimensions(uint32_t* width, uint32_t* height) {
    *width = Plugin::config().width();
    *height = Plugin::config().height();
}

void dwnd_handle_events(void) {
}

bool dwnd_start_frame(void) {
    return true;
}

void dwnd_swap_buffers_begin(void) {
    if (hDC == NULL)
        SwapBuffers(wglGetCurrentDC());
    else
        SwapBuffers(hDC);
}

void dwnd_swap_buffers_end(void) {
}

double dwnd_get_time(void) {
    return 0.;
}

void dwnd_set_target_fps(int fps) {
}

void dwnd_set_maximum_frame_latency(int latency) {
}

const char* dwnd_get_key_name(int scancode) {
    return nullptr;
}

struct GfxWindowManagerAPI gfx_dwnd = {
    dwnd_init,
    dwnd_close,
    dwnd_set_keyboard_callbacks,
    dwnd_set_fullscreen_changed_callback,
    dwnd_set_fullscreen,
    dwnd_get_active_window_refresh_rate,
    dwnd_set_cursor_visibility,
    dwnd_main_loop,
    dwnd_get_dimensions,
    dwnd_handle_events,
    dwnd_start_frame,
    dwnd_swap_buffers_begin,
    dwnd_swap_buffers_end,
    dwnd_get_time,
    dwnd_set_target_fps,
    dwnd_set_maximum_frame_latency,
    dwnd_get_key_name,
};
