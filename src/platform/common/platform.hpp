#pragma once
#include "core/input.hpp"
#include "core/math.hpp"
#include <string>

namespace px {

// Platform code can implement these interfaces. Core/content code cannot call OS/3DS APIs directly.
class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    virtual void poll(InputState& state) = 0;
};

class ISaveBackend {
public:
    virtual ~ISaveBackend() = default;
    virtual bool write(const std::string& slot, const std::string& data) = 0;
    virtual bool read(const std::string& slot, std::string& data) = 0;
};

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;
    virtual void playCue(const std::string& cueId) = 0;
};

struct RenderActor {
    std::string modelId;
    Vec2 position;
    float y{0.0f};
    float facingRadians{0.0f};
    std::string animationId;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void beginFrame() = 0;
    virtual void drawActor(const RenderActor& actor) = 0;
    virtual void drawDialogue(const std::string& speaker, const std::string& text) = 0;
    virtual void endFrame() = 0;
};

} // namespace px
