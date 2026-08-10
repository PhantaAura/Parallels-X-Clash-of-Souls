#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace px {

struct DialogueLine {
    std::string speaker;
    std::string text;
    std::string portraitId;
    std::string expression{"neutral"};
    std::string focusActorId;

    DialogueLine(std::string speakerValue, std::string textValue,
                 std::string portraitValue = {}, std::string expressionValue = "neutral",
                 std::string focusValue = {})
        : speaker(std::move(speakerValue)), text(std::move(textValue)),
          portraitId(std::move(portraitValue)), expression(std::move(expressionValue)),
          focusActorId(std::move(focusValue)) {}
};

class DialogueRegistry {
public:
    DialogueRegistry();
    const std::vector<DialogueLine>& get(const std::string& sceneId) const;
    bool has(const std::string& sceneId) const;

private:
    std::unordered_map<std::string, std::vector<DialogueLine>> scenes_;
};

} // namespace px
