#pragma once
#include "core/types.hpp"
#include <map>
#include <set>
#include <string>

namespace px {

enum class InputPreset {
    Modern,
    Legacy,
    Custom
};

struct InputProfile {
    InputPreset preset{InputPreset::Modern};
    std::map<Action, std::string> bindings;
};

class InputProfiles {
public:
    static InputProfile modern();
    static InputProfile legacy();
};

class InputState {
public:
    void beginFrame();
    void set(Action action, bool held);
    bool down(Action action) const;
    bool pressed(Action action) const;
    bool released(Action action) const;

private:
    std::set<Action> held_;
    std::set<Action> pressed_;
    std::set<Action> released_;
};

} // namespace px
