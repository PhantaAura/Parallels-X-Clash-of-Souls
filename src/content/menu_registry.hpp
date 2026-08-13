#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace px {

enum class MenuModeId : std::uint8_t {
    Continue,
    Story,
    ArenaBattle,
    OnlinePlay,
    VsCpu,
    TwoPlayer,
    Training,
    Extras,
    Options,
    Credits,
    Arcade
};

struct MenuModeDefinition {
    MenuModeId id{MenuModeId::Story};
    std::string stableId;
    std::string label;
    std::string kicker;
    std::string description;
    std::string status;
    std::string artId;
    std::string accentId;
    bool implemented{false};
};

class MenuRegistry {
public:
    MenuRegistry();
    const std::vector<MenuModeDefinition>& modes() const { return modes_; }
    const MenuModeDefinition& get(MenuModeId id) const;
    const MenuModeDefinition& get(const std::string& stableId) const;

private:
    std::vector<MenuModeDefinition> modes_;
};

} // namespace px
