#!/usr/bin/env python3
from __future__ import annotations
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else pathlib.Path.cwd().resolve()
failures = []
passes = []

def text(rel):
    p = root / rel
    if not p.exists():
        failures.append(f"missing {rel}")
        return ""
    return p.read_text(encoding="utf-8")

def require(rel, needle, label):
    value = text(rel)
    if needle in value: passes.append(label)
    else: failures.append(label)

# Architecture / milestone locks.
require("src/content/cutscene_registry.hpp", "enum class CutsceneActionKind", "U7A shared cinematic action language")
require("src/core/runtime.cpp", "applyCutsceneActions", "U7A shared RuntimeSession staging")
require("src/core/runtime.cpp", "objectSwapCooldownTime_ = 4.5f", "U7B long free Object Swap cooldown")
require("src/core/runtime.cpp", "kFreeSwapRange = 225.0f", "U7B short free Object Swap range")
require("src/core/runtime.cpp", "lensBlindnessAmount", "U7B Lens blindness presentation")
require("src/content/chapter_registry.cpp", 'chapters_.emplace("rrvvfo_ch2"', "U8 continuous Chapter 2 content")
require("src/content/map_registry.cpp", 'tournament.id = "tournament_grounds"', "U8 shared tournament hub map")
require("src/content/arena_encounter_registry.cpp", '"ch2_vs_hamual"', "U9 Hamual match")
require("src/content/arena_encounter_registry.cpp", '"ch2_vs_daniel"', "U9 Daniel match")
require("src/content/arena_encounter_registry.cpp", '"ch2_vs_wade"', "U9 Wade match")
require("src/content/arena_encounter_registry.cpp", '"ch2_vs_plouke"', "U9 Plouke final")
require("src/core/save.hpp", "kSchemaVersion = 6", "U9 Tournament Card save schema")
require("src/platform/3ds/legacy_ui_3ds.cpp", "TOURNAMENT CARD", "U10 3DS Tournament Card")
require("src/platform/macos/main.mm", "TOURNAMENT CARD", "U10 macOS Tournament Card")
require("src/core/runtime.cpp", 'game_.story().chapterId == "rrvvfo_ch1"', "U10 chapter-safe save scoping")
require("docs/U7_U10_GOLDEN_GATES.md", "U7 Golden Regression Gate", "milestone acceptance gates")

# One-engine rule.
cmake = text("CMakeLists.txt")
if "src/content/arena_encounter_registry.cpp" in cmake and cmake.count("add_library(px_core") == 1:
    passes.append("one shared px_core architecture")
else:
    failures.append("one shared px_core architecture")
for forbidden in ["chapter2_engine", "tournament_engine", "TournamentEngine", "Chapter2Engine"]:
    if forbidden in "\n".join([text("src/core/runtime.cpp"), text("src/content/chapter_registry.cpp"), cmake]):
        failures.append(f"forbidden separate engine token: {forbidden}")

# Object Swap can never regress into a dash mechanic/name in gameplay/content.
gameplay_sources = "\n".join([
    text("src/core/runtime.cpp"), text("src/core/runtime.hpp"),
    text("src/content/exploration_registry.cpp"), text("src/content/chapter_registry.cpp")
]).lower()
if "object_swap_dash" in gameplay_sources or "object swap dash" in gameplay_sources:
    failures.append("Object Swap incorrectly named/implemented as a dash")
else:
    passes.append("Object Swap literal-swap naming lock")

# Chapter 1 never exposes Shots of Agony.
hotbar = text("src/core/ability_hotbar.cpp")
start = hotbar.find("AbilityHotbarCatalog::rrvvfoChapter1")
end = hotbar.find("AbilityHotbarCatalog::rrvvfoWithShotsOfAgony")
ch1_hotbar = hotbar[start:end] if start >= 0 and end > start else hotbar
if "shotsOfAgony" in ch1_hotbar or "SHOTS OF AGONY" in ch1_hotbar:
    failures.append("Shots of Agony exposed in Chapter 1 hotbar")
else:
    passes.append("Shots of Agony remains unavailable in Chapter 1")

# Color language.
require("src/platform/macos/main.mm", "{1.0f,.78f,.12f,.56f}", "Object Swap gold visual language")
require("src/platform/macos/main.mm", "{.63f,.22f,.92f,.68f}", "Lens purple visual language")

# Exact legacy idle protection: authored source must still state 0.75s idle.
anim = text("assets/characters/rrvvfo/rrvvfo-core.animation.json")
if re.search(r'"name"\s*:\s*"idle"', anim) and (re.search(r'"duration"\s*:\s*0\.75(?:0+)?', anim) or '"durationSeconds": 0.75' in anim):
    passes.append("Legacy idle remains 0.75 seconds")
else:
    failures.append("Legacy idle 0.75-second lock not found")
require("src/core/runtime.cpp", "Combat retreat is now its own authored clip", "dedicated combat_retreat lock")

# Story order / outcome locks.
chapter = text("src/content/chapter_registry.cpp")
order = ["ch2_vs_hamual","ch2_vs_daniel","ch2_bark_pouki","ch2_vs_wade","ch2_vs_plouke"]
positions = [chapter.find(x) for x in order]
if all(p >= 0 for p in positions) and positions == sorted(positions):
    passes.append("Tournament required match order")
else:
    failures.append("Tournament required match order")
require("src/core/runtime.cpp", "PloukeStoryFinal", "Plouke final story resolution")
if "saboteur" in text("src/content/dialogue_registry.cpp").lower() and "ch2_cracked_ring" in text("src/content/dialogue_registry.cpp"):
    # The word may exist elsewhere; fail only if an explicit reveal/fight token was authored.
    if "ch2_cracked_ring_saboteur" in text("src/content/dialogue_registry.cpp") or "ch2_saboteur_fight" in chapter:
        failures.append("Chapter 2 Cracked Ring reveals/fights a saboteur")
    else:
        passes.append("Cracked Ring stays investigation-only")
else:
    passes.append("Cracked Ring stays investigation-only")

print("PARALLELS X U7-U10 SOURCE VERIFIER")
for item in passes: print("PASS:", item)
if failures:
    print("\nFAILED INVARIANTS:")
    for item in failures: print("FAIL:", item)
    raise SystemExit(1)
print(f"\nPASS: {len(passes)} locked invariants verified")
print("NOTE: source verification is not a substitute for Mac/Linux/real Old 3DS acceptance.")
