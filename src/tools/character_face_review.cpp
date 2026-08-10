#include "content/character_face.hpp"
#include "content/character_model_asset.hpp"
#include "content/skeletal_animation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr int kWidth = 1800;
constexpr int kHeight = 1000;

struct Panel {
    int x{0}, y{0}, width{0}, height{0};
    px::RrvvfoFaceExpression expression{px::RrvvfoFaceExpression::Neutral};
};

struct ScreenVertex {
    float x{0.0f}, y{0.0f}, depth{0.0f};
};

float edge(float ax, float ay, float bx, float by, float pxValue, float pyValue) {
    return (pxValue - ax) * (by - ay) - (pyValue - ay) * (bx - ax);
}

std::uint8_t channel(float value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + .5f);
}

void fillRect(std::vector<std::array<std::uint8_t, 3>>& pixels,
              int x, int y, int width, int height, std::array<std::uint8_t, 3> color) {
    const int left = std::clamp(x, 0, kWidth);
    const int right = std::clamp(x + width, 0, kWidth);
    const int top = std::clamp(y, 0, kHeight);
    const int bottom = std::clamp(y + height, 0, kHeight);
    for (int py = top; py < bottom; ++py)
        for (int pxValue = left; pxValue < right; ++pxValue)
            pixels[static_cast<std::size_t>(py * kWidth + pxValue)] = color;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: px_character_face_review ASSET OUTPUT.ppm\n";
        return 2;
    }

    px::CharacterModelAsset model;
    std::string error;
    if (!model.loadCooked(argv[1], &error)) {
        std::cerr << error << "\n";
        return 3;
    }
    px::SkeletalAnimationPlayer player;
    player.bind(&model);
    if (!player.setState("idle", true)) {
        std::cerr << "The model has no idle clip\n";
        return 4;
    }
    player.seek(.375f);

    std::vector<std::array<float, 3>> positions;
    positions.reserve(model.vertices().size());
    for (const auto& vertex : model.vertices()) positions.push_back(player.skinPosition(vertex));

    std::vector<std::array<std::uint8_t, 3>> pixels(
        static_cast<std::size_t>(kWidth * kHeight), {20, 22, 27});
    fillRect(pixels, 0, 0, kWidth, 12, {210, 35, 29});
    fillRect(pixels, 0, kHeight - 12, kWidth, 12, {210, 35, 29});

    const std::array<Panel, 7> panels{{
        {30, 42, 610, 916, px::RrvvfoFaceExpression::Neutral},
        {660, 42, 360, 446, px::RrvvfoFaceExpression::Confident},
        {1035, 42, 360, 446, px::RrvvfoFaceExpression::Focused},
        {1410, 42, 360, 446, px::RrvvfoFaceExpression::Grit},
        {660, 512, 360, 446, px::RrvvfoFaceExpression::Hurt},
        {1035, 512, 360, 446, px::RrvvfoFaceExpression::Shout},
        {1410, 512, 360, 446, px::RrvvfoFaceExpression::Blink},
    }};

    for (const auto& panel : panels) {
        fillRect(pixels, panel.x, panel.y, panel.width, panel.height, {42, 45, 52});
        fillRect(pixels, panel.x, panel.y, panel.width, 6, {116, 25, 24});
        fillRect(pixels, panel.x, panel.y + panel.height - 6, panel.width, 6, {116, 25, 24});
        fillRect(pixels, panel.x, panel.y, 6, panel.height, {116, 25, 24});
        fillRect(pixels, panel.x + panel.width - 6, panel.y, 6, panel.height, {116, 25, 24});

        std::vector<float> depth(static_cast<std::size_t>(kWidth * kHeight),
                                 std::numeric_limits<float>::lowest());
        const float centerX = .016f;
        const float centerY = 1.645f;
        const float scale = std::min(panel.width * .88f / .72f, panel.height * .90f / .66f);
        auto project = [&](const std::array<float, 3>& position) {
            return ScreenVertex{
                panel.x + panel.width * .5f + (position[0] - centerX) * scale,
                panel.y + panel.height * .51f - (position[1] - centerY) * scale,
                position[2],
            };
        };
        auto rasterTriangle = [&](const ScreenVertex& a, const ScreenVertex& b, const ScreenVertex& c,
                                  const std::array<float, 4>& sourceColor, float light) {
            const float area = edge(a.x, a.y, b.x, b.y, c.x, c.y);
            if (std::abs(area) < 1.0e-5f) return;
            const int left = std::max(panel.x + 7, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
            const int right = std::min(panel.x + panel.width - 8,
                                       static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
            const int top = std::max(panel.y + 7, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
            const int bottom = std::min(panel.y + panel.height - 8,
                                        static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));
            const std::array<std::uint8_t, 3> color{
                channel(sourceColor[0] * light), channel(sourceColor[1] * light),
                channel(sourceColor[2] * light)};
            for (int py = top; py <= bottom; ++py) {
                for (int pxValue = left; pxValue <= right; ++pxValue) {
                    const float sampleX = static_cast<float>(pxValue) + .5f;
                    const float sampleY = static_cast<float>(py) + .5f;
                    const float wa = edge(b.x, b.y, c.x, c.y, sampleX, sampleY) / area;
                    const float wb = edge(c.x, c.y, a.x, a.y, sampleX, sampleY) / area;
                    const float wc = 1.0f - wa - wb;
                    if (wa < -1.0e-5f || wb < -1.0e-5f || wc < -1.0e-5f) continue;
                    const float z = wa * a.depth + wb * b.depth + wc * c.depth;
                    const auto pixel = static_cast<std::size_t>(py * kWidth + pxValue);
                    if (z <= depth[pixel]) continue;
                    depth[pixel] = z;
                    pixels[pixel] = color;
                }
            }
        };

        for (const auto& submesh : model.submeshes()) {
            const auto& material = model.materials()[submesh.materialIndex];
            for (std::uint32_t offset = 0; offset < submesh.indexCount; offset += 3) {
                const auto i0 = model.indices()[submesh.firstIndex + offset];
                const auto i1 = model.indices()[submesh.firstIndex + offset + 1];
                const auto i2 = model.indices()[submesh.firstIndex + offset + 2];
                const auto& p0 = positions[i0];
                const auto& p1 = positions[i1];
                const auto& p2 = positions[i2];
                const float dx1 = p1[0] - p0[0], dy1 = p1[1] - p0[1], dz1 = p1[2] - p0[2];
                const float dx2 = p2[0] - p0[0], dy2 = p2[1] - p0[1], dz2 = p2[2] - p0[2];
                const float nx = dy1 * dz2 - dz1 * dy2;
                const float ny = dz1 * dx2 - dx1 * dz2;
                const float nz = dx1 * dy2 - dy1 * dx2;
                const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
                const float facing = length > 1.0e-6f ? std::abs(nz) / length : 0.0f;
                const float light = .62f + .38f * facing;
                rasterTriangle(project(p0), project(p1), project(p2), material.color, light);
            }
        }

        for (const auto& triangle : px::rrvvfoFaceTriangles(panel.expression)) {
            const auto a = player.skinPosition(px::rrvvfoFaceVertex(triangle.positions[0]));
            const auto b = player.skinPosition(px::rrvvfoFaceVertex(triangle.positions[1]));
            const auto c = player.skinPosition(px::rrvvfoFaceVertex(triangle.positions[2]));
            rasterTriangle(project(a), project(b), project(c), triangle.color, 1.0f);
        }
    }

    std::ofstream output(argv[2], std::ios::binary);
    if (!output) {
        std::cerr << "Could not open output: " << argv[2] << "\n";
        return 5;
    }
    output << "P6\n" << kWidth << " " << kHeight << "\n255\n";
    for (const auto& pixel : pixels)
        output.write(reinterpret_cast<const char*>(pixel.data()), 3);
    std::cout << "Rendered actual-model face approval sheet -> " << argv[2] << "\n";
    return 0;
}
