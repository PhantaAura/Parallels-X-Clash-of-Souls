#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace px {

struct StoryRecapFrame {
    std::string id;
    std::string caption;
    std::string body;
    std::string visualId;
    float suggestedSeconds{10.0f};
};

struct StoryRecapSection {
    std::string id;
    std::string title;
    std::vector<StoryRecapFrame> frames;
};

class StoryRecapRegistry {
public:
    StoryRecapRegistry();
    const std::vector<StoryRecapSection>& sections() const { return sections_; }
    const StoryRecapSection& get(const std::string& id) const;
    float suggestedRuntimeSeconds() const;
    std::size_t totalFrameCount() const;

private:
    std::vector<StoryRecapSection> sections_;
};

} // namespace px
