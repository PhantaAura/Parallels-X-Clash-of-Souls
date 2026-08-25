#pragma once
#include "content/character_face.hpp"
#include "content/character_model_asset.hpp"
#include "content/skeletal_animation.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/combat_manual_registry.hpp"
#include "content/menu_registry.hpp"
#include "content/ui_presentation_registry.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/menu_state.hpp"
#include "core/runtime.hpp"
#include "platform/desktop/sdl_compat.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace px::desktop {

class PresentationRenderer {
public:
    PresentationRenderer(SDL_Renderer* renderer,
                         const UiPresentationRegistry& ui,
                         const MenuRegistry& menus,
                         const CombatManualRegistry& manual,
                         const WorldPresentationRegistry& worlds,
                         const CharacterPresentationRegistry& characters,
                         const CharacterModelRepository& characterModels,
                         const SkeletalAnimationPlayer& playerAnimation);

    void renderMenu(const MenuSnapshot& menu);
    void renderManual(std::size_t pageIndex, const RuntimeView* training = nullptr);
    void renderRuntime(const RuntimeView& view);
    void renderModelReview(const RuntimeView& view);
    void renderIdlePoseReview(int legacyFrame, float sampleTime);
    void renderAnimationReview(const std::string& label, float sampleTime, float duration);
    void renderLegacyComparison(const RuntimeView& view);
    bool writePpm(const std::string& path) const;

private:
    void color(UiColor value);
    void fill(UiRect rect, UiColor value);
    void outline(UiRect rect, UiColor value, int thickness = 2);
    void line(int x1, int y1, int x2, int y2, UiColor value, int thickness = 1);
    void circle(int cx, int cy, int radius, UiColor value);
    void diagonalBand(UiRect rect, int slant, UiColor value);
    void text(const std::string& value, int x, int y, int scale, UiColor ink, bool shadow = false);
    int wrapped(const std::string& value, UiRect rect, int scale, UiColor ink, int lineGap = 3);
    int textWidth(const std::string& value, int scale) const;
    void centered(const std::string& value, int centerX, int y, int scale, UiColor ink, bool shadow = false);
    void glyph(char c, int x, int y, int scale, UiColor value);
    void fighter(int cx, int feetY, int scale, UiColor body, UiColor edge, bool mentor = false);
    bool characterModel(const std::string& characterId, int cx, int feetY, int pixelHeight,
                        float yawDegrees, bool focused,
                        RrvvfoFaceExpression faceExpression = RrvvfoFaceExpression::Neutral);
    void xMotif(int cx, int cy, int size, UiColor value, int thickness);
    void bar(UiRect rect, float ratio, UiColor fillColor, const std::string& label);
    void drawBackdrop(UiColor accent);
    void drawTitle();
    void drawMode(const MenuSnapshot& menu);
    void drawSubmenu(const MenuSnapshot& menu);
    void drawStoryCharacter(const MenuSnapshot& menu);
    void drawRecap(const MenuSnapshot& menu);
    void drawComingLater(const MenuSnapshot& menu);
    void drawUnlock(const MenuSnapshot& menu);
    void drawWorld(const RuntimeView& view);
    void drawHud(const RuntimeView& view);
    void drawDialogue(const RuntimeView& view);
    void drawActionOverlay(const RuntimeView& view);
    void drawPause(const RuntimeView& view);

    SDL_Renderer* renderer_{nullptr};
    const UiPresentationRegistry& ui_;
    [[maybe_unused]] const MenuRegistry& menus_;
    const CombatManualRegistry& manual_;
    const WorldPresentationRegistry& worlds_;
    const CharacterPresentationRegistry& characters_;
    const CharacterModelRepository& characterModels_;
    const SkeletalAnimationPlayer& playerAnimation_;
};

} // namespace px::desktop
