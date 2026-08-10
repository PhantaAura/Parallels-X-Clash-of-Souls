#pragma once
#include "core/types.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace px {

class ChapterRegistry {
public:
    ChapterRegistry();
    const ChapterDefinition& get(const std::string& id) const;
    bool has(const std::string& id) const;
    std::vector<std::string> ids() const;

private:
    std::unordered_map<std::string, ChapterDefinition> chapters_;
};

} // namespace px
