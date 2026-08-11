#include "content/world_presentation_registry.hpp"
#include "core/math.hpp"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace px {
namespace {

PresentationColor rgb(float r, float g, float b, float a = 1.0f) {
    return {r / 255.0f, g / 255.0f, b / 255.0f, a};
}

void box(WorldPresentationDefinition& stage, const std::string& id,
         float x, float y, float z, float sx, float sy, float sz,
         PresentationColor color, PresentationDetailTier detail = PresentationDetailTier::Essential,
         float yaw = 0.0f, std::string blocker = {}) {
    stage.primitives.push_back({id, PresentationPrimitiveKind::Box,
        {x, y, z, sx, sy, sz, yaw}, color, detail, std::move(blocker)});
}

void cylinder(WorldPresentationDefinition& stage, const std::string& id,
              float x, float y, float z, float sx, float sy, float sz,
              PresentationColor color, PresentationDetailTier detail = PresentationDetailTier::Essential,
              std::string blocker = {}) {
    stage.primitives.push_back({id, PresentationPrimitiveKind::Cylinder,
        {x, y, z, sx, sy, sz, 0.0f}, color, detail, std::move(blocker)});
}

void cone(WorldPresentationDefinition& stage, const std::string& id,
          float x, float y, float z, float sx, float sy, float sz,
          PresentationColor color, PresentationDetailTier detail = PresentationDetailTier::Full,
          std::string blocker = {}) {
    stage.primitives.push_back({id, PresentationPrimitiveKind::Cone,
        {x, y, z, sx, sy, sz, 0.0f}, color, detail, std::move(blocker)});
}

void tree(WorldPresentationDefinition& stage, const std::string& id, float x, float z,
          float scale, PresentationDetailTier detail = PresentationDetailTier::Full) {
    cylinder(stage, id + "_trunk", x, 46.0f * scale, z, 22.0f * scale, 92.0f * scale, 22.0f * scale,
             rgb(96, 64, 42), detail);
    cone(stage, id + "_lower", x, 118.0f * scale, z, 112.0f * scale, 128.0f * scale, 112.0f * scale,
         rgb(49, 94, 54), detail);
    cone(stage, id + "_upper", x, 174.0f * scale, z, 82.0f * scale, 120.0f * scale, 82.0f * scale,
         rgb(63, 116, 64), detail);
}

void addTrainingField(WorldPresentationDefinition& field) {
    constexpr float originX = -1240.0f;
    // Prototype 2.9A.40.7.1.1 visual lock: these are the authored dimensions,
    // camera-space landmarks and color relationships of Sage Training Field.
    // Keep this scene readable as the Legacy field before adding remaster detail.
    box(field, "field_outer_turf", originX, -42.0f, 0.0f, 1320.0f, 28.0f, 920.0f, rgb(56, 90, 52));
    box(field, "field_ground_base", originX, -26.0f, 0.0f, 1160.0f, 52.0f, 780.0f, rgb(72, 106, 63));
    box(field, "field_grass_surface", originX, 1.0f, 0.0f, 1080.0f, 5.0f, 700.0f, rgb(114, 168, 88));

    // The pale tiling is part of the Legacy field texture, not a perspective
    // debug grid. Its low contrast is preserved deliberately.
    for (float x = originX - 520.0f, index = 0.0f; x <= originX + 520.01f; x += 135.0f, index += 1.0f)
        box(field, "field_tile_x_" + std::to_string(static_cast<int>(index)), x, 4.5f, 0.0f,
            1.5f, 1.0f, 660.0f, rgb(215, 237, 186, .13f), PresentationDetailTier::Full);
    for (float z = -330.0f, index = 0.0f; z <= 330.01f; z += 110.0f, index += 1.0f)
        box(field, "field_tile_z_" + std::to_string(static_cast<int>(index)), originX, 4.5f, z,
            1040.0f, 1.0f, 1.5f, rgb(215, 237, 186, .11f), PresentationDetailTier::Full);

    // A ring plus a faint cross matches the Legacy practice-circle marking.
    // A filled cylinder here made the previous Linux review read as a debug pad.
    constexpr int ringSegments = 40;
    constexpr float ringRadius = 128.0f;
    constexpr float pi = 3.14159265358979323846f;
    for (int i = 0; i < ringSegments; ++i) {
        const float a = static_cast<float>(i) / ringSegments * pi * 2.0f;
        const float b = static_cast<float>(i + 1) / ringSegments * pi * 2.0f;
        const float middle = (a + b) * .5f;
        const float chord = 2.0f * ringRadius * std::sin((b - a) * .5f);
        box(field, "field_center_ring_" + std::to_string(i),
            originX + std::cos(middle) * ringRadius, 5.0f, std::sin(middle) * ringRadius,
            chord + .8f, 1.4f, 3.0f, rgb(231, 245, 189, .48f), PresentationDetailTier::Full,
            middle * 180.0f / pi + 90.0f);
    }
    box(field, "field_center_cross_x", originX, 5.0f, 0.0f, 308.0f, 1.4f, 2.0f,
        rgb(231, 245, 189, .24f), PresentationDetailTier::Full);
    box(field, "field_center_cross_z", originX, 5.0f, 0.0f, 308.0f, 1.4f, 2.0f,
        rgb(231, 245, 189, .24f), PresentationDetailTier::Full, 90.0f);

    box(field, "field_north_bank", originX, 34.0f, -430.0f, 1280.0f, 72.0f, 170.0f, rgb(77, 116, 63));
    box(field, "field_northwest_rise", originX - 470.0f, 72.0f, -360.0f, 190.0f, 150.0f, 150.0f, rgb(63, 107, 60));
    box(field, "field_northeast_rise", originX + 475.0f, 82.0f, -355.0f, 215.0f, 170.0f, 170.0f, rgb(65, 110, 64));
    box(field, "field_southwest_rise", originX - 430.0f, 36.0f, 285.0f, 150.0f, 72.0f, 140.0f, rgb(95, 130, 71));
    box(field, "field_southeast_rise", originX + 430.0f, 40.0f, 280.0f, 170.0f, 80.0f, 150.0f, rgb(100, 135, 75));

    // The field uses the recognizable low-poly block canopies from Legacy;
    // conical forest-road trees belong to the later road scene instead.
    box(field, "field_tree_west_trunk", originX - 315.0f, 52.0f, -285.0f, 24.0f, 104.0f, 24.0f, rgb(94, 62, 39));
    box(field, "field_tree_west_canopy", originX - 315.0f, 118.0f, -285.0f, 118.0f, 90.0f, 118.0f, rgb(53, 111, 59));
    box(field, "field_tree_east_trunk", originX + 330.0f, 56.0f, -270.0f, 26.0f, 112.0f, 26.0f, rgb(94, 62, 39));
    box(field, "field_tree_east_canopy", originX + 330.0f, 126.0f, -270.0f, 126.0f, 96.0f, 126.0f, rgb(57, 119, 66));
    box(field, "field_boundary_west", originX - 505.0f, 30.0f, 20.0f, 30.0f, 60.0f, 80.0f, rgb(130, 111, 91));
    box(field, "field_boundary_east", originX + 505.0f, 34.0f, -20.0f, 34.0f, 68.0f, 92.0f, rgb(125, 105, 87));

    for (int i = 0; i < 5; ++i) {
        const float x = originX - 520.0f + static_cast<float>(i) * 260.0f;
        for (const float z : {-330.0f, 330.0f}) {
            const std::string side = z < 0.0f ? "north" : "south";
            box(field, "field_fence_" + side + "_post_" + std::to_string(i), x, 18.0f, z,
                7.0f, 36.0f, 7.0f, rgb(111, 81, 48), PresentationDetailTier::Full);
            box(field, "field_fence_" + side + "_cap_" + std::to_string(i), x, 39.0f, z,
                12.0f, 8.0f, 12.0f, rgb(216, 182, 118), PresentationDetailTier::Full);
        }
    }
    for (const float x : {originX - 520.0f, originX + 520.0f}) {
        const std::string side = x < originX ? "west" : "east";
        for (const float z : {-70.0f, 190.0f}) {
            box(field, "field_fence_" + side + "_post_" + std::to_string(static_cast<int>(z)), x, 18.0f, z,
                7.0f, 36.0f, 7.0f, rgb(111, 81, 48), PresentationDetailTier::Full);
            box(field, "field_fence_" + side + "_cap_" + std::to_string(static_cast<int>(z)), x, 39.0f, z,
                12.0f, 8.0f, 12.0f, rgb(216, 182, 118), PresentationDetailTier::Full);
        }
    }
    box(field, "field_fence_north_rail", originX, 26.0f, -330.0f, 1040.0f, 4.0f, 4.0f, rgb(141, 106, 66, .62f), PresentationDetailTier::Full);
    box(field, "field_fence_south_rail", originX, 26.0f, 330.0f, 1040.0f, 4.0f, 4.0f, rgb(141, 106, 66, .62f), PresentationDetailTier::Full);
    box(field, "field_fence_west_rail", originX - 520.0f, 26.0f, 0.0f, 4.0f, 4.0f, 660.0f, rgb(141, 106, 66, .62f), PresentationDetailTier::Full);
    box(field, "field_fence_east_rail", originX + 520.0f, 26.0f, 0.0f, 4.0f, 4.0f, 660.0f, rgb(141, 106, 66, .62f), PresentationDetailTier::Full);
}

void addTrainingRoad(WorldPresentationDefinition& road) {
    box(road, "road_outer_ground", 0.0f, -41.0f, 0.0f, 3280.0f, 28.0f, 2140.0f, rgb(52, 86, 48));
    box(road, "road_ground_base", 0.0f, -28.0f, 0.0f, 3100.0f, 56.0f, 1980.0f, rgb(53, 90, 53));
    box(road, "road_grass_surface", 0.0f, 1.0f, 0.0f, 2980.0f, 5.0f, 1860.0f, rgb(87, 131, 74));

    for (int x = -1370, index = 0; x <= 1370; x += 135, ++index) {
        tree(road, "north_tree_" + std::to_string(index), static_cast<float>(x), -785.0f, 0.88f + (index % 3) * 0.08f);
        tree(road, "south_tree_" + std::to_string(index), static_cast<float>(x), 785.0f, 0.92f + ((index + 1) % 3) * 0.07f);
    }
    for (int x = -880, index = 0; x <= 1120; x += 180, ++index) {
        tree(road, "inner_north_tree_" + std::to_string(index), static_cast<float>(x), -540.0f, 0.78f + (index % 2) * 0.09f);
        tree(road, "inner_south_tree_" + std::to_string(index), static_cast<float>(x), 540.0f, 0.80f + ((index + 1) % 2) * 0.08f);
    }

    // Sage sanctuary and its three focus pillars / hanging bell.
    box(road, "sanctuary_outer", -1160.0f, 10.0f, 70.0f, 610.0f, 15.0f, 410.0f, rgb(184, 155, 108));
    box(road, "sanctuary_middle", -1160.0f, 17.0f, 70.0f, 430.0f, 8.0f, 265.0f, rgb(216, 194, 141));
    box(road, "sanctuary_inner", -1160.0f, 25.0f, 70.0f, 315.0f, 5.0f, 190.0f, rgb(239, 224, 168));
    cylinder(road, "sanctuary_post_west", -1380.0f, 72.0f, 70.0f, 24.0f, 144.0f, 24.0f, rgb(94, 68, 48));
    cylinder(road, "sanctuary_post_east", -940.0f, 72.0f, 70.0f, 24.0f, 144.0f, 24.0f, rgb(94, 68, 48));
    box(road, "sanctuary_roof_lower", -1160.0f, 142.0f, -110.0f, 530.0f, 18.0f, 30.0f, rgb(216, 174, 77));
    box(road, "sanctuary_roof_middle", -1160.0f, 168.0f, -110.0f, 440.0f, 22.0f, 44.0f, rgb(181, 61, 56));
    box(road, "sanctuary_roof_top", -1160.0f, 194.0f, -110.0f, 330.0f, 18.0f, 62.0f, rgb(51, 76, 99));
    cylinder(road, "focus_pillar_west", -1320.0f, 66.0f, 230.0f, 46.0f, 132.0f, 46.0f, rgb(117, 118, 111));
    cylinder(road, "focus_pillar_center", -1160.0f, 86.0f, 260.0f, 52.0f, 172.0f, 52.0f, rgb(133, 133, 123));
    cylinder(road, "focus_pillar_east", -1000.0f, 66.0f, 230.0f, 46.0f, 132.0f, 46.0f, rgb(117, 118, 111));
    cylinder(road, "bell_frame_west", -1290.0f, 95.0f, -62.0f, 18.0f, 190.0f, 18.0f, rgb(81, 55, 38));
    cylinder(road, "bell_frame_east", -1030.0f, 95.0f, -62.0f, 18.0f, 190.0f, 18.0f, rgb(81, 55, 38));
    box(road, "bell_frame_crossbar", -1160.0f, 181.0f, -62.0f, 278.0f, 18.0f, 20.0f, rgb(104, 71, 45));
    cone(road, "sage_bell", -1160.0f, 138.0f, -62.0f, 68.0f, 72.0f, 28.0f, rgb(212, 169, 75), PresentationDetailTier::Essential);
    box(road, "sanctuary_facade", -1160.0f, 92.0f, -265.0f, 410.0f, 180.0f, 250.0f, rgb(58, 38, 55), PresentationDetailTier::Full);
    box(road, "sanctuary_facade_band", -1160.0f, 192.0f, -265.0f, 475.0f, 28.0f, 300.0f, rgb(163, 41, 56), PresentationDetailTier::Full);
    box(road, "sanctuary_facade_roof", -1160.0f, 226.0f, -265.0f, 540.0f, 20.0f, 345.0f, rgb(42, 27, 42), PresentationDetailTier::Full);
    cylinder(road, "sanctuary_front_post_west", -1325.0f, 72.0f, -85.0f, 34.0f, 144.0f, 34.0f, rgb(108, 73, 48), PresentationDetailTier::Full);
    cylinder(road, "sanctuary_front_post_east", -995.0f, 72.0f, -85.0f, 34.0f, 144.0f, 34.0f, rgb(108, 73, 48), PresentationDetailTier::Full);

    // Authored dirt road segments, river, timber bridge and Tournament Road gate.
    box(road, "road_sanctuary", -1160.0f, 6.0f, 70.0f, 520.0f, 8.0f, 330.0f, rgb(198, 168, 115));
    box(road, "road_west", -700.0f, 6.0f, 60.0f, 420.0f, 8.0f, 205.0f, rgb(201, 171, 115));
    box(road, "road_midwest", -300.0f, 6.0f, 35.0f, 400.0f, 8.0f, 190.0f, rgb(201, 171, 115));
    box(road, "road_east_bank", 245.0f, 6.0f, 20.0f, 300.0f, 8.0f, 180.0f, rgb(201, 171, 115));
    box(road, "road_center", 500.0f, 6.0f, 10.0f, 300.0f, 8.0f, 175.0f, rgb(201, 171, 115));
    box(road, "road_east", 820.0f, 6.0f, 0.0f, 390.0f, 8.0f, 185.0f, rgb(201, 171, 115));
    box(road, "road_outskirts", 1190.0f, 6.0f, -10.0f, 360.0f, 8.0f, 195.0f, rgb(201, 171, 115));
    box(road, "east_extension_ground_base", 1740.0f, -28.0f, 0.0f, 1250.0f, 56.0f, 1980.0f, rgb(53, 90, 53));
    box(road, "east_extension_grass_surface", 1740.0f, 1.0f, 0.0f, 1230.0f, 5.0f, 1860.0f, rgb(87, 131, 74));
    box(road, "road_post_collapse", 1375.0f, 6.0f, -35.0f, 380.0f, 8.0f, 190.0f, rgb(201,171,115));
    box(road, "road_lens_approach", 1570.0f, 6.0f, -10.0f, 360.0f, 8.0f, 190.0f, rgb(201,171,115));
    box(road, "road_final_outskirts", 1850.0f, 6.0f, -10.0f, 520.0f, 8.0f, 205.0f, rgb(201,171,115));
    for(int x=1450,index=0;x<=2050;x+=150,++index){
        tree(road,"east_north_tree_"+std::to_string(index),static_cast<float>(x),-690.0f,.82f+(index%2)*.08f);
        tree(road,"east_south_tree_"+std::to_string(index),static_cast<float>(x),690.0f,.86f+((index+1)%2)*.08f);
    }
    cylinder(road,"checkpoint_post_north",1420.0f,62.0f,-125.0f,22.0f,124.0f,22.0f,rgb(91,63,49),PresentationDetailTier::Essential);
    cylinder(road,"checkpoint_post_south",1420.0f,62.0f,125.0f,22.0f,124.0f,22.0f,rgb(91,63,49),PresentationDetailTier::Essential);
    box(road,"checkpoint_banner",1420.0f,128.0f,0.0f,18.0f,34.0f,285.0f,rgb(207,72,69,.90f),PresentationDetailTier::Essential);
    box(road,"checkpoint_booth",1455.0f,38.0f,190.0f,95.0f,76.0f,90.0f,rgb(172,142,96),PresentationDetailTier::Full);
    for(int i=0;i<4;++i){
        const float x=1510.0f+i*170.0f;
        cylinder(road,"final_banner_post_"+std::to_string(i),x,68.0f,-240.0f,10.0f,136.0f,10.0f,rgb(77,56,44),PresentationDetailTier::Full);
        box(road,"final_banner_"+std::to_string(i),x,108.0f,-240.0f,10.0f,58.0f,76.0f,i%2?rgb(52,122,161,.86f):rgb(190,63,61,.86f),PresentationDetailTier::Full);
    }

    // The direct road has physically fallen away. The north-side trail is visible
    // from both sides so reconnecting feels geographic rather than teleported.
    for (int i=0;i<7;++i) {
        const float x=985.0f+static_cast<float>(i)*38.0f;
        const float z=-30.0f+std::sin(static_cast<float>(i)*1.2f)*70.0f;
        cylinder(road,"collapse_boulder_"+std::to_string(i),x,38.0f,z,70.0f+(i%3)*18.0f,76.0f,62.0f,rgb(91,82,70),PresentationDetailTier::Essential,"terrain_collapse");
    }
    const Vec2 detourPads[]={{890,-300},{960,-430},{1035,-515},{1115,-520},{1200,-455},{1290,-285}};
    for(int i=0;i<6;++i) {
        box(road,"detour_ledge_"+std::to_string(i),detourPads[i].x,7.0f,detourPads[i].z,
            105.0f,10.0f,95.0f, i<3?rgb(126,112,89):rgb(143,121,89),PresentationDetailTier::Essential,0.0f,"terrain_collapse");
    }
    box(road,"detour_jump_rubble_1",902.0f,24.0f,-320.0f,42.0f,42.0f,112.0f,rgb(95,85,72),PresentationDetailTier::Essential,5.0f);
    box(road,"detour_jump_rubble_2",970.0f,27.0f,-442.0f,44.0f,48.0f,118.0f,rgb(91,82,70),PresentationDetailTier::Essential,-4.0f);
    box(road,"detour_jump_rubble_3",1045.0f,29.0f,-525.0f,46.0f,52.0f,120.0f,rgb(87,79,68),PresentationDetailTier::Essential,3.0f);
    box(road,"detour_dash_gap",1115.0f,3.0f,-520.0f,110.0f,4.0f,82.0f,rgb(45,42,44,.92f),PresentationDetailTier::Essential,0.0f,"terrain_collapse");
    cylinder(road,"detour_swap_anchor",1205.0f,48.0f,-455.0f,34.0f,96.0f,34.0f,rgb(74,181,210,.82f),PresentationDetailTier::Essential);
    box(road, "river", 75.0f, 4.0f, 0.0f, 165.0f, 7.0f, 1680.0f, rgb(59, 140, 198, 0.94f));
    box(road, "river_bank_west", -30.0f, 12.0f, 0.0f, 46.0f, 22.0f, 1720.0f, rgb(133, 105, 68));
    box(road, "river_bank_east", 180.0f, 12.0f, 0.0f, 46.0f, 22.0f, 1720.0f, rgb(133, 105, 68));
    // The actual road crossing is broken. Pieces stop at each bank; the river
    // blocker remains active after Object Swap.
    for (const float x : {-45.0f, 195.0f}) {
        box(road, "broken_bridge_rail_" + std::to_string(static_cast<int>(x)), x, 20.0f, 0.0f, 14.0f, 14.0f, 240.0f, rgb(111, 78, 47));
        for (int z = -90; z <= 90; z += 60) {
            box(road, "broken_bridge_plank_" + std::to_string(static_cast<int>(x)) + "_" + std::to_string(z),
                x, 18.0f, static_cast<float>(z), 60.0f, 10.0f, 30.0f, rgb(123, 90, 55), PresentationDetailTier::Essential, 2.0f);
        }
    }
    cylinder(road, "tournament_gate_north", 600.0f, 72.0f, -135.0f, 38.0f, 145.0f, 38.0f, rgb(74, 47, 46), PresentationDetailTier::Essential, "swap_gate");
    cylinder(road, "tournament_gate_south", 600.0f, 72.0f, 135.0f, 38.0f, 145.0f, 38.0f, rgb(74, 47, 46), PresentationDetailTier::Essential, "swap_gate");
    box(road, "tournament_gate_red", 600.0f, 154.0f, 0.0f, 58.0f, 24.0f, 340.0f, rgb(210, 72, 70), PresentationDetailTier::Essential, 0.0f, "swap_gate");
    box(road, "tournament_gate_gold", 600.0f, 182.0f, 0.0f, 42.0f, 24.0f, 300.0f, rgb(240, 201, 91), PresentationDetailTier::Essential, 0.0f, "swap_gate");

    const Vec2 update3MainPads[]={{305,265},{365,185},{438,285},{520,105}};
    const Vec2 update3ForestPads[]={{285,-250},{345,-430},{430,-540},{520,-300}};
    const Vec2 update3CliffPads[]={{285,255},{340,420},{410,550},{480,430},{535,255}};
    for(int i=0;i<4;++i){
        cylinder(road,"update3_route_main_pad_"+std::to_string(i),update3MainPads[i].x,7,update3MainPads[i].z,74,4,58,rgb(185,151,99,.72f),PresentationDetailTier::Full);
        cylinder(road,"update3_route_forest_pad_"+std::to_string(i),update3ForestPads[i].x,7,update3ForestPads[i].z,62,4,52,rgb(82,112,70,.80f),PresentationDetailTier::Full);
        cylinder(road,"update3_forest_bell_post_"+std::to_string(i),update3ForestPads[i].x,34,update3ForestPads[i].z,8,68,8,rgb(82,64,47),PresentationDetailTier::Full);
    }
    for(int i=0;i<5;++i)
        box(road,"update3_route_cliff_pad_"+std::to_string(i),update3CliffPads[i].x,7,update3CliffPads[i].z,72,10,68,rgb(126,116,102),PresentationDetailTier::Full);

    const float bannerX[] = {-820.0f, -560.0f, -300.0f, 310.0f, 850.0f};
    for (int i = 0; i < 5; ++i) {
        cylinder(road, "route_banner_post_" + std::to_string(i), bannerX[i], 76.0f, -245.0f, 11.0f, 152.0f, 11.0f, rgb(76, 57, 44), PresentationDetailTier::Full);
        box(road, "route_banner_" + std::to_string(i), bannerX[i], 124.0f, -245.0f, 12.0f, 64.0f, 86.0f,
            i % 2 ? rgb(180, 62, 59, 0.88f) : rgb(50, 123, 160, 0.88f), PresentationDetailTier::Full);
        box(road, "route_banner_cap_" + std::to_string(i), bannerX[i], 158.0f, -245.0f, 18.0f, 9.0f, 100.0f,
            rgb(216, 184, 88), PresentationDetailTier::Full);
    }

    box(road, "outskirts_hall", 1910.0f, 190.0f, -260.0f, 650.0f, 370.0f, 85.0f, rgb(42, 26, 53), PresentationDetailTier::Full);
    box(road, "outskirts_hall_band", 1910.0f, 390.0f, -260.0f, 760.0f, 40.0f, 120.0f, rgb(211, 63, 120), PresentationDetailTier::Full);
    box(road, "outskirts_hall_crown", 1910.0f, 445.0f, -260.0f, 850.0f, 30.0f, 145.0f, rgb(240, 201, 91), PresentationDetailTier::Full);
    box(road, "outskirts_shop_fire", 1660.0f, 40.0f, 250.0f, 170.0f, 80.0f, 150.0f, rgb(179, 90, 61), PresentationDetailTier::Full);
    box(road, "outskirts_shop_blue", 1850.0f, 42.0f, 265.0f, 190.0f, 84.0f, 165.0f, rgb(61, 120, 180), PresentationDetailTier::Full);
    box(road, "outskirts_shop_violet", 2020.0f, 44.0f, 240.0f, 155.0f, 88.0f, 145.0f, rgb(107, 74, 167), PresentationDetailTier::Full);
    box(road, "road_prop_west", -420.0f, 26.0f, -250.0f, 100.0f, 52.0f, 70.0f, rgb(139, 119, 92), PresentationDetailTier::Full);
    box(road, "road_prop_mid", -260.0f, 22.0f, 255.0f, 82.0f, 44.0f, 64.0f, rgb(143, 121, 89), PresentationDetailTier::Full);
    box(road, "road_prop_east", 845.0f, 25.0f, -245.0f, 92.0f, 50.0f, 74.0f, rgb(128, 110, 84), PresentationDetailTier::Full);

    // Legacy landmark-art layer: irregular dirt, split-log fences, training
    // shrines, creek stones/reeds and distant mountain silhouettes.
    for (int i = 0; i < 13; ++i) {
        const float x = -1280.0f + static_cast<float>(i) * 215.0f;
        cylinder(road, "dirt_patch_" + std::to_string(i), x, 8.0f, std::sin(static_cast<float>(i) * .85f) * 45.0f,
                 300.0f + static_cast<float>(i % 3) * 48.0f, 3.0f, 164.0f + static_cast<float>(i % 2) * 36.0f,
                 i % 2 ? rgb(182,147,98,.76f) : rgb(194,160,108,.76f), PresentationDetailTier::Full);
    }
    for (const float z : {-360.0f, 360.0f}) for (int i = 0; i < 6; ++i) {
        const float x = -1180.0f + static_cast<float>(i) * 210.0f;
        cylinder(road, "trail_fence_post_" + std::to_string(static_cast<int>(z)) + "_" + std::to_string(i), x, 34.0f, z, 12.0f, 68.0f, 12.0f, rgb(101,68,47), PresentationDetailTier::Full);
        box(road, "trail_fence_rail_" + std::to_string(static_cast<int>(z)) + "_" + std::to_string(i), x, 47.0f, z, 150.0f, 8.0f, 8.0f, rgb(118,81,56), PresentationDetailTier::Full, 4.0f);
    }
    const Vec2 shrinePositions[] = {{-1110,250},{-930,-250},{-720,250}};
    for (int i=0;i<3;++i) {
        cylinder(road, "training_shrine_post_"+std::to_string(i), shrinePositions[i].x,54,shrinePositions[i].z,24,108,24,rgb(105,69,49),PresentationDetailTier::Full);
        cylinder(road, "training_shrine_cap_"+std::to_string(i), shrinePositions[i].x,90,shrinePositions[i].z,56,30,56,rgb(167,74,63),PresentationDetailTier::Full);
    }
    for (int i=0;i<7;++i) cylinder(road,"creek_stone_"+std::to_string(i),-10.0f+i*28.0f,14.0f,-115.0f+i*38.0f,24,18,20,rgb(116,120,111),PresentationDetailTier::Full);
    const Vec2 mountains[]={{-1300,-930},{-700,-1020},{0,-1050},{650,-1000},{1250,-930}};
    const float heights[]={360,450,390,510,420};
    for(int i=0;i<5;++i){cone(road,"mountain_"+std::to_string(i),mountains[i].x,heights[i]/2-10,mountains[i].z,520,heights[i],300,rgb(74,102,90,.46f),PresentationDetailTier::Full);cone(road,"mountain_snow_"+std::to_string(i),mountains[i].x,heights[i]-30,mountains[i].z,150,80,90,rgb(216,224,213,.36f),PresentationDetailTier::Full);}

    box(road, "fallen_log", 340.0f, 30.0f, 0.0f, 38.0f, 36.0f, 310.0f, rgb(107,72,47), PresentationDetailTier::Essential, -4.0f, "fallen_tree_center");
    box(road, "lens_thicket", 1700.0f, 35.0f, 0.0f, 75.0f, 70.0f, 250.0f, rgb(60,105,57), PresentationDetailTier::Essential, 0.0f, "lens_roadblock");
    box(road, "lens_sign_post", 1640.0f, 68.0f, 0.0f, 18.0f, 95.0f, 18.0f, rgb(106,73,47), PresentationDetailTier::Essential, 0.0f, "lens_roadblock");
    box(road, "lens_sign", 1640.0f, 120.0f, 0.0f, 15.0f, 55.0f, 120.0f, rgb(200,74,67), PresentationDetailTier::Essential, 0.0f, "lens_roadblock");

    // Update 6: route personality without changing collision or route counts.
    for (int i=0;i<4;++i) {
        box(road,"u6_main_crate_"+std::to_string(i),330.0f+i*72.0f,24.0f,465.0f+(i%2)*42.0f,
            48.0f,48.0f,48.0f,rgb(148,101,61),PresentationDetailTier::Full);
    }
    for (int i=0;i<4;++i) {
        cylinder(road,"u6_forest_bell_post_"+std::to_string(i),300.0f+i*70.0f,55.0f,-665.0f+(i%2)*36.0f,
                 12.0f,110.0f,12.0f,rgb(86,66,48),PresentationDetailTier::Full);
    }
    cylinder(road,"u6_cliff_overlook_post_a",505.0f,62.0f,655.0f,14.0f,124.0f,14.0f,rgb(104,76,55),PresentationDetailTier::Full);
    cylinder(road,"u6_cliff_overlook_post_b",565.0f,62.0f,655.0f,14.0f,124.0f,14.0f,rgb(104,76,55),PresentationDetailTier::Full);
    box(road,"u6_cliff_overlook_banner",535.0f,108.0f,655.0f,72.0f,34.0f,6.0f,rgb(204,67,56,.90f),PresentationDetailTier::Full);

    // Update 2: make the optional southern route read as an authored trail instead
    // of empty grass around the edge of a collision blocker.
    for (int i = 0; i < 6; ++i) {
        const float x = 1555.0f + static_cast<float>(i) * 55.0f;
        const float z = 500.0f + std::sin(static_cast<float>(i) * .8f) * 34.0f;
        cylinder(road, "south_detour_dirt_" + std::to_string(i), x, 7.0f, z,
                 130.0f, 3.0f, 86.0f, rgb(183,148,98,.82f), PresentationDetailTier::Full);
    }
    cylinder(road, "south_detour_post_a", 1580.0f, 46.0f, 455.0f, 12.0f, 92.0f, 12.0f, rgb(104,70,47), PresentationDetailTier::Full);
    cylinder(road, "south_detour_post_b", 1790.0f, 46.0f, 475.0f, 12.0f, 92.0f, 12.0f, rgb(104,70,47), PresentationDetailTier::Full);
    box(road, "south_detour_flag", 1790.0f, 78.0f, 475.0f, 10.0f, 38.0f, 60.0f, rgb(210,72,70,.86f), PresentationDetailTier::Full);

    road.ambientActors = {
        {"dojo_student", "ambient_student", {-900.0f, 0.0f, 300.0f, 1,1,1, -30.0f}, PresentationDetailTier::Full},
        {"traveler", "ambient_traveler", {-520.0f, 0.0f, -300.0f, 1,1,1, 25.0f}, PresentationDetailTier::Full},
        {"road_worker", "ambient_worker", {365.0f, 0.0f, 300.0f, 1,1,1, -90.0f}, PresentationDetailTier::Full},
        {"lost_competitor", "ambient_competitor", {730.0f, 0.0f, -285.0f, 1,1,1, 45.0f}, PresentationDetailTier::Full},
        {"tournament_fan", "ambient_fan", {1480.0f, 0.0f, 285.0f, 1,1,1, -45.0f}, PresentationDetailTier::Full}
        ,{"vendor", "ambient_vendor", {1750.0f, 0.0f, 260.0f, 1,1,1, 25.0f}, PresentationDetailTier::Full}
        ,{"sign_painter", "ambient_painter", {1840.0f, 0.0f, -300.0f, 1,1,1, -25.0f}, PresentationDetailTier::Full}
    };
}

} // namespace

WorldPresentationRegistry::WorldPresentationRegistry() {
    WorldPresentationDefinition field;
    field.id = "training-field";
    field.displayName = "Sage Training Field";
    field.camera = {38.0f, 43.0f, 900.0f, 930.0f, 1320.0f, 410.0f, 42.0f, 8.0f, 3000.0f,
                    -1240.0f, 0.0f, 190.0f, 135.0f, false};
    field.clearColor = rgb(139, 201, 232);
    field.fogColor = rgb(183, 220, 232);
    field.fogNear = 760.0f;
    field.fogFar = 1900.0f;
    addTrainingField(field);
    stages_.emplace(field.id, std::move(field));

    WorldPresentationDefinition road;
    road.id = "training-road";
    road.displayName = "Training Grounds and Tournament Road";
    road.camera = {38.0f, 45.0f, 980.0f, 930.0f, 1160.0f, 430.0f, 44.0f, 8.0f, 4200.0f,
                   280.0f, 0.0f, 2050.0f, 820.0f, true};
    road.clearColor = rgb(112, 173, 209);
    road.fogColor = rgb(185, 214, 205);
    road.fogNear = 1000.0f;
    road.fogFar = 2900.0f;
    addTrainingRoad(road);
    stages_.emplace(road.id, std::move(road));
}

const WorldPresentationDefinition& WorldPresentationRegistry::get(const std::string& id) const {
    const auto it = stages_.find(id);
    if (it == stages_.end()) throw std::out_of_range("Unknown world presentation: " + id);
    return it->second;
}

bool WorldPresentationRegistry::has(const std::string& id) const {
    return stages_.find(id) != stages_.end();
}

std::vector<std::string> WorldPresentationRegistry::ids() const {
    std::vector<std::string> result;
    result.reserve(stages_.size());
    for (const auto& pair : stages_) result.push_back(pair.first);
    return result;
}

} // namespace px
