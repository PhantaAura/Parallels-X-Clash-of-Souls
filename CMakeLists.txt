cmake_minimum_required(VERSION 3.16)
project(ParallelsXClashOfSouls VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_library(px_core
    src/core/game.cpp
    src/core/input.cpp
    src/core/save.cpp
    src/core/world.cpp
    src/core/field_movement.cpp
    src/core/combat.cpp
    src/core/ability_hotbar.cpp
    src/core/story_progression.cpp
    src/core/story_unlocks.cpp
    src/core/menu_state.cpp
    src/core/quest.cpp
    src/core/runtime.cpp
    src/content/chapter_registry.cpp
    src/content/map_registry.cpp
    src/content/cutscene_registry.cpp
    src/content/dialogue_registry.cpp
    src/content/exploration_registry.cpp
    src/content/training_registry.cpp
    src/content/world_presentation_registry.cpp
    src/content/character_presentation_registry.cpp
    src/content/character_model_asset.cpp
    src/content/character_face.cpp
    src/content/skeletal_animation.cpp
    src/content/adventure_registry.cpp
    src/content/menu_registry.cpp
    src/content/story_route_registry.cpp
    src/content/story_recap_registry.cpp
    src/content/combat_manual_registry.cpp
    src/content/faction_registry.cpp
    src/content/ui_presentation_registry.cpp
)
target_include_directories(px_core PUBLIC src)

add_executable(px_headless_demo src/platform/desktop/headless_main.cpp)
target_link_libraries(px_headless_demo PRIVATE px_core)

add_executable(px_runtime_smoke src/platform/desktop/runtime_smoke.cpp)
target_link_libraries(px_runtime_smoke PRIVATE px_core)

add_executable(px_content_exporter src/tools/content_export.cpp)
target_link_libraries(px_content_exporter PRIVATE px_core)

add_executable(px_character_weight_review src/tools/character_weight_review.cpp)
target_link_libraries(px_character_weight_review PRIVATE px_core)

add_executable(px_character_face_review src/tools/character_face_review.cpp)
target_link_libraries(px_character_face_review PRIVATE px_core)

add_executable(px_core_tests tests/core_tests.cpp)
target_link_libraries(px_core_tests PRIVATE px_core)
add_test(NAME px_core_tests COMMAND px_core_tests)

add_executable(px_menu_tests tests/menu_tests.cpp)
target_link_libraries(px_menu_tests PRIVATE px_core)
add_test(NAME px_menu_tests COMMAND px_menu_tests)

add_executable(px_model_asset_tests tests/model_asset_tests.cpp)
target_link_libraries(px_model_asset_tests PRIVATE px_core)
add_test(NAME px_model_asset_tests COMMAND px_model_asset_tests
    "${CMAKE_SOURCE_DIR}/assets/characters/rrvvfo/rrvvfo-dev.pxskel")

add_executable(px_skeletal_animation_tests tests/skeletal_animation_tests.cpp)
target_link_libraries(px_skeletal_animation_tests PRIVATE px_core)
add_test(NAME px_skeletal_animation_tests COMMAND px_skeletal_animation_tests
    "${CMAKE_SOURCE_DIR}/assets/characters/rrvvfo/rrvvfo-dev.pxskel")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME px_source_architecture_tests
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/source_architecture_tests.py)

# macOS is the lead graphical development target. It is a platform shell around px_core,
# not a second gameplay engine. The renderer can be replaced without touching chapter logic.
if(APPLE)
    enable_language(OBJCXX)
    add_executable(ParallelsX MACOSX_BUNDLE src/platform/macos/main.mm)
    set(PX_RRVVFO_DEV_ASSET "${CMAKE_SOURCE_DIR}/assets/characters/rrvvfo/rrvvfo-dev.pxskel")
    set_source_files_properties("${PX_RRVVFO_DEV_ASSET}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/assets/characters/rrvvfo")
    target_sources(ParallelsX PRIVATE "${PX_RRVVFO_DEV_ASSET}")
    target_link_libraries(ParallelsX PRIVATE px_core
        "-framework Cocoa"
        "-framework GameController"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore")
    target_compile_options(ParallelsX PRIVATE -fobjc-arc)
    set_target_properties(ParallelsX PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Parallels X: Clash of Souls"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.phantaaura.parallelsx.clashofsouls"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.4.8"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "3.0.0"
        OUTPUT_NAME "Parallels X - Clash of Souls")
endif()

# Linux is a graphical Codex QA/development target. It links the installed SDL2
# runtime through a tiny stable-ABI declaration because the validation image may
# not include the SDL development headers. All game/menu/story state remains in
# px_core.
if(UNIX AND NOT APPLE)
    find_library(PX_SDL2_LIBRARY NAMES SDL2-2.0 SDL2)
    if(NOT PX_SDL2_LIBRARY)
        find_file(PX_SDL2_LIBRARY NAMES libSDL2-2.0.so.0
            PATHS /lib /usr/lib /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu)
    endif()
    if(PX_SDL2_LIBRARY)
        add_executable(ParallelsX
            src/platform/linux/main.cpp
            src/platform/linux/presentation_renderer.cpp)
        target_link_libraries(ParallelsX PRIVATE px_core ${PX_SDL2_LIBRARY})
        add_executable(px_linux ALIAS ParallelsX)
    else()
        message(WARNING "SDL2 runtime not found; Linux graphical target is unavailable")
    endif()
endif()
