#pragma once
#include <string>
#include <vector>

namespace px {

struct CombatManualEntry {
    std::string label;
    std::string keyboardPrompt;
    std::string controllerPrompt;
    std::string description;
    std::string diagramId;
};

struct CombatManualPage {
    std::string id;
    std::string category;
    std::string title;
    std::string kicker;
    std::string summary;
    std::vector<CombatManualEntry> entries;
};

class CombatManualRegistry {
public:
    CombatManualRegistry();
    const std::vector<CombatManualPage>& pages() const { return pages_; }
    const CombatManualPage& get(const std::string& id) const;
    std::vector<std::string> categories() const;

private:
    std::vector<CombatManualPage> pages_;
};

} // namespace px
