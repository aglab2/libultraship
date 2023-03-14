#include <ui.h>

#include <windows.h>
#include <ui_windows.h>

#include "plugin.h"

#include <bit>
#include <functional>
#include <stdlib.h>
#include <string>
#include <memory>

#if defined _M_IX86
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

namespace UI {
static const std::string DefaultLine = std::string(22, ' ');
static constexpr int MaxHotKeys = 3;

// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
    rtrim(s);
    ltrim(s);
}

// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}

class Window {
  public:
    Window(HWND hwnd) : hwnd_(hwnd) {
        EnableWindow(hwnd_, FALSE);
    }

    virtual ~Window() {
        EnableWindow(hwnd_, TRUE);
        SetActiveWindow(hwnd_);
    }

  protected:
    HWND hwnd_;

    void show(uiWindow* window) {
        uiWindowOnClosing(
            window,
            [](uiWindow* w, void* data) {
                ((Window*)data)->onClosing();
                return 1;
            },
            this);
        uiControlShow(uiControl(window));
    }

    virtual void onClosing() {
        uiQuit();
    }
};

class MainWindow : public Window {
  public:
    explicit MainWindow(HWND hParent) : Window(hParent) {
        window_ = uiNewWindow("Fast3DShip", 200, 200, 0, hwnd_);
        uiWindowSetMargined(window_, true);
        uiWindowSetResizeable(window_, false);

        auto main = uiNewHorizontalBox();
        uiBoxSetPadded(main, 1);
        uiWindowSetChild(window_, uiControl(main));

        resolution_ = encodeResolution(Resolution{ Plugin::config().width, Plugin::config().height });
        fullScreenResolution_ =
            encodeResolution(Resolution{ Plugin::config().fullScreenWidth, Plugin::config().fullScreenHeight });
        renderer_ = (int)Plugin::config().renderingApi;
        vsync_ = toComboBoxInt(Plugin::config().vsyncMode);
        msaa_ = std::bit_width((unsigned) Plugin::config().msaa) - 1;
        depthType_ = (int)Plugin::config().depthBiasType;

        {
            auto group = uiNewGroup("General");
            auto box = uiNewVerticalBox();
            uiGroupSetChild(group, uiControl(box));

            uiBoxAppend(box, uiControl(uiNewLabel("Resolution")), false);
            addEntry(box, uiNewEntry(), &resolution_);
            uiBoxAppend(box, uiControl(uiNewLabel("Fullscreen resolution")), false);
            addEntry(box, uiNewEntry(), &fullScreenResolution_);
            uiBoxAppend(box, uiControl(uiNewLabel("Renderer")), false);
            {
                auto combo = uiNewCombobox();
                uiComboboxAppend(combo, "DirectX 11");
                uiComboboxAppend(combo, "OpenGL");
                addComboBox(box, combo, &renderer_);
            }
            uiBoxAppend(box, uiControl(uiNewLabel("VSync")), false);
            { 
                auto combo = uiNewCombobox();
                uiComboboxAppend(combo, "OFF");
                uiComboboxAppend(combo, "1");
                uiComboboxAppend(combo, "2");
                uiComboboxAppend(combo, "3");
                uiComboboxAppend(combo, "4");
                uiComboboxAppend(combo, "5");
                uiComboboxAppend(combo, "Adaptive");
                addComboBox(box, combo, &vsync_);
            }
            uiBoxAppend(box, uiControl(uiNewLabel("Multi-Sample Anti-Aliasing")), false);
            {
                auto combo = uiNewCombobox();
                uiComboboxAppend(combo, "OFF");
                uiComboboxAppend(combo, "2");
                uiComboboxAppend(combo, "4");
                uiComboboxAppend(combo, "8");
                addComboBox(box, combo, &msaa_);
            }
            uiBoxAppend(box, uiControl(uiNewLabel("Depth Type")), false);
            {
                auto combo = uiNewCombobox();
                uiComboboxAppend(combo, "Default");
                uiComboboxAppend(combo, "Scaled N64-like");
                uiComboboxAppend(combo, "Scaled no vanish");
                addComboBox(box, combo, &depthType_);
            }

            uiBoxAppend(main, uiControl(group), false);
        }
#if 0
        {
            auto group = uiNewGroup("General shortcuts");
            auto grid = uiNewGrid();
            uiGroupSetChild(group, uiControl(grid));

            int counter = 0;
#define HOTKEY(row, view, name, cmd, desc) \
    if (0 == row)                          \
        addHotKey(counter, name##_, grid, desc, Config::get().name);
#include "xhotkeys.h"
#undef HOTKEY

            uiBoxAppend(main, uiControl(group), false);
        }
        {
            auto group = uiNewGroup("Savestate shortcuts");
            auto grid = uiNewGrid();
            uiGroupSetChild(group, uiControl(grid));

            int counter = 0;
#define HOTKEY(row, view, name, cmd, desc) \
    if (1 == row)                          \
        addHotKey(counter, name##_, grid, desc, Config::get().name);
#include "xhotkeys.h"
#undef HOTKEY

            uiBoxAppend(main, uiControl(group), false);
        }
#endif

        show(window_);
    }

  private:
    uiWindow* window_;
    std::string resolution_;
    std::string fullScreenResolution_;
    int renderer_;
    int vsync_;
    int msaa_;
    int depthType_;

    void addCheckbox(uiBox* b, uiCheckbox* cb, bool* modifying) {
        uiCheckboxOnToggled(
            cb, [](auto cb, void* modifying) { *((bool*)modifying) = uiCheckboxChecked(cb); }, modifying);
        uiBoxAppend(b, uiControl(cb), false);
        uiCheckboxSetChecked(cb, *modifying);
    }

    void addEntry(uiBox* b, uiEntry* entry, std::string* modifying) {
        uiEntryOnChanged(
            entry, [](auto entry, void* modifying) { *((std::string*)modifying) = uiEntryText(entry); }, modifying);
        uiBoxAppend(b, uiControl(entry), false);
        uiEntrySetText(entry, modifying->c_str());
    }

    void addComboBox(uiBox* b, uiCombobox* combo, int* modifying) {
        uiComboboxOnSelected(
            combo, [](auto combo, void* modifying) { *((int*)modifying) = uiComboboxSelected(combo); }, modifying);
        uiBoxAppend(b, uiControl(combo), false);
        uiComboboxSetSelected(combo, *modifying);
    }

    struct Resolution
    {
        int width;
        int height;
    };

    Resolution decodeResolution(const std::string& line) { 
        size_t pos = line.find('x');
        if (pos == std::string::npos)
        {
            return {};
        }

        std::string widthStr = line.substr(0, pos);
        trim(widthStr);
        std::string heightStr = line.substr(pos + 1);
        trim(heightStr);

        return { std::atoi(widthStr.c_str()), std::atoi(heightStr.c_str()) };
    }

    std::string encodeResolution(Resolution resolution)
    {
        return std::to_string(resolution.width) + " x " + std::to_string(resolution.height);
    }

    VsyncMode toVsyncMode(int val) {
        if (val > (int) VsyncMode::V5) {
            return VsyncMode::AUTOMATIC;
        } else {
            return (VsyncMode)val;
        }
    }

    int toComboBoxInt(VsyncMode mode) {
        if (mode == VsyncMode::AUTOMATIC) {
            return 1 + (int)VsyncMode::V5;
        } else {
            return (int)mode;
        }
    }

    virtual void onClosing() override {
        auto& config = Plugin::config();

        {
            Resolution resolution = decodeResolution(resolution_);
            config.width = resolution.width;
            config.height = resolution.height;
        }
        {
            Resolution resolution = decodeResolution(fullScreenResolution_);
            config.fullScreenWidth = resolution.width;
            config.fullScreenHeight= resolution.height;
        }
        config.renderingApi = (RenderingAPI)renderer_;
        config.vsyncMode = toVsyncMode(vsync_);
        config.msaa = 1 << msaa_;
        config.depthBiasType = (DepthBiasType)depthType_;

        config.save();
        Window::onClosing();
    }
};

void show(HWND hParent) {
    uiInitOptions o{};
    uiInit(&o);
    auto wnd = std::make_unique<MainWindow>(hParent);
    uiMain(); // blocks till window is closed
    uiUninit();
}
} // namespace UI
