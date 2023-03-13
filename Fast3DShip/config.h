#pragma once

#include <string>

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
    Config();

    auto width() const {
        return width_;
    }
    auto height() const {
        return height_;
    }
    auto fullScreenWidth() const {
        return fullScreenWidth_;
    }
    auto fullScreenHeight() const {
        return fullScreenHeight_;
    }
    auto vsyncMode() const {
        return vsyncMode_;
    }
    auto nerfFogFactor() const {
        return nerfFogFactor_;
    }
    auto shadowBias() const {
        return shadowBias_;
    }
    auto renderingApi() const {
        return renderingApi_;
    }
    auto sampleCount() const {
        return sampleCount_;
    }
    auto swapEffect() const {
        return swapEffect_;
    }
    auto scaling() const {
        return scaling_;
    }
    auto swapFlags() const {
        return swapFlags_;
    }
    auto presentFlags() const {
        return presentFlags_;
    }
    auto msaa() const {
        return msaa_;
    }
    auto internalResolution() const {
        return internalResolution_;
    }
    auto depthBiasType() const {
        return depthBiasType_;
    }

    const std::string& configPath() const {
        return configPath_;
    }

  private:
    int width_ = 640;
    int height_ = 480;
    int fullScreenWidth_ = 1920;
    int fullScreenHeight_ = 1080;
    VsyncMode vsyncMode_ = VsyncMode::DISABLED;
    int nerfFogFactor_ = 0;
    float shadowBias_ = 2.f;
    RenderingAPI renderingApi_ = RenderingAPI::D3D11;
    std::string configPath_;
    int sampleCount_ = 1;
    int swapEffect_ = 0;
    int scaling_ = 0;
    int swapFlags_ = 0;
    int presentFlags_ = 0;
    int msaa_ = 1;
    float internalResolution_ = 1.f;
    DepthBiasType depthBiasType_ = DepthBiasType::SCALED_NO_VANISH;

    bool read(const std::string&);
};