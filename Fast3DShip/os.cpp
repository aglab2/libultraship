#include "os.h"

#include <Windows.h>

namespace OS {
namespace {
double getSysOpType() {
    static double sType = 0;
    if (sType)
        return sType;

    int ret = 0;
    NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW);
    OSVERSIONINFOEXW osInfo;

    *(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

    if (NULL != RtlGetVersion) {
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        RtlGetVersion(&osInfo);
        ret = osInfo.dwMajorVersion;
    }
    sType = ret;
    return ret;
}
} // namespace

int W8Point1OrLater() {
    return getSysOpType() >= 8.1;
}

int W8OrLater() {
    return getSysOpType() >= 8;
}

std::string ExecutablePath() {
    int count = 128;
    std::string path;
    do {
        count *= 2;
        path = std::string(count, 0);
    } while (0 == GetModuleFileName(NULL, (char*)path.data(), count));

    return path;
}
}; // namespace OS
