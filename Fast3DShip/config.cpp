#include "config.h"

#include <filesystem>
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <Windows.h>

#include "os.h"
#include "plugin.h"

Config::Config() {
    std::filesystem::path exePath(OS::ExecutablePath());
    auto exeDir = exePath.parent_path();

    auto configDir = exeDir / "Config";
    if (!std::filesystem::exists(configDir))
        std::filesystem::create_directory(configDir);

    auto configFile = configDir / "f3d.yaml";
    configPath_ = configFile.string();
    // if (std::filesystem::exists(configFile))
    read(configFile.string());
    // else
    //     write(configFile.u8string());
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

static int toSwapEffect(std::string api) {
    std::transform(api.begin(), api.end(), api.begin(), [](unsigned char c) { return std::tolower(c); });
    if ("discard" == api)
        return 0;
    if ("sequential" == api)
        return 1;
    if ("flipsequential" == api)
        return 3;
    if ("flipdiscard" == api)
        return 4;

    return 0;
}

static int toScaling(std::string api) {
    std::transform(api.begin(), api.end(), api.begin(), [](unsigned char c) { return std::tolower(c); });
    if ("stretch" == api)
        return 0;
    if ("none" == api)
        return 1;
    if ("aspectratiostretch" == api)
        return 2;

    return 0;
}

bool Config::read(const std::string& p) try {
    YAML::Node config = YAML::LoadFile(p);

    width_ = config["width"].as<int>();
    height_ = config["height"].as<int>();
    vsyncMode_ = toVsyncMode(config["vsync"].as<std::string>());
    try {
        nerfFogFactor_ = config["nerfFog"].as<int>();
    } catch (...) {}
    try {
        shadowBias_ = config["shadowBias"].as<float>();
    } catch (...) {}
    try {
        fullScreenWidth_ = config["fullScreenWidth"].as<int>();
    } catch (...) { fullScreenWidth_ = width_; }
    try {
        fullScreenWidth_ = config["fullScreenHeight"].as<int>();
    } catch (...) { fullScreenHeight_ = height_; }
    try {
        renderingApi_ = toRenderingApi(config["api"].as<std::string>());
    } catch (...) { renderingApi_ = RenderingAPI::D3D11; }

    try {
        sampleCount_ = config["sampleCount"].as<int>();
    } catch (...) { sampleCount_ = 1; }

    try {
        auto str = config["swapEffect"].as<std::string>();
        swapEffect_ = toSwapEffect(std::move(str));
    } catch (...) { swapEffect_ = 0; }

    try {
        auto str = config["scaling"].as<std::string>();
        scaling_ = toScaling(std::move(str));
    } catch (...) { scaling_ = 0; }

    try {
        swapFlags_ = config["swapFlags"].as<int>();
    } catch (...) { swapFlags_ = 0; }

    try {
        msaa_ = config["msaa"].as<int>();
    } catch (...) { msaa_ = 1; }

    try {
        depthBiasType_ = (DepthBiasType) config["presentFlags"].as<int>();
    } catch (...) { depthBiasType_ = DepthBiasType::SCALED_NO_VANISH; }

    return true;
} catch (...) {
    MessageBox(nullptr, "Failed to read 'f3d.yaml' config file, using default", "Config Reader", MB_OK | MB_ICONERROR);
    return false;
}
