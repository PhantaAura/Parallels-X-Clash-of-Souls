#include "platform/3ds/legacy_ui_3ds.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace px::platform3ds {
namespace {

constexpr float kTopWidth = 400.0f;
constexpr float kBottomWidth = 320.0f;
constexpr float kScreenHeight = 240.0f;
// Anything below this was only a handful of physical pixels on Old 3DS and
// became unreadable once displayed on the actual screen instead of a mockup.
constexpr float kMinimumReadableScale = .36f;
constexpr u32 kInk = 0xFF080808;
constexpr u32 kWhite = 0xFFF8F6EE;
constexpr u32 kPaper = 0xFFF1E8CB;
constexpr u32 kNavy = 0xFF211207;
constexpr u32 kBlue = 0xFFB45B0D;
constexpr u32 kBlueBright = 0xFFE47C14;
constexpr u32 kYellow = 0xFF46D7FF;
constexpr u32 kOrange = 0xFF2A9BFF;
constexpr u32 kRed = 0xFF242EE0;
constexpr u32 kMuted = 0xFFB8B0A5;
constexpr u32 kLightBlue = 0xFFFFD861;
constexpr u32 kGreen = 0xFF65C96A;

u32 alpha(u32 color, u8 value) { return (color & 0x00FFFFFFu) | (static_cast<u32>(value) << 24); }

std::string manualPrompt3ds(const CombatManualEntry& entry) {
    const auto& id = entry.diagramId;
    if (id == "manual_move") return "CIRCLE PAD";
    if (id == "manual_jump") return "B";
    if (id == "manual_dash") return "D-PAD UP";
    if (id == "manual_light" || id == "manual_pursuit_light") return "Y";
    if (id == "manual_heavy" || id == "manual_pursuit_heavy") return "X";
    if (id == "manual_launcher") return "Y + X";
    if (id == "manual_guard" || id == "manual_perfect_block") return "R";
    if (id == "manual_grab") return "A";
    if (id == "manual_pursuit") return "Y+X -> D-PAD UP";
    if (id == "manual_buffering") return "Y / X DURING CHASE";
    if (id == "manual_pursuit_tech") return "D-PAD UP + 15 ENERGY";
    if (id == "manual_charge") return "HOLD D-PAD DOWN";
    if (id == "manual_counter") return "D-PAD LEFT";
    if (id == "manual_breaker") return "D-PAD RIGHT";
    if (id == "manual_fire_blast") return "L + Y";
    if (id == "manual_object_swap") return "L + X";
    if (id == "manual_lens") return "L + B";
    if (id == "manual_3ds") return "PHYSICAL BUTTONS";
    return entry.controllerPrompt;
}

void roundedRect(float x, float y, float width, float height, float radius,
                 float depth, u32 color) {
    if (width <= 0.0f || height <= 0.0f) return;
    const float r = std::clamp(radius, 0.0f, std::min(width, height) * .5f);
    if (r <= .5f) {
        C2D_DrawRectSolid(x, y, depth, width, height, color);
        return;
    }
    C2D_DrawRectSolid(x + r, y, depth, width - r * 2.0f, height, color);
    C2D_DrawRectSolid(x, y + r, depth, width, height - r * 2.0f, color);
    C2D_DrawCircleSolid(x + r, y + r, depth, r, color);
    C2D_DrawCircleSolid(x + width - r, y + r, depth, r, color);
    C2D_DrawCircleSolid(x + r, y + height - r, depth, r, color);
    C2D_DrawCircleSolid(x + width - r, y + height - r, depth, r, color);
}

} // namespace

LegacyUi3ds::~LegacyUi3ds() { shutdown(); }

bool LegacyUi3ds::init(std::string* error) {
    textBuffer_ = C2D_TextBufNew(8192);
    if (!textBuffer_) {
        if (error) *error = "Could not allocate the Legacy UI text buffer";
        return false;
    }
    return true;
}

void LegacyUi3ds::shutdown() {
    if (textBuffer_) C2D_TextBufDelete(textBuffer_);
    textBuffer_ = nullptr;
}

void LegacyUi3ds::beginFrame() {
    if (textBuffer_) C2D_TextBufClear(textBuffer_);
}

void LegacyUi3ds::label(const std::string& value, float x, float y, float scale,
                        u32 color, float wrapWidth, u32 alignment) {
    if (!textBuffer_ || value.empty()) return;
    scale = std::max(scale, kMinimumReadableScale);
    C2D_Text text{};
    if (!C2D_TextParse(&text, textBuffer_, value.c_str())) return;
    C2D_TextOptimize(&text);
    u32 flags = C2D_WithColor | alignment;
    if (wrapWidth > 0.0f) {
        flags |= C2D_WordWrap;
        C2D_DrawText(&text, flags, x, y, .98f, scale, scale, color, wrapWidth);
    } else {
        C2D_DrawText(&text, flags, x, y, .98f, scale, scale, color);
    }
}

void LegacyUi3ds::centered(const std::string& value, float x, float y, float width,
                           float scale, u32 color) {
    label(value, x + width * .5f, y, scale, color, 0.0f, C2D_AlignCenter);
}

void LegacyUi3ds::fitted(const std::string& value, float x, float y, float width,
                         float preferredScale, float minimumScale, u32 color) {
    if (!textBuffer_ || value.empty()) return;
    C2D_Text text{};
    if (!C2D_TextParse(&text, textBuffer_, value.c_str())) return;
    C2D_TextOptimize(&text);
    const float readableMinimum = std::max(minimumScale, kMinimumReadableScale);
    float scale = std::max(preferredScale, readableMinimum);
    if (text.width * scale > width && text.width > 0.0f)
        scale = width / text.width;
    if (scale < readableMinimum) {
        const float ratio = std::clamp(width / std::max(1.0f, text.width * readableMinimum), .08f, 1.0f);
        const std::size_t maximum = std::max<std::size_t>(4, static_cast<std::size_t>(value.size() * ratio));
        const std::string shortened = clipped(value, maximum);
        C2D_Text compact{};
        if (!C2D_TextParse(&compact, textBuffer_, shortened.c_str())) return;
        C2D_TextOptimize(&compact);
        scale = readableMinimum;
        while (compact.width * scale > width && scale > kMinimumReadableScale)
            scale = std::max(kMinimumReadableScale, scale - .01f);
        C2D_DrawText(&compact, C2D_WithColor, x, y, .98f, scale, scale, color);
        return;
    }
    C2D_DrawText(&text, C2D_WithColor, x, y, .98f, scale, scale, color);
}

void LegacyUi3ds::blueField(float width, float height) {
    C2D_DrawRectangle(0, 0, .70f, width, height, kNavy, kBlue, kNavy, kBlueBright);
    for (float x = -height; x < width; x += 35.0f)
        C2D_DrawLine(x, height, alpha(kWhite, 18), x + height, 0, alpha(kWhite, 18), 1.0f, .72f);
    for (float y = 16; y < height; y += 32)
        C2D_DrawLine(0, y, alpha(kWhite, 12), width, y, alpha(kWhite, 12), .7f, .72f);
}

void LegacyUi3ds::hardPanel(float x, float y, float width, float height, u32 fill,
                            u32 border, u32 accent, bool selected) {
    C2D_DrawRectSolid(x + 4, y + 4, .78f, width, height, alpha(kInk, 185));
    C2D_DrawRectSolid(x, y, .80f, width, height, border);
    C2D_DrawRectSolid(x + 2, y + 2, .82f, width - 4, height - 4, fill);
    C2D_DrawRectSolid(x + 2, y + 2, .86f, selected ? 7.0f : 4.0f, height - 4, accent);
    if (selected) {
        C2D_DrawRectSolid(x + 2, y + 2, .87f, width - 4, 3, accent);
        C2D_DrawTriangle(x + width - 16, y + 2, fill,
                         x + width - 2, y + 2, accent,
                         x + width - 2, y + 16, fill, .88f);
    }
}

void LegacyUi3ds::legacyButton(float x, float y, float width, float height,
                               const std::string& value, bool selected, bool enabled) {
    const u32 fill = selected ? kOrange : enabled ? kWhite : alpha(kMuted, 245);
    hardPanel(x, y, width, height, fill, kInk, selected ? kYellow : alpha(kWhite, 110), selected);
    fitted(value, x + 12, y + height * .23f, width - 20, .47f, .32f,
           enabled ? kInk : 0xFF5A5652);
}

void LegacyUi3ds::remakePanel(float x, float y, float width, float height, u32 fill,
                              u32 accent, bool selected) {
    const float radius = std::clamp(height * .18f, 7.0f, 14.0f);
    roundedRect(x + 3, y + 5, width, height, radius, .78f, alpha(kInk, 150));
    roundedRect(x, y, width, height, radius, .80f, selected ? accent : kInk);
    roundedRect(x + 2, y + 2, width - 4, height - 4, std::max(3.0f, radius - 2.0f),
                .83f, fill);
    C2D_DrawRectSolid(x + radius, y + 3, .86f, width - radius * 2.0f, 2,
                      selected ? alpha(kWhite, 180) : alpha(kWhite, 95));
    C2D_DrawCircleSolid(x + 11, y + height - 10, .87f, 3.0f, accent);
}

void LegacyUi3ds::remakeButton(float x, float y, float width, float height,
                               const std::string& value, const std::string& badge,
                               bool selected, bool enabled) {
    const u32 fill = !enabled ? alpha(kMuted, 245) : selected ? kOrange : kPaper;
    const u32 accent = !enabled ? 0xFF77716B : selected ? kYellow : kBlueBright;
    remakePanel(x, y, width, height, fill, accent, selected);

    const float badgeRadius = std::clamp(height * .27f, 9.0f, 15.0f);
    const float badgeX = x + badgeRadius + 8.0f;
    const float badgeY = y + height * .5f;
    C2D_DrawCircleSolid(badgeX + 1, badgeY + 2, .87f, badgeRadius + 1, alpha(kInk, 125));
    C2D_DrawCircleSolid(badgeX, badgeY, .89f, badgeRadius,
                        enabled ? (selected ? kYellow : kBlue) : alpha(kMuted, 255));
    centered(badge.empty() ? ">" : badge,
             badgeX - badgeRadius, badgeY - 8, badgeRadius * 2.0f,
             badge.size() > 2 ? .27f : .38f, enabled ? kInk : 0xFF5A5652);

    fitted(value, badgeX + badgeRadius + 7, y + height * .5f - 8,
           width - badgeRadius * 2.0f - 29, height >= 42 ? .40f : .34f,
           .30f, enabled ? kInk : 0xFF5A5652);
}

void LegacyUi3ds::hintPill(float x, float y, float width, const std::string& value,
                           const std::string& badge) {
    roundedRect(x + 2, y + 3, width, 20, 9, .80f, alpha(kInk, 120));
    roundedRect(x, y, width, 20, 9, .83f, alpha(kNavy, 238));
    if (!badge.empty()) {
        C2D_DrawCircleSolid(x + 11, y + 10, .88f, 7, kYellow);
        centered(badge, x + 4, y + 4, 14, .27f, kInk);
        fitted(value, x + 22, y + 5, width - 29, .26f, .22f, kWhite);
    } else {
        centered(value, x + 5, y + 5, width - 10, .26f, kWhite);
    }
}

void LegacyUi3ds::meter(float x, float y, float width, float ratio, u32 fill,
                        const std::string& caption, bool rightAligned) {
    const float clamped = std::clamp(ratio, 0.0f, 1.0f);
    if (!rightAligned) {
        label(caption, x, y - 1, .27f, kWhite);
        C2D_DrawRectSolid(x, y + 9, .91f, width, 7, kInk);
        C2D_DrawRectSolid(x + 1, y + 10, .93f, (width - 2) * clamped, 5, fill);
    } else {
        label(caption, x + width, y - 1, .27f, kWhite, 0.0f, C2D_AlignRight);
        C2D_DrawRectSolid(x, y + 9, .91f, width, 7, kInk);
        C2D_DrawRectSolid(x + width - 1 - (width - 2) * clamped, y + 10, .93f,
                          (width - 2) * clamped, 5, fill);
    }
}

void LegacyUi3ds::topBrand(const std::string& section) {
    label("PARALLELS", 14, 8, .60f, kWhite);
    label("X", 126, 8, .60f, kYellow);
    label("CLASH OF SOULS", 16, 27, .25f, kLightBlue);
    label(section, 388, 12, .28f, kWhite, 0.0f, C2D_AlignRight);
    C2D_DrawLine(12, 40, alpha(kWhite, 200), 388, 40, alpha(kWhite, 200), 1.0f, .92f);
}

void LegacyUi3ds::drawTopMenu(const MenuSnapshot& snapshot, bool modelAvailable) {
    const bool liveRouteModel = snapshot.screen == MenuScreen::StoryCharacterSelect &&
                                snapshot.selectedRoute.id == "rrvvfo" && modelAvailable;
    if (!liveRouteModel) {
        blueField(kTopWidth, kScreenHeight);
    } else {
        // The live cooked model is already on this target. Preserve it under a
        // dedicated character bay instead of replacing it with an unrelated
        // illustration. Opaque Legacy-blue framing also hides any undefined
        // color outside the mesh pass on older PICA revisions.
        C2D_DrawRectangle(0, 0, .70f, kTopWidth, 70, kNavy, kBlue, kNavy, kBlue);
        C2D_DrawRectangle(0, 70, .70f, 274, kScreenHeight - 70,
                          kNavy, kBlue, kNavy, kBlueBright);
        C2D_DrawRectSolid(0, 222, .71f, kTopWidth, 18, kNavy);
        for (float x = -kScreenHeight; x < kTopWidth; x += 35.0f)
            C2D_DrawLine(x, kScreenHeight, alpha(kWhite, 18), x + kScreenHeight, 0,
                         alpha(kWhite, 18), 1.0f, .72f);
    }
    if (snapshot.screen == MenuScreen::Title) {
        C2D_DrawRectSolid(0, 0, .74f, kTopWidth, 4, kYellow);
        label("PARALLELS", 47, 54, 1.15f, alpha(kInk, 170));
        label("PARALLELS", 43, 50, 1.15f, kWhite);
        label("X", 318, 50, 1.15f, kYellow);
        centered("CLASH OF SOULS", 20, 95, 360, .55f, kLightBlue);
        centered("LEGACY REMAKE", 20, 121, 360, .27f, alpha(kWhite, 210));
        C2D_DrawRectSolid(72, 154, .82f, 256, 3, kYellow);
        centered("CHAPTER 1 - BACK TO NORMAL", 32, 167, 336, .31f, kWhite);
        return;
    }

    if (snapshot.screen == MenuScreen::ModeSelect) {
        topBrand("MODE SELECT");
        centered("MODE SELECT", 30, 48, 340, .83f, kWhite);
        centered("CHOOSE ONE MODE", 30, 75, 340, .26f, kLightBlue);
        const auto& mode = snapshot.selectedMode;
        hardPanel(48, 97, 304, 88, alpha(kBlue, 245), kWhite,
                  mode.implemented ? kYellow : kMuted, true);
        fitted(mode.label, 73, 110, 245, .75f, .50f, kWhite);
        fitted(mode.kicker, 73, 140, 245, .37f, .27f,
               mode.implemented ? kYellow : kMuted);
        label("<", 18, 119, .75f, kYellow);
        label(">", 365, 119, .75f, kYellow);
        centered(mode.implemented ? "SELECT" : "LOCKED", 260, 159, 76, .30f,
                 mode.implemented ? kInk : kWhite);
        if (mode.implemented) C2D_DrawRectSolid(258, 156, .91f, 78, 22, kWhite);
        if (mode.id == MenuModeId::Story) {
            const bool recap = snapshot.storySoFarSelected;
            hardPanel(49, 194, 142, 32, recap ? alpha(kBlue, 245) : kWhite,
                      kInk, recap ? kMuted : kYellow, !recap);
            hardPanel(209, 194, 142, 32, recap ? kWhite : alpha(kBlue, 245),
                      kInk, recap ? kYellow : kMuted, recap);
            centered("STORY MODE", 55, 202, 130, .38f, recap ? kWhite : kInk);
            centered("STORY SO FAR", 215, 202, 130, .38f, recap ? kInk : kWhite);
        }
        return;
    }

    if (snapshot.screen == MenuScreen::StoryCharacterSelect) {
        topBrand("STORY");
        centered("ROUTE SELECT", 30, 47, 340, .82f, kWhite);
        label("STORY MODE", 22, 79, .29f, kYellow);
        hardPanel(18, 96, 250, 128, alpha(kBlue, 245), kWhite, kYellow, true);
        label("NEW STORY", 34, 108, .25f, kRed);
        fitted(snapshot.selectedRoute.characterName.empty() ? "RRVVFO" : snapshot.selectedRoute.characterName,
               34, 125, 205, .78f, .56f, kWhite);
        fitted(snapshot.selectedRoute.title.empty() ? "THE LOST YEAR" : snapshot.selectedRoute.title,
               34, 154, 210, .36f, .28f, kYellow);
        fitted(modelAvailable ? "NEW MODEL + FACE" : "MODEL LOAD REQUIRED",
               34, 180, 212, .25f, .20f, modelAvailable ? kWhite : kRed);
        label("LIVE RIG", 309, 203, .23f, kYellow, 0.0f, C2D_AlignCenter);
        label("<", 6, 135, .62f, alpha(kWhite, 150));
        label(">", 378, 135, .62f, alpha(kWhite, 150));
        return;
    }

    if (snapshot.screen == MenuScreen::StorySoFar) {
        topBrand("STORY SO FAR");
        centered(snapshot.recapSection.title, 22, 49, 356, .68f, kWhite);
        hardPanel(18, 86, 364, 134, kWhite, kInk, kYellow, true);
        label("STORY SO FAR", 34, 100, .25f, kRed);
        fitted(snapshot.recapFrame.caption, 34, 119, 330, .56f, .38f, kInk);
        label(snapshot.recapFrame.body, 34, 150, .31f, kInk, 322);
        return;
    }

    topBrand(snapshot.screen == MenuScreen::UnlockCelebration ? "NEW ROUTE" : "STORY");
    hardPanel(36, 70, 328, 128, kWhite, kInk, kYellow, true);
    centered(snapshot.screen == MenuScreen::UnlockCelebration ? "ROUTE UNLOCKED" : "COMING LATER",
             48, 91, 304, .68f, kInk);
    centered(snapshot.unlockRouteId.empty() ? "THE STORY CONTINUES" : snapshot.unlockRouteId,
             54, 136, 292, .34f, kRed);
}

void LegacyUi3ds::drawBottomMenu(const MenuSnapshot& snapshot, bool modelAvailable) {
    blueField(kBottomWidth, kScreenHeight);
    if (snapshot.screen == MenuScreen::Title) {
        centered("PARALLELS X", 16, 22, 288, .55f, kWhite);
        centered("CLASH OF SOULS", 16, 46, 288, .30f, kLightBlue);
        C2D_DrawCircleSolid(45, 122, .73f, 38, alpha(kYellow, 28));
        C2D_DrawCircleSolid(279, 107, .73f, 26, alpha(kWhite, 22));
        remakeButton(32, 89, 256, 65, "START GAME", "A", true, true);
        hintPill(40, 174, 240, "NATIVE 3DS • SAME GAMEPLAY");
        centered("REDUCED VISUAL DETAIL ONLY", 20, 207, 280, .25f, kYellow);
        return;
    }

    if (snapshot.screen == MenuScreen::ModeSelect) {
        const bool storyMode = snapshot.selectedMode.id == MenuModeId::Story;
        remakePanel(10, 10, 300, storyMode ? 136 : 145, kPaper, kYellow, true);
        label(snapshot.selectedMode.kicker, 26, 23, .28f, kRed);
        fitted(snapshot.selectedMode.label, 26, 43, 268, .68f, .46f, kInk);
        label(snapshot.selectedMode.description, 26, 75, .31f, kInk, 268);
        label("STATUS", 26, 119, .23f, kBlue);
        label(snapshot.selectedMode.status, 87, 117, .31f,
              snapshot.selectedMode.implemented ? kGreen : 0xFF6C6864);
        if (storyMode) {
            hintPill(92, 151, 136, "SWITCH CHOICE", "UP");
            remakeButton(8, 179, 148, 47, "STORY MODE", "A",
                         !snapshot.storySoFarSelected, true);
            remakeButton(164, 179, 148, 47, "STORY SO FAR", "A",
                         snapshot.storySoFarSelected, true);
        } else {
            hintPill(90, 160, 140, "L / R  CHANGE MODE");
            remakeButton(38, 187, 244, 39,
                         snapshot.selectedMode.implemented ? "ENTER MODE" : "COMING LATER",
                         "A", snapshot.selectedMode.implemented, snapshot.selectedMode.implemented);
        }
        return;
    }

    if (snapshot.screen == MenuScreen::StoryCharacterSelect) {
        remakePanel(10, 10, 300, 151, kPaper, kYellow, true);
        label("RRVVFO STORY", 26, 24, .27f, kRed);
        fitted(snapshot.selectedRoute.title.empty() ? "THE LOST YEAR" : snapshot.selectedRoute.title,
               26, 43, 265, .60f, .42f, kInk);
        label(snapshot.selectedRoute.description, 26, 75, .28f, kInk, 268);
        label(modelAvailable ? "MODEL: RRVVFO 15 REPAIRED + FACE" : "MODEL ERROR - START BLOCKED",
              26, 134, .23f, modelAvailable ? kBlue : kRed);
        const auto& actions = snapshot.routeActions;
        hintPill(88, 166, 144, "L / R  CHANGE ROUTE");
        if (actions.empty()) remakeButton(42, 190, 236, 37, "STORY COMING LATER", "A", false, false);
        else {
            const std::size_t index = std::min(snapshot.routeActionIndex, actions.size() - 1);
            remakeButton(34, 190, 252, 37, actions[index], "A", true, true);
        }
        return;
    }

    if (snapshot.screen == MenuScreen::StorySoFar) {
        remakePanel(10, 10, 300, 168, kPaper, kYellow, true);
        label("SECTION", 26, 27, .23f, kRed);
        fitted(snapshot.recapSection.title, 26, 45, 266, .51f, .36f, kInk);
        label(snapshot.recapFrame.body, 26, 75, .28f, kInk, 266);
        char progress[64];
        std::snprintf(progress, sizeof(progress), "%lu / %lu",
                      static_cast<unsigned long>(snapshot.recapFrameIndex + 1),
                      static_cast<unsigned long>(snapshot.recapSection.frames.size()));
        label(progress, 26, 148, .27f, kBlue);
        remakeButton(18, 190, 137, 37, "PREV / NEXT", "LR", false);
        remakeButton(165, 190, 137, 37, "CONTINUE", "A", true);
        return;
    }

    remakePanel(16, 28, 288, 144, kWhite, kYellow, true);
    centered(snapshot.screen == MenuScreen::UnlockCelebration ? "A NEW PATH OPENS" : "NOT RELEASED YET",
             28, 55, 264, .53f, kInk);
    label("No unfinished content is exposed in the playable build.", 37, 97, .29f, kInk, 246);
    remakeButton(58, 188, 204, 39, "BACK", "B", true);
}

void LegacyUi3ds::drawTopGameplay(const RuntimeView& view) {
    if (view.trainingManualVisible) {
        blueField(kTopWidth, kScreenHeight);
        hardPanel(8, 8, 384, 224, kPaper, kInk, kYellow, true);
        label("THE SAGE'S COMBAT MANUAL", 22, 18, .40f, kRed);
        fitted(view.trainingManualPage.title, 22, 42, 352, .58f, .40f, kInk);
        label(view.trainingManualPage.summary, 22, 70, .36f, 0xFF4A4642, 352);

        for (std::size_t index = 0; index < view.trainingManualPage.entries.size() && index < 6; ++index) {
            const float x = index % 2 == 0 ? 18.0f : 205.0f;
            const float y = 111.0f + static_cast<float>(index / 2) * 39.0f;
            hardPanel(x, y, 177, 34, kWhite, kInk, kYellow, false);
            fitted(view.trainingManualPage.entries[index].label, x + 10, y + 5, 157, .38f, .36f, kInk);
            fitted(manualPrompt3ds(view.trainingManualPage.entries[index]), x + 10, y + 19,
                   157, .36f, .36f, kRed);
        }
        return;
    }

    if (view.mode == GameMode::ArenaCombat && !view.dialogueVisible && !view.pauseVisible &&
        !view.trainingManualVisible) {
        meter(8, 5, 146, view.player.hp / std::max(1.0f, view.player.maxHp), kRed, "RRVVFO");
        meter(246, 5, 146, view.opponent.hp / std::max(1.0f, view.opponent.maxHp), kWhite, "THE SAGE", true);
        if (view.showEnergy) meter(8, 23, 102, view.player.energy / 100.0f, kLightBlue, "ENERGY");
        if (view.showGuard) meter(290, 23, 102, view.player.guard / 100.0f, kYellow, "GUARD", true);
    } else if (!view.dialogueVisible && !view.pauseVisible && !view.trainingManualVisible &&
               !view.choiceVisible && !view.qteVisible) {
        hardPanel(7, 7, 260, 68, alpha(kNavy, 238), kWhite, kYellow, true);
        label("CHAPTER 1 - CURRENT OBJECTIVE", 20, 14, .36f, kYellow);
        fitted(view.objective.empty() ? "EXPLORE" : view.objective, 20, 33, 234, .42f, .36f, kWhite);
        if (!view.objectiveDetail.empty()) fitted(view.objectiveDetail, 20, 53, 234, .36f, .36f, kMuted);
    }

    if (!view.nearbyInteractionLabel.empty() && !view.dialogueVisible && !view.pauseVisible) {
        hardPanel(310, 194, 82, 37, alpha(kNavy, 238), kWhite, kYellow, true);
        label("A", 319, 202, .41f, kYellow);
        fitted(view.nearbyInteractionLabel, 343, 204, 42, .22f, .17f, kWhite);
    }
    if (view.lensActive) {
        C2D_DrawRectSolid(0, 0, .96f, kTopWidth, 3, kYellow);
        C2D_DrawRectSolid(0, kScreenHeight - 3, .96f, kTopWidth, 3, kYellow);
    }
    if (view.impactFlash > 0.0f && !view.reducedFlashes) {
        const u8 flashAlpha = static_cast<u8>(std::clamp(view.impactFlash * 2.7f, 0.0f, .38f) * 255.0f);
        C2D_DrawRectSolid(0, 0, .97f, kTopWidth, kScreenHeight, alpha(kPaper, flashAlpha));
    }
}

const char* LegacyUi3ds::faceName(RrvvfoFaceExpression expression) {
    return rrvvfoFaceExpressionName(expression);
}

void LegacyUi3ds::dialogue(const RuntimeView& view, RrvvfoFaceExpression faceExpression,
                           bool modelAvailable) {
    C2D_DrawRectSolid(0, 0, .76f, kBottomWidth, kScreenHeight, alpha(kNavy, 172));
    const bool rrvvfo = view.dialogueSpeaker == "RRVVFO" || view.dialogueFocusActorId == "rrvvfo";
    const u32 speakerAccent = rrvvfo ? kOrange : kYellow;
    remakePanel(8, 52, 304, 163, kWhite, speakerAccent, true);
    roundedRect(19, 37, 153, 35, 12, .90f, kInk);
    roundedRect(21, 39, 149, 31, 10, .93f, rrvvfo ? kOrange : kPaper);
    C2D_DrawCircleSolid(37, 54, .95f, 10, speakerAccent);
    centered(rrvvfo ? "R" : "!", 27, 48, 20, .34f, kInk);
    fitted(view.dialogueSpeaker, 52, 47, 82, .36f, .27f, kInk);
    fitted(view.dialogueExpression.empty() ? faceName(faceExpression) : view.dialogueExpression,
           135, 49, 27, .21f, .16f, kInk);
    label(view.dialogueText, 25, 84, view.largerText ? .36f : .32f, kInk, 273);
    C2D_DrawLine(24, 171, alpha(kBlue, 55), 296, 171, alpha(kBlue, 55), 1.0f, .94f);
    roundedRect(220, 179, 76, 25, 11, .94f, kPaper);
    C2D_DrawCircleSolid(234, 191, .96f, 9, kYellow);
    centered("A", 225, 185, 18, .29f, kInk);
    fitted("CONTINUE", 247, 185, 43, .22f, .20f, kInk);
    char count[32];
    std::snprintf(count, sizeof(count), "%lu/%lu",
                  static_cast<unsigned long>(view.dialogueIndex + 1),
                  static_cast<unsigned long>(view.dialogueCount));
    label(count, 289, 18, .23f, kWhite, 0.0f, C2D_AlignRight);
    label(modelAvailable ? "FACE RIG ACTIVE" : "MODEL ERROR", 18, 18, .22f,
          modelAvailable ? speakerAccent : kRed);
}

void LegacyUi3ds::pause(const RuntimeView& view) {
    blueField(kBottomWidth, kScreenHeight);
    remakePanel(8, 8, 304, 222, kPaper, kLightBlue, true);
    label("PAUSED", 24, 18, .61f, kInk);
    fitted(view.pausePageTitle, 24, 45, 272, .36f, .28f, kRed);
    float y = 67;
    for (const auto& section : view.pauseSections) {
        if (y > 94) break;
        fitted(section, 24, y, 272, .25f, .19f, 0xFF4A4642);
        y += 15;
    }
    const std::size_t optionCount = std::min<std::size_t>(view.pauseOptions.size(), 4);
    const std::size_t startIndex = view.pauseOptions.size() <= 4 ? 0 :
        std::min<std::size_t>(view.pauseSelection > 1 ? view.pauseSelection - 1 : 0, view.pauseOptions.size() - 4);
    const float firstY = optionCount <= 3 ? 108.0f : 101.0f;
    const float buttonHeight = optionCount <= 3 ? 31.0f : 27.0f;
    const float buttonStep = optionCount <= 3 ? 34.0f : 29.0f;
    for (std::size_t row = 0; row < optionCount; ++row) {
        const std::size_t index = startIndex + row;
        remakeButton(20, firstY + static_cast<float>(row) * buttonStep, 280, buttonHeight,
                     clipped(view.pauseOptions[index], 32), index == view.pauseSelection ? "A" : "",
                     index == view.pauseSelection, true);
    }
    if (view.pauseOptions.size() > 4) {
        char page[48]; std::snprintf(page,sizeof(page),"%lu / %lu  •  SCROLL",
            static_cast<unsigned long>(view.pauseSelection + 1), static_cast<unsigned long>(view.pauseOptions.size()));
        fitted(page, 210, 85, 84, .22f, .18f, kBlue);
    }
    if (!view.saveStatus.empty()) fitted(view.saveStatus, 24, 211, 272, .20f, .17f, kBlue);
}

void LegacyUi3ds::manual(const RuntimeView& view) {
    blueField(kBottomWidth, kScreenHeight);
    remakePanel(8, 8, 304, 224, kPaper, kYellow, true);
    label("CHOOSE TRAINING", 24, 19, .54f, kRed);
    label("THE MANUAL IS ON THE TOP SCREEN", 24, 44, .36f, 0xFF4A4642);

    const std::size_t optionCount = std::min<std::size_t>(view.trainingManualOptions.size(), 4);
    const float firstY = optionCount <= 2 ? 78.0f : optionCount == 3 ? 63.0f : 58.0f;
    const float buttonHeight = optionCount <= 2 ? 45.0f : optionCount == 3 ? 39.0f : 31.0f;
    const float buttonStep = optionCount <= 2 ? 54.0f : optionCount == 3 ? 45.0f : 35.0f;
    for (std::size_t index = 0; index < optionCount; ++index) {
        remakeButton(20, firstY + static_cast<float>(index) * buttonStep, 280, buttonHeight,
                     clipped(view.trainingManualOptions[index], 36),
                     index == view.trainingManualSelection ? "A" : "",
                     index == view.trainingManualSelection, true);
    }

    char page[40];
    std::snprintf(page, sizeof(page), "L / R MANUAL PAGE   %lu / %lu",
                  static_cast<unsigned long>(view.trainingManualPageIndex + 1),
                  static_cast<unsigned long>(view.trainingManualPageCount));
    centered(page, 18, 190, 284, .36f, kRed);
    centered("CIRCLE PAD UP/DOWN  •  A START", 18, 210, 284, .36f, kInk);
}

void LegacyUi3ds::choice(const RuntimeView& view) {
    blueField(kBottomWidth, kScreenHeight);
    remakePanel(12, 16, 296, 208, kWhite, kYellow, true);
    label("CHOICE", 28, 31, .25f, kRed);
    fitted(view.choiceTitle, 28, 50, 262, .51f, .35f, kInk);
    float y = 88;
    for (std::size_t index = 0; index < view.choiceOptions.size() && index < 4; ++index) {
        remakeButton(26, y, 268, 31, clipped(view.choiceOptions[index], 34),
                     index == view.choiceIndex ? "A" : "", index == view.choiceIndex, true);
        y += 35;
    }
    centered("CIRCLE PAD - SELECT     A - CONFIRM", 28, 204, 264, .22f, kBlue);
}

void LegacyUi3ds::qte(const RuntimeView& view) {
    blueField(kBottomWidth, kScreenHeight);
    remakePanel(12, 21, 296, 194, kWhite, kOrange, true);
    label("ACTION SEQUENCE", 28, 36, .25f, kRed);
    fitted(view.qteTitle, 28, 56, 262, .52f, .35f, kInk);
    float x = 31;
    for (std::size_t index = 0; index < view.qteSequence.size() && index < 6; ++index) {
        const bool done = index < view.qteIndex;
        const bool active = index == view.qteIndex;
        C2D_DrawCircleSolid(x + 18, 122, .91f, 19, kInk);
        C2D_DrawCircleSolid(x + 18, 120, .93f, 16, active ? kYellow : done ? kGreen : kPaper);
        const char* action = view.qteSequence[index] == Action::MoveLeft ? "<" :
                             view.qteSequence[index] == Action::MoveRight ? ">" : "B";
        centered(action, x, 113, 36, .45f, kInk);
        x += 43;
    }
    char seconds[64];
    std::snprintf(seconds, sizeof(seconds), "%.1f SECONDS    ATTEMPT %d",
                  view.qteSecondsRemaining, view.qteAttempt);
    centered(seconds, 28, 159, 264, .32f, kRed);
}

void LegacyUi3ds::utility(const RuntimeView& view) {
    blueField(kBottomWidth, kScreenHeight);
    remakePanel(10, 8, 300, 69, alpha(kNavy, 245), kYellow, true);
    C2D_DrawCircleSolid(31, 29, .89f, 11, kYellow);
    centered("1", 20, 22, 22, .34f, kInk);
    label("CHAPTER", 49, 18, .23f, kYellow);
    fitted(view.currentArea, 49, 35, 244, .43f, .31f, kWhite);
    fitted(view.objective, 25, 57, 268, .23f, .18f, kMuted);

    if (view.hotbarVisible && !view.hotbar.empty()) {
        const float gap = 4;
        const float width = (300.0f - gap * static_cast<float>(view.hotbar.size() - 1)) /
                            static_cast<float>(view.hotbar.size());
        float x = 10;
        for (const auto& slot : view.hotbar) {
            const bool ready = slot.state == AbilityState::Ready;
            remakePanel(x, 87, width, 78, ready ? kPaper : alpha(kMuted, 250),
                        ready ? kYellow : kMuted, ready);
            const char* combo = slot.id == "fireBlast" ? "L+Y" : slot.id == "objectSwap" ? "L+X" : slot.id == "lensOfTruth" ? "L+B" : "L+?";
            roundedRect(x + width * .5f - 22, 94, 44, 22, 8, .90f, ready ? kYellow : alpha(kMuted,255));
            centered(combo, x + width * .5f - 22, 99, 44, .31f, kInk);
            fitted(slot.label, x + 7, 122, width - 14, .24f, .18f, kInk);
            centered(ready ? "READY" : "LOCK", x + 5, 146, width - 10, .19f,
                     ready ? kBlue : 0xFF77716B);
            x += width + gap;
        }
    } else {
        remakePanel(10, 87, 300, 78, kPaper, kYellow, false);
        label("FIELD CONTROLS", 25, 98, .28f, kRed);
        hintPill(23, 120, 88, "MOVE", "PAD");
        hintPill(116, 120, 87, "JUMP", "B");
        hintPill(208, 120, 88, "DASH", "UP");
    }

    if (!view.combatFeedback.empty() || !view.gameplayNotice.empty()) {
        const std::string& notice = !view.combatFeedback.empty() ? view.combatFeedback : view.gameplayNotice;
        remakePanel(14, 174, 292, 35, kWhite, kOrange, true);
        C2D_DrawCircleSolid(34, 191, .91f, 9, kOrange);
        centered("!", 25, 184, 18, .30f, kInk);
        fitted(notice, 50, 184, 242, .25f, .20f, kInk);
    } else {
        hintPill(43, 181, 234, "A INTERACT   •   START PAUSE");
    }
    hintPill(10, 216, 146, "SAVE", "SEL");
    hintPill(164, 216, 146, "MENU", "STA");
}

void LegacyUi3ds::drawBottomGameplay(const RuntimeView& view,
                                     RrvvfoFaceExpression faceExpression,
                                     bool modelAvailable) {
    if (view.pauseVisible) pause(view);
    else if (view.trainingManualVisible) manual(view);
    else if (view.choiceVisible) choice(view);
    else if (view.qteVisible) qte(view);
    else if (view.dialogueVisible) dialogue(view, faceExpression, modelAvailable);
    else utility(view);
}

void LegacyUi3ds::drawFatal(const std::string& title, const std::string& detail,
                            bool bottomScreen) {
    blueField(bottomScreen ? kBottomWidth : kTopWidth, kScreenHeight);
    const float width = bottomScreen ? 292.0f : 360.0f;
    const float x = bottomScreen ? 14.0f : 20.0f;
    hardPanel(x, 35, width, 166, kWhite, kInk, kRed, true);
    label("BUILD BLOCKED", x + 18, 51, .27f, kRed);
    fitted(title, x + 18, 73, width - 36, .56f, .38f, kInk);
    label(detail, x + 18, 110, .29f, kInk, width - 36);
    label("The playable Rrvvfo fallback is disabled.", x + 18, 173, .23f, kRed);
}

std::string LegacyUi3ds::clipped(const std::string& value, std::size_t maximum) {
    if (value.size() <= maximum) return value;
    if (maximum < 4) return value.substr(0, maximum);
    return value.substr(0, maximum - 3) + "...";
}

} // namespace px::platform3ds
