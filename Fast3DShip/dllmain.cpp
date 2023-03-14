#include <Windows.h>

#include "plugin.h"

#include "gfx_pc.h"
#include "gfx_direct3d11.h"
#include "gfx_dxgi.h"
#include "gfx_opengl.h"
#include "gfx_ucode.h"
#include "dwnd.h"
#include "tool_ui.h"
#include "queue_executor.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <future>
#include <thread>

static std::atomic_bool gFullscreen = false;
static bool gCanDispatch = false;
static std::mutex gRSPQueueMutex;
static QueueExecutor gRSPQueue;

static void plugin_init(void) {
    auto& config = Plugin::config();

    switch (config.renderingApi) {
        case RenderingAPI::D3D11:
            gfx_init(&gfx_dxgi_api, &gfx_direct3d11_api, "SM64", gFullscreen /*fullscreen*/);
            break;
        case RenderingAPI::OPENGL:
            gfx_init(&gfx_dwnd, &gfx_opengl_api, "SM64", gFullscreen);
            break;
    }

    // TODO: This should be configurable
    gfx_get_current_rendering_api()->set_texture_filter(FILTER_LINEAR);

    gfx_current_dimensions.width = Plugin::config().width;
    gfx_current_dimensions.height = Plugin::config().height;
    gfx_current_game_window_viewport.x = 0;
    gfx_current_game_window_viewport.y = 0;
    gfx_current_game_window_viewport.width = Plugin::config().width;
    gfx_current_game_window_viewport.height = Plugin::config().height;
}

static void plugin_deinit(void) {
    gfx_deinit();
}

static uint32_t gUCStart = -1;
static uint32_t gUCDStart = -1;
static uint32_t gUCDSize = -1;

static void load_ucode(uint32_t ucStart, uint32_t ucDStart, uint32_t ucDSize) {
    if (gUCStart == ucStart)
        return;

    gUCStart = ucStart;
    gUCDStart = ucDStart;
    gUCDSize = ucDSize;

    gUCode = parseUcode(ucStart, ucDStart, ucDSize);
}

static void plugin_dl(void) {
    auto& info = Plugin::info();
    auto dlistStart = *(uint32_t*)(info.DMEM + 0xff0);
    auto dlistSize = *(uint32_t*)(info.DMEM + 0xff4);

    auto ucStart = *(uint32_t*)(info.DMEM + 0xfd0);
    auto ucDStart = *(uint32_t*)(info.DMEM + 0xfd8);
    auto ucDSize = *(uint32_t*)(info.DMEM + 0xfdc);

    load_ucode(ucStart, ucDStart, ucDSize);
    gfx_start_frame();
    gfx_run((Gfx*)&info.RDRAM[dlistStart], {});
}

static void plugin_draw(void) {
    gfx_end_frame();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

/******************************************************************
  Function: CaptureScreen
  Purpose:  This function dumps the current frame to a file
  input:    pointer to the directory to save the file to
  output:   none
*******************************************************************/
EXPORT void CALL CaptureScreen(char* Directory) {
}

/******************************************************************
  Function: ChangeWindow
  Purpose:  to change the window between fullscreen and window
            mode. If the window was in fullscreen this should
            change the screen to window mode and vice vesa.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL ChangeWindow(void) {
    gFullscreen = !gFullscreen;
    gfx_get_current_window_manager_api()->set_fullscreen(gFullscreen);
}

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL CloseDLL(void) {
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/
EXPORT void CALL DllAbout(HWND hParent) {
}

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/
EXPORT void CALL DllConfig(HWND hParent) {
    UI::show(hParent);
    {
        std::lock_guard<std::mutex> lck(gRSPQueueMutex);
        if (gCanDispatch) {
            gRSPQueue.async([]() {
                plugin_deinit();
                plugin_init();
            });
        }
    }
}

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/
EXPORT void CALL DllTest(HWND hParent) {
}

/******************************************************************
  Function: DrawScreen
  Purpose:  This function is called when the emulator receives a
            WM_PAINT message. This allows the gfx to fit in when
            it is being used in the desktop.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL DrawScreen(void) {
}

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/
EXPORT void CALL GetDllInfo(PLUGIN_INFO* PluginInfo) {
    PluginInfo->MemoryBswaped = true;
    strncpy_s(PluginInfo->Name, sizeof(PluginInfo->Name), Plugin::name(), sizeof(PluginInfo->Name));
    PluginInfo->NormalMemory = true;
    PluginInfo->Type = PLUGIN_TYPE_GFX;
    PluginInfo->Version = 0x0103;
}

/******************************************************************
  Function: InitiateGFX
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 graphics
            uses. This is not called from the emulation thread.
  Input:    Gfx_Info is passed to this function which is defined
            above.
  Output:   TRUE on success
            FALSE on failure to initialise

  ** note on interrupts **:
  To generate an interrupt set the appropriate bit in MI_INTR_REG
  and then call the function CheckInterrupts to tell the emulator
  that there is a waiting interrupt.
*******************************************************************/
EXPORT BOOL CALL InitiateGFX(GFX_INFO Gfx_Info) {
    Plugin::setInfo(Gfx_Info);
    return TRUE;
}

/******************************************************************
  Function: MoveScreen
  Purpose:  This function is called in response to the emulator
            receiving a WM_MOVE passing the xpos and ypos passed
            from that message.
  input:    xpos - the x-coordinate of the upper-left corner of the
            client area of the window.
            ypos - y-coordinate of the upper-left corner of the
            client area of the window.
  output:   none
*******************************************************************/
EXPORT void CALL MoveScreen(int xpos, int ypos) {
}

/******************************************************************
  Function: ProcessDList
  Purpose:  This function is called when there is a Dlist to be
            processed. (High level GFX list)
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL ProcessDList(void) {
    auto& cfg = Plugin::config();
    QueueExecutor::SyncToken token;
    {
        std::lock_guard<std::mutex> lck(gRSPQueueMutex);
        if (gCanDispatch) {
            token = gRSPQueue.sync(plugin_dl);
            gRSPQueue.async(plugin_draw);
        }
    }
}

/******************************************************************
  Function: ProcessRDPList
  Purpose:  This function is called when there is a Dlist to be
            processed. (Low level GFX list)
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL ProcessRDPList(void) {
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL RomClosed(void) {
    auto& config = Plugin::config();
    {
        std::lock_guard<std::mutex> lck(gRSPQueueMutex);
        if (gCanDispatch) {
            gRSPQueue.async(plugin_deinit);
            gCanDispatch = false;
        }
    }
    gRSPQueue.stop();
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the
            emulation thread)
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL RomOpen(void) {
    auto& config = Plugin::config();

    gRSPQueue.start(RenderingAPI::OPENGL != config.renderingApi);
    {
        std::lock_guard<std::mutex> lck(gRSPQueueMutex);
        gCanDispatch = true;
        gRSPQueue.async(plugin_init);
    }
}

/******************************************************************
  Function: ShowCFB
  Purpose:  Useally once Dlists are started being displayed, cfb is
            ignored. This function tells the dll to start displaying
            them again.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL ShowCFB(void) {
}

/******************************************************************
  Function: UpdateScreen
  Purpose:  This function is called in response to a vsync of the
            screen were the VI bit in MI_INTR_REG has already been
            set
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL UpdateScreen(void) {
}

/******************************************************************
  Function: ViStatusChanged
  Purpose:  This function is called to notify the dll that the
            ViStatus registers value has been changed.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL ViStatusChanged(void) {
}

/******************************************************************
  Function: ViWidthChanged
  Purpose:  This function is called to notify the dll that the
            ViWidth registers value has been changed.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL ViWidthChanged(void) {
}