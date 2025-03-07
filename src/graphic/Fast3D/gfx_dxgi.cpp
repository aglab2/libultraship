#if defined(ENABLE_DX11) || defined(ENABLE_DX12)

#include <stdint.h>
#include <math.h>

#include <map>
#include <set>
#include <string>

#include <windows.h>
#include <wrl/client.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>
#include <versionhelpers.h>

#include <shellscalingapi.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include "libultraship/libultra/gbi.h"

#include "gfx_window_manager_api.h"
#include "gfx_rendering_api.h"
#include "gfx_direct3d_common.h"
#include "gfx_screen_config.h"
#include "gfx_pc.h"
#include "menu/ImGuiImpl.h"
#include "core/bridge/consolevariablebridge.h"
#include "misc/Hooks.h"

#include "../../../Fast3DShip/plugin.h"

#define DECLARE_GFX_DXGI_FUNCTIONS
#include "gfx_dxgi.h"

#define WINCLASS_NAME L"N64GAME"
#define GFX_BACKEND_NAME "DXGI"

#define FRAME_INTERVAL_NS_NUMERATOR 1000000000
#define FRAME_INTERVAL_NS_DENOMINATOR (dxgi.target_fps)

using namespace Microsoft::WRL; // For ComPtr

static struct {
    HWND h_wnd;
    uint32_t current_width, current_height;
    bool is_running = true;

    HMODULE dxgi_module;
    HRESULT(__stdcall* CreateDXGIFactory1)(REFIID riid, void** factory);
    HRESULT(__stdcall* CreateDXGIFactory2)(UINT flags, REFIID iid, void** factory);

    bool process_dpi_awareness_done;

    RECT last_window_rect;
    bool is_full_screen, last_maximized_state;

    bool dxgi1_4;
    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGISwapChain1> swap_chain;
    HANDLE waitable_object;
    ComPtr<IUnknown> swap_chain_device; // D3D11 Device or D3D12 Command Queue
    std::function<void()> before_destroy_swap_chain_fn;
    uint64_t qpc_init, qpc_freq;
    bool dropped_frame;
    float detected_hz;
    UINT length_in_vsync_frames;
    uint32_t target_fps;
    uint32_t maximum_frame_latency;
    uint32_t applied_maximum_frame_latency;

    void (*on_fullscreen_changed)(bool is_now_fullscreen);
    void (*run_one_game_iter)(void);
    bool (*on_key_down)(int scancode);
    bool (*on_key_up)(int scancode);
    void (*on_all_keys_up)(void);
} dxgi = { };

static void load_dxgi_library(void) {
    dxgi.dxgi_module = LoadLibraryW(L"dxgi.dll");
    *(FARPROC*)&dxgi.CreateDXGIFactory1 = GetProcAddress(dxgi.dxgi_module, "CreateDXGIFactory1");
    *(FARPROC*)&dxgi.CreateDXGIFactory2 = GetProcAddress(dxgi.dxgi_module, "CreateDXGIFactory2");
}

template <typename Fun> static void run_as_dpi_aware(Fun f) {
    // Make sure Windows 8.1 or newer doesn't upscale/downscale the rendered images.
    // This is an issue on Windows 8.1 and newer where moving around the window
    // between different monitors having different scaling settings will
    // by default result in the DirectX image will also be scaled accordingly.
    // The resulting scale factor is the curent monitor's scale factor divided by
    // the initial monitor's scale factor. Setting per-monitor aware disables scaling.

    // On Windows 10 1607 and later, that is solved by setting the awarenenss per window,
    // which is done by using SetThreadDpiAwarenessContext before and after creating
    // any window. When the message handler runs, the corresponding context also applies.

    // From windef.h, missing in MinGW.
    DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_UNAWARE ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED ((DPI_AWARENESS_CONTEXT)-5)

    DPI_AWARENESS_CONTEXT(WINAPI * SetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT dpiContext);
    *(FARPROC*)&SetThreadDpiAwarenessContext =
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext");
    DPI_AWARENESS_CONTEXT old_awareness_context = nullptr;
    if (SetThreadDpiAwarenessContext != nullptr) {
        old_awareness_context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    } else {
        // Solution for Windows 8.1 and newer, but before Windows 10 1607.
        // SetProcessDpiAwareness must be called before any drawing related API is called.
        if (!dxgi.process_dpi_awareness_done) {
            HMODULE shcore_module = LoadLibraryW(L"SHCore.dll");
            if (shcore_module != nullptr) {
                HRESULT(WINAPI * SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);
                *(FARPROC*)&SetProcessDpiAwareness = GetProcAddress(shcore_module, "SetProcessDpiAwareness");
                if (SetProcessDpiAwareness != nullptr) {
                    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                    // Ignore result, will fail if already called or manifest already specifies dpi awareness.
                }
                FreeLibrary(shcore_module);
            }
            dxgi.process_dpi_awareness_done = true;
        }
    }

    f();

    // Restore the old context
    if (SetThreadDpiAwarenessContext != nullptr && old_awareness_context != nullptr) {
        SetThreadDpiAwarenessContext(old_awareness_context);
    }
}

static void apply_maximum_frame_latency(bool first) {
    ComPtr<IDXGISwapChain2> swap_chain2;
    if (dxgi.swap_chain->QueryInterface(__uuidof(IDXGISwapChain2), &swap_chain2) == S_OK) {
        ThrowIfFailed(swap_chain2->SetMaximumFrameLatency(dxgi.maximum_frame_latency));
        if (first) {
            dxgi.waitable_object = swap_chain2->GetFrameLatencyWaitableObject();
            WaitForSingleObject(dxgi.waitable_object, INFINITE);
        }
    } else {
        ComPtr<IDXGIDevice1> device1;
        ThrowIfFailed(dxgi.swap_chain->GetDevice(__uuidof(IDXGIDevice1), &device1));
        ThrowIfFailed(device1->SetMaximumFrameLatency(dxgi.maximum_frame_latency));
    }
    dxgi.applied_maximum_frame_latency = dxgi.maximum_frame_latency;
}

static void toggle_borderless_window_full_screen(bool enable, bool call_callback) {
    // Windows 7 + flip mode + waitable object can't go to exclusive fullscreen,
    // so do borderless instead. If DWM is enabled, this means we get one monitor
    // sync interval of latency extra. On Win 10 however (maybe Win 8 too), due to
    // "fullscreen optimizations" the latency is eliminated.

    if (enable == dxgi.is_full_screen) {
        return;
    }

    static LONG windowedStyle;
    static HMENU windowedMenu;

    if (!enable) {
        RECT r = dxgi.last_window_rect;

        // Set in window mode with the last saved position and size
        SetWindowLongPtr(dxgi.h_wnd, GWL_STYLE, windowedStyle);
        SetWindowLongPtr(dxgi.h_wnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES | WS_EX_APPWINDOW);

        if (dxgi.last_maximized_state) {
            SetWindowPos(dxgi.h_wnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(dxgi.h_wnd, SW_MAXIMIZE);
        } else {
            SetWindowPos(dxgi.h_wnd, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED);
            ShowWindow(dxgi.h_wnd, SW_RESTORE);
        }

        if (windowedMenu)
            SetMenu(dxgi.h_wnd, windowedMenu);

        if (Plugin::info().hStatusBar)
            ShowWindow(Plugin::info().hStatusBar, SW_SHOW);

        dxgi.is_full_screen = false;
    } else {
        // Save if window is maximized or not
        WINDOWPLACEMENT window_placement;
        window_placement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(dxgi.h_wnd, &window_placement);
        dxgi.last_maximized_state = window_placement.showCmd == SW_SHOWMAXIMIZED;

        // Save window position and size if the window is not maximized
        GetWindowRect(dxgi.h_wnd, &dxgi.last_window_rect);

        // Get in which monitor the window is
        HMONITOR h_monitor = MonitorFromWindow(dxgi.h_wnd, MONITOR_DEFAULTTONEAREST);

        // Get info from that monitor
        MONITORINFOEX monitor_info;
        monitor_info.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(h_monitor, &monitor_info);
        RECT r = monitor_info.rcMonitor;

        windowedStyle = GetWindowLongPtr(dxgi.h_wnd, GWL_STYLE);

        // Set borderless full screen to that monitor
        SetWindowLongPtr(dxgi.h_wnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        SetWindowPos(dxgi.h_wnd, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED);

        windowedMenu = GetMenu(dxgi.h_wnd);
        if (windowedMenu)
            SetMenu(dxgi.h_wnd, NULL);

        if (Plugin::info().hStatusBar)
            ShowWindow(Plugin::info().hStatusBar, SW_HIDE);

        dxgi.is_full_screen = true;
    }

    if (dxgi.on_fullscreen_changed != nullptr && call_callback) {
        dxgi.on_fullscreen_changed(enable);
    }
}

static void onkeydown(WPARAM w_param, LPARAM l_param) {
    int key = ((l_param >> 16) & 0x1ff);
    if (dxgi.on_key_down != nullptr) {
        dxgi.on_key_down(key);
    }
}
static void onkeyup(WPARAM w_param, LPARAM l_param) {
    int key = ((l_param >> 16) & 0x1ff);
    if (dxgi.on_key_up != nullptr) {
        dxgi.on_key_up(key);
    }
}

static void calculate_sync_interval() {
    const POINT ptZero = { 0, 0 };
    HMONITOR h_monitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFOEX monitor_info;
    monitor_info.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(h_monitor, &monitor_info);

    DEVMODE dev_mode;
    dev_mode.dmSize = sizeof(DEVMODE);
    dev_mode.dmDriverExtra = 0;
    EnumDisplaySettings(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &dev_mode);

    if (dev_mode.dmDisplayFrequency >= 29 && dev_mode.dmDisplayFrequency <= 31) {
        dxgi.length_in_vsync_frames = 1;
    } else if (dev_mode.dmDisplayFrequency >= 59 && dev_mode.dmDisplayFrequency <= 61) {
        dxgi.length_in_vsync_frames = 2;
    } else if (dev_mode.dmDisplayFrequency >= 89 && dev_mode.dmDisplayFrequency <= 91) {
        dxgi.length_in_vsync_frames = 3;
    } else if (dev_mode.dmDisplayFrequency >= 119 && dev_mode.dmDisplayFrequency <= 121) {
        dxgi.length_in_vsync_frames = 4;
    } else {
        dxgi.length_in_vsync_frames = 0;
    }
}

void gfx_dxgi_init(const char* game_name, const char* gfx_api_name, bool start_in_fullscreen, uint32_t width,
                   uint32_t height) {
    Plugin::resize(start_in_fullscreen);

    LARGE_INTEGER qpc_init, qpc_freq;
    QueryPerformanceCounter(&qpc_init);
    QueryPerformanceFrequency(&qpc_freq);
    dxgi.qpc_init = qpc_init.QuadPart;
    dxgi.qpc_freq = qpc_freq.QuadPart;

    dxgi.target_fps = 60;
    dxgi.maximum_frame_latency = 1;

    // Prepare window title

    dxgi.h_wnd = Plugin::hWnd();

    load_dxgi_library();

    if (start_in_fullscreen) {
        toggle_borderless_window_full_screen(true, false);
    }

    auto vsyncMode = Plugin::config().vsyncMode;
    if (vsyncMode == VsyncMode::AUTOMATIC) {
        calculate_sync_interval();
    } else {
        dxgi.length_in_vsync_frames = (UINT)vsyncMode;
    }
}

void gfx_dxgi_deinit() {
    // TODO: Hacky, use cpp dtors
    HMODULE dxgi_module = std::move(dxgi.dxgi_module);
    CloseHandle(dxgi.waitable_object);
    dxgi = {};
    FreeLibrary(dxgi_module);
}

static void gfx_dxgi_close() {
    dxgi.is_running = false;
}

static void gfx_dxgi_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
    dxgi.on_fullscreen_changed = on_fullscreen_changed;
}

static void gfx_dxgi_set_cursor_visibility(bool visible) {
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showcursor
    // https://devblogs.microsoft.com/oldnewthing/20091217-00/?p=15643
    // ShowCursor uses a counter, not a boolean value, and increments or decrements that value when called
    // This means we need to keep calling it until we get the value we want
    int cursorVisibilityCounter;
    if (visible) {
        do {
            cursorVisibilityCounter = ShowCursor(true);
        } while (cursorVisibilityCounter < 0);
    } else {
        do {
            cursorVisibilityCounter = ShowCursor(false);
        } while (cursorVisibilityCounter >= 0);
    }
}

static void gfx_dxgi_set_fullscreen(bool enable) {
    Plugin::resize(enable);
    toggle_borderless_window_full_screen(enable, true);
}

static void gfx_dxgi_get_active_window_refresh_rate(uint32_t* refresh_rate) {
    *refresh_rate = (uint32_t)roundf(dxgi.detected_hz);
}

static void gfx_dxgi_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode),
                                            void (*on_all_keys_up)(void)) {
    dxgi.on_key_down = on_key_down;
    dxgi.on_key_up = on_key_up;
    dxgi.on_all_keys_up = on_all_keys_up;
}

static void gfx_dxgi_main_loop(void (*run_one_game_iter)(void)) {
    dxgi.run_one_game_iter = run_one_game_iter;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) && dxgi.is_running) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Ship::ExecuteHooks<Ship::ExitGame>();
}

static void gfx_dxgi_get_dimensions(uint32_t* width, uint32_t* height) {
    *width = dxgi.current_width;
    *height = dxgi.current_height;
}

static void gfx_dxgi_handle_events(void) {
    /*MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }*/
}

static uint64_t qpc_to_ns(uint64_t qpc) {
    return qpc / dxgi.qpc_freq * 1000000000 + qpc % dxgi.qpc_freq * 1000000000 / dxgi.qpc_freq;
}

static uint64_t qpc_to_100ns(uint64_t qpc) {
    return qpc / dxgi.qpc_freq * 10000000 + qpc % dxgi.qpc_freq * 10000000 / dxgi.qpc_freq;
}

static bool gfx_dxgi_start_frame(void) {
    return true;
}

static void gfx_dxgi_swap_buffers_begin(void) {
    ThrowIfFailed(dxgi.swap_chain->Present(dxgi.length_in_vsync_frames, 0));
    dxgi.dropped_frame = false;
}

static void gfx_dxgi_swap_buffers_end(void) {
    if (dxgi.applied_maximum_frame_latency > dxgi.maximum_frame_latency) {
        // There seems to be a bug that if latency is decreased, there is no effect of that operation, so recreate swap
        // chain
        if (dxgi.waitable_object != nullptr) {
            if (!dxgi.dropped_frame) {
                // Wait the last time on this swap chain
                WaitForSingleObject(dxgi.waitable_object, INFINITE);
            }
            CloseHandle(dxgi.waitable_object);
            dxgi.waitable_object = nullptr;
        }

        dxgi.before_destroy_swap_chain_fn();

        dxgi.swap_chain.Reset();

        gfx_dxgi_create_swap_chain(dxgi.swap_chain_device.Get(), move(dxgi.before_destroy_swap_chain_fn));

        return; // Make sure we don't wait a second time on the waitable object, since that would hang the program
    } else if (dxgi.applied_maximum_frame_latency != dxgi.maximum_frame_latency) {
        apply_maximum_frame_latency(false);
    }

    if (!dxgi.dropped_frame) {
        if (dxgi.waitable_object != nullptr) {
            WaitForSingleObject(dxgi.waitable_object, 1000);
        }
        // else TODO: maybe sleep until some estimated time the frame will be shown to reduce lag
    }
}

static double gfx_dxgi_get_time(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)(t.QuadPart - dxgi.qpc_init) / dxgi.qpc_freq;
}

static void gfx_dxgi_set_target_fps(int fps) {
    dxgi.target_fps = fps;
}

static void gfx_dxgi_set_maximum_frame_latency(int latency) {
    dxgi.maximum_frame_latency = latency;
}

void gfx_dxgi_create_factory_and_device(bool debug, int d3d_version,
                                        bool (*create_device_fn)(IDXGIAdapter1* adapter, bool test_only)) {
    if (dxgi.CreateDXGIFactory2 != nullptr) {
        ThrowIfFailed(
            dxgi.CreateDXGIFactory2(debug ? DXGI_CREATE_FACTORY_DEBUG : 0, __uuidof(IDXGIFactory2), &dxgi.factory));
    } else {
        ThrowIfFailed(dxgi.CreateDXGIFactory1(__uuidof(IDXGIFactory2), &dxgi.factory));
    }

    {
        ComPtr<IDXGIFactory4> factory4;
        if (dxgi.factory->QueryInterface(__uuidof(IDXGIFactory4), &factory4) == S_OK) {
            dxgi.dxgi1_4 = true;
        }
    }

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; dxgi.factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & 2 /*DXGI_ADAPTER_FLAG_SOFTWARE*/) { // declaration missing in mingw headers
            continue;
        }
        if (create_device_fn(adapter.Get(), true)) {
            break;
        }
    }
    create_device_fn(adapter.Get(), false);
}

void gfx_dxgi_create_swap_chain(IUnknown* device, std::function<void()>&& before_destroy_fn) {
    bool win8 = IsWindows8OrGreater();                 // DXGI_SCALING_NONE is only supported on Win8 and beyond
    bool dxgi_13 = dxgi.CreateDXGIFactory2 != nullptr; // DXGI 1.3 introduced waitable object

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
    swap_chain_desc.BufferCount = 3;
    swap_chain_desc.Width = 0;
    swap_chain_desc.Height = 0;
    swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.Scaling = win8 ? DXGI_SCALING_NONE : DXGI_SCALING_STRETCH;
    swap_chain_desc.SwapEffect =
        dxgi.dxgi1_4 ? DXGI_SWAP_EFFECT_FLIP_DISCARD : // Introduced in DXGI 1.4 and Windows 10
            DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // Apparently flip sequential was also backported to Win 7 Platform Update
    swap_chain_desc.Flags = dxgi_13 ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0;
    swap_chain_desc.SampleDesc.Count = 1;

    run_as_dpi_aware([&]() {
        // When setting size for the buffers, the values that DXGI puts into the desc (that can later be retrieved by
        // GetDesc1) have been divided by the current scaling factor. By making this call dpi aware, no division will be
        // performed. The same goes for IDXGISwapChain::ResizeBuffers(), however that function is currently only called
        // from the message handler.
        ThrowIfFailed(dxgi.factory->CreateSwapChainForHwnd(device, dxgi.h_wnd, &swap_chain_desc, nullptr, nullptr,
                                                           &dxgi.swap_chain));
    });
    ThrowIfFailed(dxgi.factory->MakeWindowAssociation(dxgi.h_wnd, DXGI_MWA_NO_ALT_ENTER));

    apply_maximum_frame_latency(true);

    ThrowIfFailed(dxgi.swap_chain->GetDesc1(&swap_chain_desc));
    dxgi.current_width = swap_chain_desc.Width;
    dxgi.current_height = swap_chain_desc.Height;

    dxgi.swap_chain_device = device;
    dxgi.before_destroy_swap_chain_fn = std::move(before_destroy_fn);
}

void gfx_dxgi_destroy_swap_chain() {
    dxgi.before_destroy_swap_chain_fn();
    dxgi.swap_chain.Reset();
    dxgi.swap_chain_device.Reset();
}

void gfx_dxgi_destroy_factory_and_device() {
    dxgi.factory.Reset();
    dxgi.swap_chain_device.Reset();
}

HWND gfx_dxgi_get_h_wnd(void) {
    return dxgi.h_wnd;
}

IDXGISwapChain1* gfx_dxgi_get_swap_chain() {
    return dxgi.swap_chain.Get();
}

void ThrowIfFailed(HRESULT res) {
    if (FAILED(res)) {
        fprintf(stderr, "Error: 0x%08X\n", res);
        throw res;
    }
}

void ThrowIfFailed(HRESULT res, HWND h_wnd, const char* message) {
    if (FAILED(res)) {
        char full_message[256];
        sprintf(full_message, "%s\n\nHRESULT: 0x%08X", message, res);
        MessageBoxA(h_wnd, full_message, "Error", MB_OK | MB_ICONERROR);
        throw res;
    }
}

const char* gfx_dxgi_get_key_name(int scancode) {
    static char text[64];
    GetKeyNameTextA(scancode << 16, text, 64);
    return text;
}

extern "C" struct GfxWindowManagerAPI gfx_dxgi_api = { gfx_dxgi_init,
                                                       gfx_dxgi_deinit,
                                                       gfx_dxgi_close,
                                                       gfx_dxgi_set_keyboard_callbacks,
                                                       gfx_dxgi_set_fullscreen_changed_callback,
                                                       gfx_dxgi_set_fullscreen,
                                                       gfx_dxgi_get_active_window_refresh_rate,
                                                       gfx_dxgi_set_cursor_visibility,
                                                       gfx_dxgi_main_loop,
                                                       gfx_dxgi_get_dimensions,
                                                       gfx_dxgi_handle_events,
                                                       gfx_dxgi_start_frame,
                                                       gfx_dxgi_swap_buffers_begin,
                                                       gfx_dxgi_swap_buffers_end,
                                                       gfx_dxgi_get_time,
                                                       gfx_dxgi_set_target_fps,
                                                       gfx_dxgi_set_maximum_frame_latency,
                                                       gfx_dxgi_get_key_name };

#endif
