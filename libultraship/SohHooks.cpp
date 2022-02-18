#include "SohHooks.h"
#include <map>
#include <string>
#include <vector>
#include <stdarg.h>
#include <iostream>

std::map<std::string, std::vector<HookFunc>> listeners;

namespace Moon {
    void registerHookListener(HookListener listener) {
        listeners[listener.hookName].push_back(listener.callback);
    }
}

namespace MoonInternal {
    bool handleHook(HookCall call) {
        std::string hookName = std::string(call.name);
        for (int l = 0; l < listeners[hookName].size(); l++) {
            (listeners[hookName][l])(call);
        }
        return call.cancelled;
    }
}


std::string hookName;
std::map<std::string, void*> initArgs;
std::map<std::string, void*> hookArgs;

/*
#############################
   Module: Hook C++ Handle
#############################
*/

namespace MoonInternal {
    void bindHook(std::string name) {
        hookName = name;
    }

    void initBindHook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                initArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }
    }

    bool callBindHook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                hookArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }

        bool cancelled = MoonInternal::handleHook({
            .name = hookName,
            .baseArgs = initArgs,
            .hookedArgs = hookArgs
            });

        hookName = "";
        initArgs.clear();
        hookArgs.clear();

        return cancelled;
    }
}

/*
#############################
    Module: Hook C Handle
#############################
*/

extern "C" {

    void moon_bind_hook(char* name) {
        hookName = std::string(name);
    }

    void moon_init_hook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                initArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }
    }

    bool moon_call_hook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                hookArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }

        bool cancelled = MoonInternal::handleHook({
            .name = hookName,
            .baseArgs = initArgs,
            .hookedArgs = hookArgs
            });

        hookName = "";
        initArgs.clear();
        hookArgs.clear();

        return cancelled;
    }

}