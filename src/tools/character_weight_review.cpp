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

constexpr int kWidth = 720;
constexpr int kHeight = 720;

struct ScreenVertex {
    float x{0.0f};
    float y{0.0f};
    float depth{0.0f};
};

float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

std::uint8_t channel(float value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + .5f);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: px_character_weight_review ASSET CLIP TIME OUTPUT.ppm\n";
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
    if (!player.setState(argv[2], true)) {
        std::cerr << "Unknown clip: " << argv[2] << "\n";
        return 4;
    }
    player.seek(std::stof(argv[3]));

    std::vector<std::array<float, 3>> positions;
    positions.reserve(model.vertices().size());
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (const auto& vertex : model.vertices()) {
        const auto position = player.skinPosition(vertex);
        positions.push_back(position);
        minX = std::min(minX, position[0]);
        maxX = std::max(maxX, position[0]);
        minY = std::min(minY, position[1]);
        maxY = std::max(maxY, position[1]);
    }
    const float scale = .84f * std::min(
        static_cast<float>(kWidth) / std::max(.01f, maxX - minX),
        static_cast<float>(kHeight) / std::max(.01f, maxY - minY));
    const float centerX = (minX + maxX) * .5f;
    const float centerY = (minY + maxY) * .5f;

    std::vector<std::array<std::uint8_t, 3>> pixels(
        static_cast<std::size_t>(kWidth * kHeight), {238, 239, 242});
    std::vector<float> depth(
        static_cast<std::size_t>(kWidth * kHeight), std::numeric_limits<float>::lowest());

    auto project = [&](const std::array<float, 3>& position) {
        return ScreenVertex{
            kWidth * .5f + (position[0] - centerX) * scale,
            kHeight * .5f - (position[1] - centerY) * scale,
            position[2],
        };
    };

    for (const auto& submesh : model.submeshes()) {
        if (submesh.materialIndex >= model.materials().size()) continue;
        const auto& material = model.materials()[submesh.materialIndex];
        for (std::uint32_t offset = 0; offset < submesh.indexCount; offset += 3) {
            const auto first = submesh.firstIndex + offset;
            const auto i0 = model.indices()[first];
            const auto i1 = model.indices()[first + 1];
            const auto i2 = model.indices()[first + 2];
            const auto a = project(positions[i0]);
            const auto b = project(positions[i1]);
            const auto c = project(positions[i2]);
            const float area = edge(a.x, a.y, b.x, b.y, c.x, c.y);
            if (std::abs(area) < 1.0e-5f) continue;

            const int left = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
            const int right = std::min(kWidth - 1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
            const int top = std::max(0, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
            const int bottom = std::min(kHeight - 1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

            const float dx1 = positions[i1][0] - positions[i0][0];
            const float dy1 = positions[i1][1] - positions[i0][1];
            const float dz1 = positions[i1][2] - positions[i0][2];
            const float dx2 = positions[i2][0] - positions[i0][0];
            const float dy2 = positions[i2][1] - positions[i0][1];
            const float dz2 = positions[i2][2] - positions[i0][2];
            const float nz = dx1 * dy2 - dy1 * dx2;
            const float normalLength = std::sqrt(
                (dy1 * dz2 - dz1 * dy2) * (dy1 * dz2 - dz1 * dy2) +
                (dz1 * dx2 - dx1 * dz2) * (dz1 * dx2 - dx1 * dz2) + nz * nz);
            const float light = .72f + .28f * (normalLength > 1.0e-6f ? std::abs(nz) / normalLength : 0.0f);
            const std::array<std::uint8_t, 3> color{
                channel(material.color[0] * light),
                channel(material.color[1] * light),
                channel(material.color[2] * light),
            };

            for (int y = top; y <= bottom; ++y) {
                for (int x = left; x <= right; ++x) {
                    const float px = static_cast<float>(x) + .5f;
                    const float py = static_cast<float>(y) + .5f;
                    const float wa = edge(b.x, b.y, c.x, c.y, px, py) / area;
                    const float wb = edge(c.x, c.y, a.x, a.y, px, py) / area;
                    const float wc = 1.0f - wa - wb;
                    if (wa < -1.0e-5f || wb < -1.0e-5f || wc < -1.0e-5f) continue;
                    const float z = wa * a.depth + wb * b.depth + wc * c.depth;
                    const auto pixel = static_cast<std::size_t>(y * kWidth + x);
                    if (z <= depth[pixel]) continue;
                    depth[pixel] = z;
                    pixels[pixel] = color;
                }
            }
        }
    }

    std::ofstream output(argv[4], std::ios::binary);
    if (!output) {
        std::cerr << "Could not open output: " << argv[4] << "\n";
        return 5;
    }
    output << "P6\n" << kWidth << " " << kHeight << "\n255\n";
    for (const auto& pixel : pixels)
        output.write(reinterpret_cast<const char*>(pixel.data()), 3);
    std::cout << "Rendered " << argv[2] << " at " << argv[3] << "s -> " << argv[4] << "\n";
    return 0;
}
