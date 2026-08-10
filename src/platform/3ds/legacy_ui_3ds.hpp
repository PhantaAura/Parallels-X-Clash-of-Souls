#pragma once

#include <citro2d.h>

#include "content/character_face.hpp"
#include "core/menu_state.hpp"
#include "core/runtime.hpp"

#include <string>

namespace px::platform3ds {

// Native Old 3DS adaptation of Legacy 2.9A's interface language. The top screen
// keeps Legacy's bold blue field and hard-edged presentation. The touch screen
// uses larger layered command tiles and compact utility groups inspired by the
// readability of Nintendo's 3DS-era RPG menus without copying their artwork.
class LegacyUi3ds {
public:
    LegacyUi3ds() = default;
    ~LegacyUi3ds();

    LegacyUi3ds(const LegacyUi3ds&) = delete;
    LegacyUi3ds& operator=(const LegacyUi3ds&) = delete;

    bool init(std::string* error = nullptr);
    void shutdown();
    void beginFrame();

    void drawTopMenu(const MenuSnapshot& snapshot, bool modelAvailable);
    void drawBottomMenu(const MenuSnapshot& snapshot, bool modelAvailable);
    void drawTopGameplay(const RuntimeView& view);
    void drawBottomGameplay(const RuntimeView& view,
                            RrvvfoFaceExpression faceExpression,
                            bool modelAvailable);
    void drawFatal(const std::string& title, const std::string& detail, bool bottomScreen);

private:
    void blueField(float width, float height);
    void label(const std::string& value, float x, float y, float scale,
               u32 color, float wrapWidth = 0.0f, u32 alignment = C2D_AlignLeft);
    void centered(const std::string& value, float x, float y, float width,
                  float scale, u32 color);
    void fitted(const std::string& value, float x, float y, float width,
                float preferredScale, float minimumScale, u32 color);
    void hardPanel(float x, float y, float width, float height, u32 fill,
                   u32 border, u32 accent, bool selected = false);
    void legacyButton(float x, float y, float width, float height,
                      const std::string& labelValue, bool selected, bool enabled = true);
    void remakePanel(float x, float y, float width, float height, u32 fill,
                     u32 accent, bool selected = false);
    void remakeButton(float x, float y, float width, float height,
                      const std::string& labelValue, const std::string& badge,
                      bool selected, bool enabled = true);
    void hintPill(float x, float y, float width, const std::string& labelValue,
                  const std::string& badge = "");
    void meter(float x, float y, float width, float ratio, u32 fill,
               const std::string& caption, bool rightAligned = false);
    void topBrand(const std::string& section);
    void dialogue(const RuntimeView& view, RrvvfoFaceExpression faceExpression,
                  bool modelAvailable);
    void pause(const RuntimeView& view);
    void manual(const RuntimeView& view);
    void choice(const RuntimeView& view);
    void qte(const RuntimeView& view);
    void utility(const RuntimeView& view);
    static std::string clipped(const std::string& value, std::size_t maximum);
    static const char* faceName(RrvvfoFaceExpression expression);

    C2D_TextBuf textBuffer_{nullptr};
};

} // namespace px::platform3ds
