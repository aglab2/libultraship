#include "config.h"

#include <io.h>
#include <fcntl.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include <shlwapi.h>
#include <Windows.h>

#include <yaml-cpp/yaml.h>

#include "plugin.h"

static const char* getConfigPath() {
    static char path[MAX_PATH]{ 0 };
    if ('\0' == *path) {
        SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path);
        PathAppend(path, "Fast3DShip");
        CreateDirectory(path, nullptr);
        PathAppend(path, "cfg.yaml");
    }
    return path;
}

static VsyncMode toVsyncMode(const std::string& mode) {
    try {
        return (VsyncMode)std::stoul(mode);
    } catch (...) {}

    if ("auto" == mode)
        return VsyncMode::AUTOMATIC;

    return VsyncMode::DISABLED;
}

static RenderingAPI toRenderingApi(std::string api) {
    std::transform(api.begin(), api.end(), api.begin(), [](unsigned char c) { return std::tolower(c); });
    if ("opengl" == api)
        return RenderingAPI::OPENGL;

    return RenderingAPI::D3D11;
}

static std::string toString(RenderingAPI api) {
    switch (api) {
        case RenderingAPI::D3D11:
            return "d3d11";
        case RenderingAPI::OPENGL:
            return "opengl";
    }

    throw std::runtime_error("Bad value");
}

static DepthBiasType toDepthBiasType(std::string dbt) {
    std::transform(dbt.begin(), dbt.end(), dbt.begin(), [](unsigned char c) { return std::tolower(c); });
    if (dbt == "default")
        return DepthBiasType::DEFAULT;
    if (dbt == "n64")
        return DepthBiasType::SCALED_N64_LIKE;
    if (dbt == "no_vanish")
        return DepthBiasType::SCALED_NO_VANISH;

    return DepthBiasType::SCALED_NO_VANISH;
}

static std::string toString(DepthBiasType dbt) {
    switch (dbt) {
        case DepthBiasType::DEFAULT:
            return "default";
        case DepthBiasType::SCALED_N64_LIKE:
            return "n64";
        case DepthBiasType::SCALED_NO_VANISH:
            return "no_vanish";
    }

    throw std::runtime_error("Bad value");
}

bool Config::load() try {
    YAML::Node config = YAML::LoadFile(getConfigPath());

    try {
        width = config["width"].as<int>();
    } catch (...) { width = 640; }
    try {
        height = config["height"].as<int>();
    } catch (...) { height = 480; }
    try {
        vsyncMode = toVsyncMode(config["vsync"].as<std::string>());
    } catch (...) { vsyncMode = VsyncMode::DISABLED; }
    try {
        fullScreenWidth = config["fullScreenWidth"].as<int>();
    } catch (...) { fullScreenWidth = width; }
    try {
        fullScreenHeight = config["fullScreenHeight"].as<int>();
    } catch (...) { fullScreenHeight = height; }
    try {
        renderingApi = toRenderingApi(config["api"].as<std::string>());
    } catch (...) { renderingApi = RenderingAPI::D3D11; }
    try {
        msaa = config["msaa"].as<int>();
    } catch (...) { msaa = 1; }
    try {
        depthBiasType = toDepthBiasType(config["depthBiasType"].as<std::string>());
    } catch (...) { depthBiasType = DepthBiasType::SCALED_NO_VANISH; }

    return true;
} catch (...) {
    return false;
}

void Config::save() {
    try {
        YAML::Node config;

        config["width"] = width;
        config["height"] = height;
        config["vsync"] = (int) vsyncMode;
        config["fullScreenWidth"] = fullScreenWidth;
        config["fullScreenHeight"] = fullScreenHeight;
        config["api"] = toString(renderingApi);
        config["msaa"] = msaa;
        config["depthBiasType"] = toString(depthBiasType);

        YAML::Emitter emitter;
        emitter << config;

        int fd = _open(getConfigPath(), _O_WRONLY | _O_CREAT | _O_TRUNC, 0666);
        if (-1 != fd) {
            _write(fd, emitter.c_str(), emitter.size());
            _close(fd);
        }
    } catch (...) {}
}