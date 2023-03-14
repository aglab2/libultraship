#pragma once

enum class VsyncMode {
    DISABLED = 0,
    V1 = 1,
    V2 = 2,
    V3 = 3,
    V4 = 4,
    V5 = 5,

    AUTOMATIC = 100,
};

enum class RenderingAPI {
    D3D11,
    OPENGL,
};

enum class DepthBiasType {
    DEFAULT = 0,
    SCALED_N64_LIKE = 1,
    SCALED_NO_VANISH = 2,
};

class Config {
  public:
    Config() {
        load();
    }

    bool load();
    void save();

    int width = 640;
    int height = 480;
    int fullScreenWidth = 1920;
    int fullScreenHeight = 1080;
    VsyncMode vsyncMode = VsyncMode::DISABLED;
    RenderingAPI renderingApi = RenderingAPI::D3D11;
    int msaa = 1;
    float internalResolution = 1.f;
    DepthBiasType depthBiasType = DepthBiasType::SCALED_NO_VANISH;
};
