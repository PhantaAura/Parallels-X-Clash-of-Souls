#include "platform/linux/presentation_renderer.hpp"
#include "content/character_face.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace px::linux {
namespace {

std::array<Uint8, 7> glyphRows(char input) {
    const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(input)));
    switch (c) {
        case 'A': return {14,17,17,31,17,17,17};
        case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {14,17,16,16,16,17,14};
        case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31};
        case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {14,17,16,23,17,17,15};
        case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {31,4,4,4,4,4,31};
        case 'J': return {7,2,2,2,18,18,12};
        case 'K': return {17,18,20,24,20,18,17};
        case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17};
        case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14};
        case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13};
        case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30};
        case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14};
        case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,21,10};
        case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4};
        case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14};
        case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31};
        case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2};
        case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14};
        case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14};
        case '9': return {14,17,17,15,1,1,14};
        case '-': return {0,0,0,31,0,0,0};
        case '_': return {0,0,0,0,0,0,31};
        case '.': return {0,0,0,0,0,6,6};
        case ',': return {0,0,0,0,0,6,4};
        case ':': return {0,6,6,0,6,6,0};
        case ';': return {0,6,6,0,6,4,8};
        case '!': return {4,4,4,4,4,0,4};
        case '?': return {14,17,1,2,4,0,4};
        case '/': return {1,2,2,4,8,8,16};
        case '\\': return {16,8,8,4,2,2,1};
        case '+': return {0,4,4,31,4,4,0};
        case '=': return {0,31,0,31,0,0,0};
        case '>': return {16,8,4,2,4,8,16};
        case '<': return {1,2,4,8,4,2,1};
        case '[': return {14,8,8,8,8,8,14};
        case ']': return {14,2,2,2,2,2,14};
        case '(': return {2,4,8,8,8,4,2};
        case ')': return {8,4,2,2,2,4,8};
        case '#': return {10,31,10,10,31,10,0};
        case '*': return {0,17,10,31,10,17,0};
        case '\'': return {4,4,2,0,0,0,0};
        case '"': return {10,10,5,0,0,0,0};
        case '|': return {4,4,4,4,4,4,4};
        default: return {0,0,0,0,0,0,0};
    }
}

SDL_Rect toSdl(UiRect rect) { return {rect.x, rect.y, rect.w, rect.h}; }

float clampRatio(float value) { return std::max(0.0f, std::min(1.0f, value)); }

std::string displayAscii(std::string value) {
    const std::array<std::pair<const char*, const char*>, 7> replacements{{
        {"\xE2\x80\x98", "'"}, {"\xE2\x80\x99", "'"},
        {"\xE2\x80\x9C", "\""}, {"\xE2\x80\x9D", "\""},
        {"\xE2\x80\x93", "-"}, {"\xE2\x80\x94", "-"},
        {"\xE2\x80\xA6", "..."},
    }};
    for (const auto& [needle, replacement] : replacements) {
        std::size_t position = 0;
        while ((position = value.find(needle, position)) != std::string::npos) {
            value.replace(position, std::char_traits<char>::length(needle), replacement);
            position += std::char_traits<char>::length(replacement);
        }
    }
    return value;
}

struct WorldPoint {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
};

WorldPoint subtract(WorldPoint a, WorldPoint b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
float dot(WorldPoint a, WorldPoint b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
WorldPoint cross(WorldPoint a, WorldPoint b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
WorldPoint normalize(WorldPoint value) {
    const float length = std::sqrt(dot(value, value));
    if (length < .0001f) return {0.0f, 1.0f, 0.0f};
    return {value.x / length, value.y / length, value.z / length};
}

WorldPoint transformPoint(WorldPoint local, const PresentationTransform& transform) {
    constexpr float pi = 3.14159265358979323846f;
    const float yaw = transform.yawDegrees * pi / 180.0f;
    const float x = local.x * transform.scaleX;
    const float y = local.y * transform.scaleY;
    const float z = local.z * transform.scaleZ;
    return {transform.x + x * std::cos(yaw) - z * std::sin(yaw),
            transform.y + y,
            transform.z + x * std::sin(yaw) + z * std::cos(yaw)};
}

class SoftwareWorldCanvas {
public:
    SoftwareWorldCanvas(SDL_Renderer* renderer, const WorldPresentationDefinition& stage,
                        float focusX, float focusZ)
        : renderer_(renderer), stage_(stage) {
        constexpr float pi = 3.14159265358979323846f;
        const float yaw = stage.camera.yawDegrees * pi / 180.0f;
        eye_ = {focusX + std::sin(yaw) * stage.camera.baseDistance,
                stage.camera.height,
                focusZ + std::cos(yaw) * stage.camera.baseDistance};
        const WorldPoint target{focusX, stage.camera.targetHeight, focusZ};
        forward_ = normalize(subtract(target, eye_));
        right_ = normalize(cross(forward_, {0.0f, 1.0f, 0.0f}));
        up_ = normalize(cross(right_, forward_));
        focal_ = 1.0f / std::tan(stage.camera.fovDegrees * pi / 360.0f);
    }

    void primitive(const WorldPrimitiveDefinition& definition) {
        switch (definition.kind) {
            case PresentationPrimitiveKind::Box: box(definition.transform, definition.color); break;
            case PresentationPrimitiveKind::Cylinder: cylinder(definition.transform, definition.color); break;
            case PresentationPrimitiveKind::Cone: cone(definition.transform, definition.color); break;
        }
    }

    void box(const PresentationTransform& transform, PresentationColor color) {
        const std::array<WorldPoint, 8> p{{
            transformPoint({-.5f,-.5f,-.5f}, transform), transformPoint({ .5f,-.5f,-.5f}, transform),
            transformPoint({ .5f, .5f,-.5f}, transform), transformPoint({-.5f, .5f,-.5f}, transform),
            transformPoint({-.5f,-.5f, .5f}, transform), transformPoint({ .5f,-.5f, .5f}, transform),
            transformPoint({ .5f, .5f, .5f}, transform), transformPoint({-.5f, .5f, .5f}, transform)
        }};
        quad(p[4], p[5], p[6], p[7], color);
        quad(p[1], p[0], p[3], p[2], color);
        quad(p[0], p[4], p[7], p[3], color);
        quad(p[5], p[1], p[2], p[6], color);
        quad(p[3], p[7], p[6], p[2], color);
        quad(p[0], p[1], p[5], p[4], color);
    }

    void cylinder(const PresentationTransform& transform, PresentationColor color, int segments = 12) {
        const WorldPoint bottomCenter = transformPoint({0.0f, -.5f, 0.0f}, transform);
        const WorldPoint topCenter = transformPoint({0.0f, .5f, 0.0f}, transform);
        constexpr float pi = 3.14159265358979323846f;
        for (int i = 0; i < segments; ++i) {
            const float a = pi * 2.0f * static_cast<float>(i) / segments;
            const float b = pi * 2.0f * static_cast<float>(i + 1) / segments;
            const WorldPoint b0 = transformPoint({std::cos(a) * .5f, -.5f, std::sin(a) * .5f}, transform);
            const WorldPoint b1 = transformPoint({std::cos(b) * .5f, -.5f, std::sin(b) * .5f}, transform);
            const WorldPoint t0 = transformPoint({std::cos(a) * .5f,  .5f, std::sin(a) * .5f}, transform);
            const WorldPoint t1 = transformPoint({std::cos(b) * .5f,  .5f, std::sin(b) * .5f}, transform);
            quad(b0, b1, t1, t0, color);
            triangle(topCenter, t0, t1, color);
            triangle(bottomCenter, b1, b0, color);
        }
    }

    void cone(const PresentationTransform& transform, PresentationColor color, int segments = 12) {
        const WorldPoint bottomCenter = transformPoint({0.0f, -.5f, 0.0f}, transform);
        const WorldPoint top = transformPoint({0.0f, .5f, 0.0f}, transform);
        constexpr float pi = 3.14159265358979323846f;
        for (int i = 0; i < segments; ++i) {
            const float a = pi * 2.0f * static_cast<float>(i) / segments;
            const float b = pi * 2.0f * static_cast<float>(i + 1) / segments;
            const WorldPoint b0 = transformPoint({std::cos(a) * .5f, -.5f, std::sin(a) * .5f}, transform);
            const WorldPoint b1 = transformPoint({std::cos(b) * .5f, -.5f, std::sin(b) * .5f}, transform);
            triangle(b0, b1, top, color);
            triangle(bottomCenter, b1, b0, color);
        }
    }

    void shadow(Vec2 position, float diameter) {
        cylinder({position.x, 3.2f, position.z, diameter, 2.4f, diameter * .64f, 0.0f},
                 {0.055f, 0.075f, 0.055f, .34f}, 18);
    }

    void model(const CharacterModelAsset& asset, const CharacterPresentationDefinition& binding,
               Vec2 position, float worldY, float yawDegrees, bool focused,
               const SkeletalAnimationPlayer* animation, RrvvfoFaceExpression faceExpression) {
        if (!asset.valid() || asset.height() <= .0001f) return;
        const auto& minimum = asset.boundsMin();
        const auto& maximum = asset.boundsMax();
        const float centerX = (minimum[0] + maximum[0]) * .5f;
        const float centerZ = (minimum[2] + maximum[2]) * .5f;
        const float scale = binding.worldHeight / asset.height();
        constexpr float pi = 3.14159265358979323846f;
        const float yaw = (yawDegrees + binding.modelYawOffsetDegrees) * pi / 180.0f;
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        auto point = [&](const CharacterModelVertex& vertex) {
            const auto positionValue = animation && animation->asset() == &asset
                ? animation->skinPosition(vertex) : vertex.position;
            const float x = (positionValue[0] - centerX) * scale;
            const float z = (positionValue[2] - centerZ) * scale;
            return WorldPoint{position.x + x * cosine - z * sine,
                              worldY + (positionValue[1] - minimum[1]) * scale,
                              position.z + x * sine + z * cosine};
        };
        for (const auto& submesh : asset.submeshes()) {
            const auto& material = asset.materials()[submesh.materialIndex];
            PresentationColor color{material.color[0], material.color[1], material.color[2], material.color[3]};
            if (focused) {
                color.r = std::min(1.0f, color.r * 1.08f);
                color.g = std::min(1.0f, color.g * 1.08f);
                color.b = std::min(1.0f, color.b * 1.08f);
            }
            for (std::uint32_t offset = 0; offset < submesh.indexCount; offset += 3) {
                const auto a = asset.indices()[submesh.firstIndex + offset];
                const auto b = asset.indices()[submesh.firstIndex + offset + 1];
                const auto c = asset.indices()[submesh.firstIndex + offset + 2];
                triangle(point(asset.vertices()[a]), point(asset.vertices()[b]), point(asset.vertices()[c]), color);
            }
        }
        if (animation && animation->asset() == &asset && asset.joints().size() > kRrvvfoFaceJointIndex) {
            for (const auto& face : rrvvfoFaceTriangles(faceExpression)) {
                const PresentationColor color{face.color[0], face.color[1], face.color[2], face.color[3]};
                triangle(point(rrvvfoFaceVertex(face.positions[0])),
                         point(rrvvfoFaceVertex(face.positions[1])),
                         point(rrvvfoFaceVertex(face.positions[2])), color);
            }
        }
    }

    void fallback(const CharacterPresentationDefinition& binding, Vec2 position,
                  float worldY, float yawDegrees, bool focused) {
        const float h = binding.worldHeight;
        auto shade = [](PresentationColor color, float amount) {
            color.r = std::min(1.0f, color.r * amount);
            color.g = std::min(1.0f, color.g * amount);
            color.b = std::min(1.0f, color.b * amount);
            return color;
        };
        const PresentationColor primary = focused ? shade(binding.primaryColor, 1.12f) : binding.primaryColor;
        const PresentationColor secondary = binding.secondaryColor;
        constexpr float pi = 3.14159265358979323846f;
        const float yaw = yawDegrees * pi / 180.0f;
        auto part = [&](float lx, float ly, float lz, float sx, float sy, float sz,
                        PresentationColor color, float localYaw = 0.0f) {
            const float wx = position.x + lx * std::cos(yaw) - lz * std::sin(yaw);
            const float wz = position.z + lx * std::sin(yaw) + lz * std::cos(yaw);
            box({wx, worldY + ly, wz, sx, sy, sz, yawDegrees + localYaw}, color);
        };
        shadow(position, h * .58f);
        if (binding.fallback == CharacterFallbackKind::ProceduralMentor) {
            const PresentationColor coat{.79f, .85f, .87f, 1.0f};
            const PresentationColor coatShade{.54f, .64f, .69f, 1.0f};
            const PresentationColor skin{.54f, .39f, .27f, 1.0f};
            part(-h*.09f, h*.18f, 0, h*.13f, h*.34f, h*.14f, secondary);
            part( h*.09f, h*.18f, 0, h*.13f, h*.34f, h*.14f, secondary);
            part(0, h*.49f, 0, h*.34f, h*.46f, h*.22f, coat);
            part(-h*.12f, h*.34f, h*.03f, h*.14f, h*.38f, h*.15f, coatShade, -4.0f);
            part( h*.12f, h*.34f, h*.03f, h*.14f, h*.38f, h*.15f, coatShade, 4.0f);
            part(-h*.24f, h*.53f, 0, h*.12f, h*.37f, h*.12f, coatShade, -12.0f);
            part( h*.24f, h*.53f, 0, h*.12f, h*.37f, h*.12f, coatShade, 12.0f);
            cylinder({position.x, worldY+h*.80f, position.z, h*.23f, h*.24f, h*.23f, yawDegrees}, skin, 12);
            part(0, h*.94f, 0, h*.30f, h*.13f, h*.28f, coat);
            part(-h*.13f, h*.99f, 0, h*.11f, h*.16f, h*.14f, coatShade, -18.0f);
            part( h*.13f, h*.99f, 0, h*.11f, h*.16f, h*.14f, coatShade, 18.0f);
        } else {
            part(-h*.09f, h*.17f, 0, h*.12f, h*.34f, h*.13f, secondary);
            part( h*.09f, h*.17f, 0, h*.12f, h*.34f, h*.13f, secondary);
            part(0, h*.49f, 0, h*.31f, h*.45f, h*.20f, primary);
            part(-h*.20f, h*.50f, 0, h*.10f, h*.38f, h*.10f, shade(primary, .82f), -8.0f);
            part( h*.20f, h*.50f, 0, h*.10f, h*.38f, h*.10f, shade(primary, .82f), 8.0f);
            cylinder({position.x, worldY+h*.82f, position.z, h*.24f, h*.24f, h*.24f, yawDegrees}, shade(primary, 1.08f), 12);
        }
    }

    void flush() {
        std::stable_sort(triangles_.begin(), triangles_.end(),
                         [](const Triangle& left, const Triangle& right) { return left.depth > right.depth; });
        for (const auto& triangle : triangles_)
            SDL_RenderGeometry(renderer_, nullptr, triangle.vertices.data(), 3, nullptr, 0);
    }

private:
    struct ProjectedPoint {
        float x{0.0f};
        float y{0.0f};
        float depth{0.0f};
        bool visible{false};
    };
    struct Triangle {
        std::array<SDL_Vertex, 3> vertices{};
        float depth{0.0f};
    };

    ProjectedPoint project(WorldPoint point) const {
        const WorldPoint relative = subtract(point, eye_);
        const float depth = dot(relative, forward_);
        if (depth <= stage_.camera.nearPlane) return {};
        constexpr float width = 1280.0f;
        constexpr float height = 720.0f;
        const float ndcX = dot(relative, right_) * focal_ / (depth * (width / height));
        const float ndcY = dot(relative, up_) * focal_ / depth;
        return {(ndcX * .5f + .5f) * width, (.5f - ndcY * .5f) * height, depth, true};
    }

    SDL_Color colorFor(PresentationColor color, WorldPoint normal, float depth) const {
        const WorldPoint light = normalize({-.45f, 1.0f, -.35f});
        const float diffuse = std::max(0.0f, dot(normal, light));
        float lightAmount = diffuse > .62f ? 1.0f : diffuse > .20f ? .78f : .58f;
        if (color.a < .9f) lightAmount = 1.0f;
        const float fogRange = std::max(1.0f, stage_.fogFar - stage_.fogNear);
        const float fog = std::clamp((depth - stage_.fogNear) / fogRange, 0.0f, 1.0f) * .72f;
        auto channel = [&](float value, float fogValue) {
            const float lit = std::clamp(value * lightAmount, 0.0f, 1.0f);
            return static_cast<Uint8>(std::clamp(lit * (1.0f - fog) + fogValue * fog, 0.0f, 1.0f) * 255.0f);
        };
        return {channel(color.r, stage_.fogColor.r), channel(color.g, stage_.fogColor.g),
                channel(color.b, stage_.fogColor.b),
                static_cast<Uint8>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f)};
    }

    void triangle(WorldPoint a, WorldPoint b, WorldPoint c, PresentationColor color) {
        const WorldPoint normal = normalize(cross(subtract(b, a), subtract(c, a)));
        if (dot(normal, subtract(eye_, a)) <= .0001f) return;
        const auto pa = project(a);
        const auto pb = project(b);
        const auto pc = project(c);
        if (!pa.visible || !pb.visible || !pc.visible) return;
        const float depth = (pa.depth + pb.depth + pc.depth) / 3.0f;
        const SDL_Color colorA = colorFor(color, normal, pa.depth);
        const SDL_Color colorB = colorFor(color, normal, pb.depth);
        const SDL_Color colorC = colorFor(color, normal, pc.depth);
        Triangle result;
        result.vertices = {{{{pa.x, pa.y}, colorA, {0,0}},
                            {{pb.x, pb.y}, colorB, {0,0}},
                            {{pc.x, pc.y}, colorC, {0,0}}}};
        result.depth = depth;
        triangles_.push_back(result);
    }

    void quad(WorldPoint a, WorldPoint b, WorldPoint c, WorldPoint d, PresentationColor color) {
        triangle(a, b, c, color);
        triangle(a, c, d, color);
    }

    SDL_Renderer* renderer_{nullptr};
    const WorldPresentationDefinition& stage_;
    WorldPoint eye_{};
    WorldPoint forward_{};
    WorldPoint right_{};
    WorldPoint up_{};
    float focal_{1.0f};
    std::vector<Triangle> triangles_;
};

} // namespace

PresentationRenderer::PresentationRenderer(SDL_Renderer* renderer,
                                           const UiPresentationRegistry& ui,
                                           const MenuRegistry& menus,
                                           const CombatManualRegistry& manual,
                                           const WorldPresentationRegistry& worlds,
                                           const CharacterPresentationRegistry& characters,
                                           const CharacterModelRepository& characterModels,
                                           const SkeletalAnimationPlayer& playerAnimation)
    : renderer_(renderer), ui_(ui), menus_(menus), manual_(manual), worlds_(worlds),
      characters_(characters), characterModels_(characterModels), playerAnimation_(playerAnimation) {}

void PresentationRenderer::color(UiColor value) {
    SDL_SetRenderDrawColor(renderer_, value.r, value.g, value.b, value.a);
}

void PresentationRenderer::fill(UiRect rect, UiColor value) {
    color(value);
    const auto target = toSdl(rect);
    SDL_RenderFillRect(renderer_, &target);
}

void PresentationRenderer::outline(UiRect rect, UiColor value, int thickness) {
    color(value);
    for (int i = 0; i < thickness; ++i) {
        const SDL_Rect target{rect.x + i, rect.y + i, rect.w - i * 2, rect.h - i * 2};
        SDL_RenderDrawRect(renderer_, &target);
    }
}

void PresentationRenderer::line(int x1, int y1, int x2, int y2, UiColor value, int thickness) {
    color(value);
    for (int i = -thickness / 2; i <= thickness / 2; ++i)
        SDL_RenderDrawLine(renderer_, x1 + i, y1, x2 + i, y2);
}

void PresentationRenderer::circle(int cx, int cy, int radius, UiColor value) {
    color(value);
    for (int y = -radius; y <= radius; ++y) {
        const int half = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
        const SDL_Rect row{cx - half, cy + y, half * 2 + 1, 1};
        SDL_RenderFillRect(renderer_, &row);
    }
}

void PresentationRenderer::diagonalBand(UiRect rect, int slant, UiColor value) {
    color(value);
    for (int row = 0; row < rect.h; ++row) {
        const int offset = rect.h ? slant * row / rect.h : 0;
        const SDL_Rect stripe{rect.x + offset, rect.y + row, rect.w, 1};
        SDL_RenderFillRect(renderer_, &stripe);
    }
}

void PresentationRenderer::glyph(char c, int x, int y, int scale, UiColor value) {
    const auto rows = glyphRows(c);
    color(value);
    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 5; ++column) {
            if (!(rows[static_cast<std::size_t>(row)] & (1u << (4 - column)))) continue;
            const SDL_Rect pixel{x + column * scale, y + row * scale, scale, scale};
            SDL_RenderFillRect(renderer_, &pixel);
        }
    }
}

void PresentationRenderer::text(const std::string& value, int x, int y, int scale, UiColor ink, bool shadow) {
    const auto display = displayAscii(value);
    int cursor = x;
    for (const char c : display) {
        if (c == '\n') { y += 8 * scale; cursor = x; continue; }
        if (shadow && c != ' ') glyph(c, cursor + scale, y + scale, scale, ui_.theme().nearBlack);
        if (c != ' ') glyph(c, cursor, y, scale, ink);
        cursor += 6 * scale;
    }
}

int PresentationRenderer::textWidth(const std::string& value, int scale) const {
    return static_cast<int>(displayAscii(value).size()) * 6 * scale;
}

void PresentationRenderer::centered(const std::string& value, int centerX, int y, int scale, UiColor ink, bool shadow) {
    text(value, centerX - textWidth(value, scale) / 2, y, scale, ink, shadow);
}

int PresentationRenderer::wrapped(const std::string& value, UiRect rect, int scale, UiColor ink, int lineGap) {
    std::istringstream words(displayAscii(value));
    std::string word;
    std::string current;
    int y = rect.y;
    const int rowHeight = 7 * scale + lineGap;
    while (words >> word) {
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && textWidth(candidate, scale) > rect.w) {
            text(current, rect.x, y, scale, ink);
            y += rowHeight;
            current = word;
            if (y + rowHeight > rect.y + rect.h) break;
        } else {
            current = candidate;
        }
    }
    if (!current.empty() && y + rowHeight <= rect.y + rect.h) {
        text(current, rect.x, y, scale, ink);
        y += rowHeight;
    }
    return y;
}

void PresentationRenderer::fighter(int cx, int feetY, int scale, UiColor body, UiColor edge, bool mentor) {
    const int head = 14 * scale;
    circle(cx, feetY - 93 * scale, head, edge);
    circle(cx, feetY - 94 * scale, head - 3 * scale, body);
    if (mentor) {
        fill({cx - 25 * scale, feetY - 112 * scale, 50 * scale, 5 * scale}, edge);
        fill({cx - 12 * scale, feetY - 120 * scale, 24 * scale, 10 * scale}, body);
    }
    fill({cx - 20 * scale, feetY - 77 * scale, 40 * scale, 48 * scale}, edge);
    fill({cx - 16 * scale, feetY - 74 * scale, 32 * scale, 42 * scale}, body);
    line(cx - 15 * scale, feetY - 68 * scale, cx - 34 * scale, feetY - 38 * scale, edge, 5 * scale);
    line(cx + 15 * scale, feetY - 68 * scale, cx + 36 * scale, feetY - 43 * scale, edge, 5 * scale);
    line(cx - 10 * scale, feetY - 30 * scale, cx - 18 * scale, feetY, edge, 6 * scale);
    line(cx + 10 * scale, feetY - 30 * scale, cx + 20 * scale, feetY, edge, 6 * scale);
}

bool PresentationRenderer::characterModel(const std::string& characterId, int cx, int feetY,
                                          int pixelHeight, float yawDegrees, bool focused,
                                          RrvvfoFaceExpression faceExpression) {
    const auto* asset = characterModels_.find(characterId);
    if (!asset || !asset->valid() || asset->height() <= 0.0001f) return false;
    const auto& binding = characters_.get(characterId);
    const auto& boundsMin = asset->boundsMin();
    const auto& boundsMax = asset->boundsMax();
    const float centerX = (boundsMin[0] + boundsMax[0]) * 0.5f;
    const float centerZ = (boundsMin[2] + boundsMax[2]) * 0.5f;
    const float scale = static_cast<float>(pixelHeight) / asset->height();
    const float yaw = (yawDegrees + binding.modelYawOffsetDegrees) * 3.1415926f / 180.0f;
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);

    struct DrawTriangle {
        std::array<SDL_Vertex, 3> vertices{};
        float depth{0.0f};
    };
    std::vector<DrawTriangle> triangles;
    triangles.reserve(asset->indices().size() / 3 + kRrvvfoFaceTriangleBudget);
    const std::array<float, 3> light = {-0.42f, 0.78f, 0.46f};
    const bool animated = playerAnimation_.asset() == asset;

    for (const auto& submesh : asset->submeshes()) {
        const auto& material = asset->materials()[submesh.materialIndex];
        for (std::uint32_t offset = 0; offset < submesh.indexCount; offset += 3) {
            DrawTriangle triangle;
            float depth = 0.0f;
            for (int corner = 0; corner < 3; ++corner) {
                const auto vertexIndex = asset->indices()[submesh.firstIndex + offset + static_cast<std::uint32_t>(corner)];
                const auto& source = asset->vertices()[vertexIndex];
                const auto skinnedPosition = animated ? playerAnimation_.skinPosition(source) : source.position;
                const float localX = skinnedPosition[0] - centerX;
                const float localZ = skinnedPosition[2] - centerZ;
                const float rotatedX = localX * cosine - localZ * sine;
                const float rotatedZ = localX * sine + localZ * cosine;
                const float rotatedNormalX = source.normal[0] * cosine - source.normal[2] * sine;
                const float rotatedNormalZ = source.normal[0] * sine + source.normal[2] * cosine;
                const float diffuse = std::max(0.0f, rotatedNormalX * light[0] + source.normal[1] * light[1] + rotatedNormalZ * light[2]);
                float lightAmount = material.unlit ? 1.0f : (diffuse > .62f ? 1.0f : diffuse > .20f ? .78f : .58f);
                if (focused) lightAmount = std::min(1.22f, lightAmount * 1.10f);
                const auto channel = [lightAmount](float value) {
                    return static_cast<Uint8>(std::clamp(value * lightAmount, 0.0f, 1.0f) * 255.0f);
                };
                triangle.vertices[static_cast<std::size_t>(corner)] = {
                    {cx + rotatedX * scale, feetY - (skinnedPosition[1] - boundsMin[1]) * scale + rotatedZ * scale * 0.08f},
                    {channel(material.color[0]), channel(material.color[1]), channel(material.color[2]),
                     static_cast<Uint8>(std::clamp(material.color[3], 0.0f, 1.0f) * 255.0f)},
                    {0.0f, 0.0f}
                };
                depth += rotatedZ;
            }
            triangle.depth = depth / 3.0f;
            triangles.push_back(triangle);
        }
    }
    if (characterId == "rrvvfo" && animated && asset->joints().size() > kRrvvfoFaceJointIndex) {
        for (const auto& face : rrvvfoFaceTriangles(faceExpression)) {
            DrawTriangle triangle;
            float depth = 0.0f;
            for (int corner = 0; corner < 3; ++corner) {
                const auto source = rrvvfoFaceVertex(face.positions[static_cast<std::size_t>(corner)]);
                const auto skinnedPosition = playerAnimation_.skinPosition(source);
                const float localX = skinnedPosition[0] - centerX;
                const float localZ = skinnedPosition[2] - centerZ;
                const float rotatedX = localX * cosine - localZ * sine;
                const float rotatedZ = localX * sine + localZ * cosine;
                const float emphasis = focused ? 1.10f : 1.0f;
                const auto channel = [emphasis](float value) {
                    return static_cast<Uint8>(std::clamp(value * emphasis, 0.0f, 1.0f) * 255.0f);
                };
                triangle.vertices[static_cast<std::size_t>(corner)] = {
                    {cx + rotatedX * scale,
                     feetY - (skinnedPosition[1] - boundsMin[1]) * scale + rotatedZ * scale * 0.08f},
                    {channel(face.color[0]), channel(face.color[1]), channel(face.color[2]),
                     static_cast<Uint8>(std::clamp(face.color[3], 0.0f, 1.0f) * 255.0f)},
                    {0.0f, 0.0f}
                };
                depth += rotatedZ;
            }
            triangle.depth = depth / 3.0f - 0.001f;
            triangles.push_back(triangle);
        }
    }
    std::stable_sort(triangles.begin(), triangles.end(),
                     [](const DrawTriangle& left, const DrawTriangle& right) { return left.depth < right.depth; });

    std::vector<SDL_Vertex> outlineVertices;
    std::vector<SDL_Vertex> colorVertices;
    outlineVertices.reserve(triangles.size() * 3);
    colorVertices.reserve(triangles.size() * 3);
    for (const auto& triangle : triangles) {
        for (auto vertex : triangle.vertices) {
            auto outline = vertex;
            outline.position.x = cx + (outline.position.x - cx) * 1.025f;
            outline.position.y = feetY + (outline.position.y - feetY) * 1.025f;
            outline.color = {10, 8, 12, 255};
            outlineVertices.push_back(outline);
            colorVertices.push_back(vertex);
        }
    }
    if (SDL_RenderGeometry(renderer_, nullptr, outlineVertices.data(),
                           static_cast<int>(outlineVertices.size()), nullptr, 0) != 0) return false;
    return SDL_RenderGeometry(renderer_, nullptr, colorVertices.data(),
                              static_cast<int>(colorVertices.size()), nullptr, 0) == 0;
}

void PresentationRenderer::xMotif(int cx, int cy, int size, UiColor value, int thickness) {
    line(cx - size, cy - size, cx + size, cy + size, value, thickness);
    line(cx + size, cy - size, cx - size, cy + size, value, thickness);
}

void PresentationRenderer::bar(UiRect rect, float ratio, UiColor fillColor, const std::string& label) {
    const auto& theme = ui_.theme();
    fill(rect, theme.nearBlack);
    outline(rect, theme.offWhite, 2);
    fill({rect.x + 4, rect.y + 4, static_cast<int>((rect.w - 8) * clampRatio(ratio)), rect.h - 8}, fillColor);
    text(label, rect.x + 8, rect.y + rect.h / 2 - 7, 2, theme.offWhite, true);
}

void PresentationRenderer::drawBackdrop(UiColor accent) {
    const auto& theme = ui_.theme();
    fill({0, 0, ui_.layout().designWidth, ui_.layout().designHeight}, theme.charcoal);
    for (int i = 0; i < 8; ++i)
        diagonalBand({-220 + i * 210, 0, 70, 720}, 210,
                     {static_cast<Uint8>(theme.panel.r / 2),
                      static_cast<Uint8>(theme.panel.g / 2),
                      static_cast<Uint8>(theme.panel.b / 2), 255});
    xMotif(1030, 360, 270, {11, 54, 108, 255}, 28);
    fill({0, 0, 1280, 12}, theme.warmGold);
    fill({0, 708, 1280, 12}, theme.warmGold);
    // Route/mode identity keeps its own accent without recoloring Legacy's
    // entire blue field.
    fill({0, 12, 10, 696}, accent);
}

void PresentationRenderer::drawTitle() {
    const auto& theme = ui_.theme();
    drawBackdrop(theme.crimson);
    xMotif(640, 314, 210, {92, 12, 20, 255}, 34);
    centered("PARALLELS", 580, 205, 10, theme.offWhite, true);
    centered("X", 1008, 170, 18, theme.fireRed, true);
    centered("CLASH OF SOULS", 640, 322, 5, theme.warmGold, true);
    diagonalBand({340, 446, 600, 62}, -18, theme.nearBlack);
    outline({338, 444, 604, 66}, theme.offWhite, 3);
    centered("PRESS ANY BUTTON", 640, 465, 4, theme.offWhite, true);
    centered("KEYBOARD  /  CONTROLLER  /  PHYSICAL BUTTONS", 640, 548, 2, theme.mutedText);
    text("3.0R  /  0.4H GOLDEN GATE QOL  /  LINUX VALIDATION", 42, 674, 2, theme.mutedText);
}

void PresentationRenderer::drawMode(const MenuSnapshot& menu) {
    const auto& theme = ui_.theme();
    const auto& layout = ui_.layout();
    const auto accent = UiPresentationRegistry::accent(theme, menu.selectedMode.accentId);
    const int motion = static_cast<int>(menu.transitionProgress * menu.transitionDirection * 170.0f);
    drawBackdrop(accent);
    text("PARALLELS X  /  CLASH OF SOULS", layout.topIdentity.x, layout.topIdentity.y, 3, accent);
    text("MODE SELECT", layout.topIdentity.x, layout.topIdentity.y + 30, 5, theme.offWhite, true);

    fill({80, 150, 1120, 360}, theme.panel);
    line(80, 150, 1148, 150, theme.offWhite, 4);
    line(80, 150, 80, 510, theme.warmGold, 8);
    line(32, 510, 1100, 510, theme.warmGold, 6);
    text("<", 58, 315, 8, accent, true);
    text(">", 1160, 315, 8, accent, true);

    if (menu.selectedMode.id == MenuModeId::Story) {
        if (!characterModel("rrvvfo", 852, 485, 248, 0.0f, true,
                            RrvvfoFaceExpression::Confident))
            text("RRVVFO MODEL REQUIRED", 760, 350, 2, theme.fireRed, true);
        fighter(1058, 474, 2, {185, 199, 205, 255}, theme.nearBlack, true);
        line(760, 480, 1140, 480, theme.ember, 5);
    } else if (menu.selectedMode.id == MenuModeId::ArenaBattle) {
        outline({770, 240, 370, 210}, accent, 7);
        line(790, 420, 1120, 270, accent, 3);
        fighter(850, 440, 2, theme.fireRed, theme.nearBlack);
        fighter(1050, 440, 2, {86, 49, 99, 255}, theme.nearBlack);
        centered("VS", 950, 300, 6, theme.offWhite, true);
    } else if (menu.selectedMode.id == MenuModeId::OnlinePlay) {
        color(accent);
        for (int radius : {48, 92, 136}) {
            for (int degree = 200; degree <= 340; degree += 3) {
                const float angle = static_cast<float>(degree) * 3.1415926f / 180.0f;
                SDL_RenderDrawPoint(renderer_, 952 + static_cast<int>(std::cos(angle) * radius),
                                   407 + static_cast<int>(std::sin(angle) * radius));
            }
        }
        circle(952, 407, 13, accent);
        xMotif(952, 330, 94, {70, 28, 34, 255}, 6);
    } else {
        xMotif(950, 360, 150, accent, 18);
        fighter(950, 500, 2, {70, 63, 74, 255}, theme.nearBlack);
    }

    text(menu.selectedMode.kicker, 126 + motion, 202, 3, accent);
    text(menu.selectedMode.label, 122 + motion, 248, menu.selectedMode.label.size() > 14 ? 5 : 7, theme.offWhite, true);
    wrapped(menu.selectedMode.description, {128 + motion, 342, 510, 104}, 2, theme.mutedText, 6);
    const std::string number = (menu.modeIndex + 1 < 10 ? "0" : "") + std::to_string(menu.modeIndex + 1);
    text(number + " / 10", 128, 458, 3, accent);

    if (!menu.selectedMode.implemented) {
        diagonalBand({430, 532, 420, 54}, -12, accent);
        centered("COMING LATER", 640, 549, 3, theme.nearBlack);
    } else if (menu.selectedMode.id == MenuModeId::Story) {
        const UiRect context = layout.storyContextAction;
        fill(context, menu.storySoFarSelected ? theme.warmGold : theme.nearBlack);
        outline(context, menu.storySoFarSelected ? theme.offWhite : accent, 3);
        centered("STORY SO FAR", context.x + context.w / 2, context.y + 17, 3,
                 menu.storySoFarSelected ? theme.nearBlack : theme.offWhite);
        text(menu.storySoFarSelected ? "UP / DOWN  SELECTED" : "UP / DOWN  OPTIONAL", context.x + 92, context.y + 41, 1,
             menu.storySoFarSelected ? theme.nearBlack : theme.mutedText);
    }

    text("LEFT / RIGHT  CHANGE MODE", 72, 665, 2, theme.mutedText);
    text("ENTER / A  CONFIRM", 984, 665, 2, theme.offWhite);
}

void PresentationRenderer::drawStoryCharacter(const MenuSnapshot& menu) {
    const auto& theme = ui_.theme();
    const auto accent = UiPresentationRegistry::accent(theme, menu.selectedRoute.accentId);
    const int motion = static_cast<int>(menu.transitionProgress * menu.transitionDirection * 150.0f);
    drawBackdrop(accent);
    text("PARALLELS X  /  STORY MODE", 58, 36, 3, theme.warmGold);
    text("ROUTE SELECT", 58, 72, 5, theme.offWhite, true);
    fill({84, 146, 690, 362}, theme.panel);
    outline({84, 146, 690, 362}, theme.offWhite, 4);
    fill({84, 146, 10, 362}, theme.warmGold);
    fill({84, 146, 690, 6}, theme.warmGold);
    text("NEW STORY", 124, 176, 2, theme.fireRed);
    text(menu.selectedRoute.characterName, 120 + motion, 218, 7, theme.offWhite, true);
    text(menu.selectedRoute.title, 122 + motion, 292, 3, theme.warmGold);
    wrapped(menu.selectedRoute.description, {124, 345, 590, 96}, 2, theme.offWhite, 6);
    text(menu.selectedRoute.id == "rrvvfo" ? "LIVE REPAIRED MODEL + EXPRESSIVE FACE" : "ROUTE ART PENDING",
         124, 466, 2, theme.mutedText);
    text("<", 38, 302, 8, theme.warmGold, true);
    text(">", 1212, 302, 8, theme.warmGold, true);

    if (menu.selectedRoute.id == "rrvvfo") {
        if (!characterModel("rrvvfo", 1005, 565, 430, 0.0f, true,
                            RrvvfoFaceExpression::Confident))
            text("RRVVFO MODEL REQUIRED", 866, 340, 3, theme.fireRed, true);
    } else {
        fighter(1005, 555, 4, accent, theme.nearBlack, menu.selectedRoute.id == "bark");
    }

    const auto action = menu.routeActions.empty() ? std::string{"STORY COMING LATER"} :
                        menu.routeActions[std::min(menu.routeActionIndex, menu.routeActions.size() - 1)];
    fill({390, 552, 500, 66}, theme.ember);
    outline({390, 550, 500, 70}, theme.nearBlack, 3);
    fill({390, 550, 500, 5}, theme.warmGold);
    centered(action, 640, 571, 3, theme.nearBlack);
    centered(std::to_string(menu.routeIndex + 1) + " / " + std::to_string(menu.visibleRoutes.size()), 640, 630, 2, theme.mutedText);
    text("ESC / B  MODE SELECT", 54, 670, 2, theme.mutedText);
    text("ENTER / A  CONFIRM", 990, 670, 2, theme.offWhite);
}

void PresentationRenderer::drawRecap(const MenuSnapshot& menu) {
    const auto& theme = ui_.theme();
    const auto& layout = ui_.layout();
    drawBackdrop(theme.warmGold);
    text("STORY MODE  /  STORY SO FAR", 54, 34, 3, theme.warmGold);
    text(menu.recapSection.title, 54, 72, 5, theme.offWhite, true);

    fill(layout.recapVisual, {29, 23, 31, 255});
    outline(layout.recapVisual, theme.offWhite, 4);
    xMotif(layout.recapVisual.x + layout.recapVisual.w / 2,
           layout.recapVisual.y + layout.recapVisual.h / 2, 155, {66, 44, 39, 255}, 14);
    fighter(layout.recapVisual.x + 260, layout.recapVisual.y + 420, 3, theme.fireRed, theme.nearBlack);
    fighter(layout.recapVisual.x + 500, layout.recapVisual.y + 425, 3, {102, 69, 118, 255}, theme.nearBlack);
    fill({layout.recapVisual.x + 22, layout.recapVisual.y + 22, 222, 34}, theme.crimson);
    text("REPLACEABLE STILL", layout.recapVisual.x + 36, layout.recapVisual.y + 31, 2, theme.offWhite);
    centered(menu.recapFrame.caption, layout.recapVisual.x + layout.recapVisual.w / 2,
             layout.recapVisual.y + layout.recapVisual.h - 46, 3, theme.warmGold, true);

    fill(layout.recapCopy, {21, 18, 24, 250});
    outline(layout.recapCopy, theme.warmGold, 4);
    const std::string sectionCount = "SECTION " + std::to_string(menu.recapSectionIndex + 1) + " / 10";
    text(sectionCount, layout.recapCopy.x + 24, layout.recapCopy.y + 25, 2, theme.warmGold);
    text(menu.recapFrame.caption, layout.recapCopy.x + 24, layout.recapCopy.y + 62, 3, theme.offWhite);
    wrapped(menu.recapFrame.body, {layout.recapCopy.x + 24, layout.recapCopy.y + 116,
            layout.recapCopy.w - 48, 250}, 2, theme.offWhite, 7);
    text("UP / DOWN", layout.recapCopy.x + 24, layout.recapCopy.y + 405, 2, theme.warmGold);
    text("SECTION SELECT", layout.recapCopy.x + 24, layout.recapCopy.y + 430, 2, theme.mutedText);
    text("LEFT / RIGHT", layout.recapCopy.x + 24, layout.recapCopy.y + 472, 2, theme.warmGold);
    text("PREVIOUS / NEXT", layout.recapCopy.x + 24, layout.recapCopy.y + 497, 2, theme.mutedText);

    for (int i = 0; i < 10; ++i) {
        const UiColor dot = i == static_cast<int>(menu.recapSectionIndex) ? theme.warmGold : UiColor{77, 68, 69, 255};
        fill({420 + i * 45, 628, i == static_cast<int>(menu.recapSectionIndex) ? 32 : 20, 8}, dot);
    }
    text("ENTER / A  NEXT", 54, 674, 2, theme.offWhite);
    text("ESC / B  EXIT     S / X  SKIP", 850, 674, 2, theme.mutedText);
}

void PresentationRenderer::drawComingLater(const MenuSnapshot& menu) {
    const auto& theme = ui_.theme();
    const auto accent = UiPresentationRegistry::accent(theme, menu.selectedRoute.accentId);
    drawBackdrop(accent);
    fighter(640, 485, 4, accent, theme.nearBlack);
    centered(menu.selectedRoute.characterName, 640, 145, 7, theme.offWhite, true);
    diagonalBand({300, 514, 680, 82}, -20, accent);
    centered("STORY COMING LATER", 640, 538, 4, theme.nearBlack);
    centered("THIS ROUTE IS DISCOVERED  /  NO FAKE MISSIONS", 640, 630, 2, theme.mutedText);
}

void PresentationRenderer::drawUnlock(const MenuSnapshot& menu) {
    const auto& theme = ui_.theme();
    drawBackdrop(theme.warmGold);
    centered("NEW STORY UNLOCKED", 640, 224, 5, theme.warmGold, true);
    centered(menu.unlockRouteId, 640, 320, 8, theme.offWhite, true);
    xMotif(640, 350, 180, {88, 63, 27, 255}, 16);
    centered("AVAILABLE IN STORY CHARACTER SELECT", 640, 510, 2, theme.mutedText);
}

void PresentationRenderer::renderMenu(const MenuSnapshot& menu) {
    switch (menu.screen) {
        case MenuScreen::Title: drawTitle(); break;
        case MenuScreen::ModeSelect: drawMode(menu); break;
        case MenuScreen::StoryCharacterSelect: drawStoryCharacter(menu); break;
        case MenuScreen::StorySoFar: drawRecap(menu); break;
        case MenuScreen::StoryComingLater: drawComingLater(menu); break;
        case MenuScreen::UnlockCelebration: drawUnlock(menu); break;
    }
}

void PresentationRenderer::renderManual(std::size_t pageIndex, const RuntimeView* training) {
    const auto& theme = ui_.theme();
    const auto& layout = ui_.layout();
    fill({0, 0, 1280, 720}, theme.nearBlack);
    diagonalBand({18, 18, 1244, 684}, -22, theme.manualPaper);
    outline({22, 22, 1236, 676}, theme.nearBlack, 6);
    text("THE SAGE'S COMBAT MANUAL", 48, 42, 3, theme.crimson);
    text("CHAPTER 1 EDITION", 48, 76, 5, theme.nearBlack);
    text(training ? "LEFT / RIGHT  PAGES" : "M / ESC  CLOSE", training ? 974 : 1040, 58, 2, theme.crimson);

    const auto& pages = manual_.pages();
    pageIndex = std::min(pageIndex, pages.size() - 1);
    fill(layout.manualNavigation, {229, 215, 181, 255});
    outline(layout.manualNavigation, theme.nearBlack, 4);
    int y = layout.manualNavigation.y + 18;
    std::string category;
    for (std::size_t i = 0; i < pages.size(); ++i) {
        if (pages[i].category != category) {
            category = pages[i].category;
            text(category, layout.manualNavigation.x + 16, y, 1, theme.crimson);
            y += 18;
        }
        const bool selected = i == pageIndex;
        if (selected) fill({layout.manualNavigation.x + 10, y - 6, layout.manualNavigation.w - 20, 31}, theme.warmGold);
        text(pages[i].title, layout.manualNavigation.x + 18, y, pages[i].title.size() > 20 ? 1 : 2,
             selected ? theme.nearBlack : UiColor{55, 48, 44, 255});
        y += 38;
    }

    const auto& page = pages[pageIndex];
    fill(layout.manualPage, {250, 246, 230, 255});
    outline(layout.manualPage, theme.nearBlack, 5);
    text(page.category + "  /  " + page.kicker, layout.manualPage.x + 24, layout.manualPage.y + 20, 2, theme.crimson);
    text(page.title, layout.manualPage.x + 22, layout.manualPage.y + 56,
         page.title.size() > 24 ? 4 : 5, theme.nearBlack);
    wrapped(page.summary, {layout.manualPage.x + 26, layout.manualPage.y + 112,
            layout.manualPage.w - 52, 58}, 2, {70, 60, 55, 255}, 4);

    int entryY = layout.manualPage.y + 182;
    for (const auto& entry : page.entries) {
        const int height = training ? 58 : 64;
        fill({layout.manualPage.x + 24, entryY, layout.manualPage.w - 48, height}, {237, 226, 198, 255});
        outline({layout.manualPage.x + 24, entryY, layout.manualPage.w - 48, height}, theme.nearBlack, 2);
        text(entry.label, layout.manualPage.x + 38, entryY + 10, 2, theme.nearBlack);
        const std::string prompts = entry.keyboardPrompt +
            (!entry.keyboardPrompt.empty() && !entry.controllerPrompt.empty() ? "  /  " : "") + entry.controllerPrompt;
        text(prompts, layout.manualPage.x + 38, entryY + 36, 1, theme.crimson);
        wrapped(entry.description, {layout.manualPage.x + 340, entryY + 10,
                layout.manualPage.w - 380, 46}, 1, {70, 60, 55, 255}, 3);
        entryY += height + (training ? 6 : 9);
        if (entryY > layout.manualPage.y + layout.manualPage.h - (training ? 100 : 65)) break;
    }
    if (training) {
        text("UP / DOWN  CHOOSE     ENTER / A  BEGIN", 348, 603, 1, theme.crimson);
        const UiRect actions{326, 620, 910, 66};
        fill(actions, theme.nearBlack);
        outline(actions, theme.crimson, 3);
        const std::size_t count = training->trainingManualOptions.size();
        const int width = count ? (actions.w - 24) / static_cast<int>(count) : actions.w;
        for (std::size_t i = 0; i < count; ++i) {
            const UiRect option{actions.x + 12 + static_cast<int>(i) * width, actions.y + 10, width - 8, 44};
            if (i == training->trainingManualSelection) fill(option, theme.warmGold);
            outline(option, i == training->trainingManualSelection ? theme.offWhite : theme.crimson, 2);
            centered(training->trainingManualOptions[i], option.x + option.w / 2, option.y + 17, 1,
                     i == training->trainingManualSelection ? theme.nearBlack : theme.offWhite);
        }
    }
}

void PresentationRenderer::drawWorld(const RuntimeView& view) {
    if (!worlds_.has(view.presentationStageId)) {
        fill({0, 0, 1280, 720}, {21, 26, 31, 255});
        return;
    }
    const auto& stage = worlds_.get(view.presentationStageId);
    for (int y = 0; y < 720; ++y) {
        const float amount = std::clamp(static_cast<float>(y) / 520.0f, 0.0f, 1.0f) * .46f;
        auto channel = [&](float clear, float fog) {
            return static_cast<Uint8>(std::clamp(clear * (1.0f - amount) + fog * amount, 0.0f, 1.0f) * 255.0f);
        };
        fill({0, y, 1280, 1}, {channel(stage.clearColor.r, stage.fogColor.r),
                                channel(stage.clearColor.g, stage.fogColor.g),
                                channel(stage.clearColor.b, stage.fogColor.b), 255});
    }

    float requestedFocusX = stage.camera.focusCenterX;
    float requestedFocusZ = stage.camera.focusCenterZ;
    if (stage.camera.followPlayer) {
        requestedFocusX = view.playerPosition.x;
        requestedFocusZ = view.playerPosition.z;
        if (view.opponentVisible) {
            requestedFocusX = (view.playerPosition.x + view.opponentPosition.x) * .5f;
            requestedFocusZ = (view.playerPosition.z + view.opponentPosition.z) * .5f;
        }
    }
    const float shakeSign = std::sin((view.playerPosition.x + view.playerPosition.z) * .017f) >= 0.0f ? 1.0f : -1.0f;
    const float shake = std::min(8.0f, view.cameraImpulse * 1.15f) * shakeSign;
    const float focusX = std::clamp(requestedFocusX + shake,
                                    stage.camera.focusCenterX - stage.camera.focusClampX,
                                    stage.camera.focusCenterX + stage.camera.focusClampX);
    const float focusZ = std::clamp(requestedFocusZ - shake * .45f,
                                    stage.camera.focusCenterZ - stage.camera.focusClampZ,
                                    stage.camera.focusCenterZ + stage.camera.focusClampZ);
    SoftwareWorldCanvas baseCanvas(renderer_, stage, focusX, focusZ);
    SoftwareWorldCanvas surfaceCanvas(renderer_, stage, focusX, focusZ);
    SoftwareWorldCanvas sceneryCanvas(renderer_, stage, focusX, focusZ);
    for (const auto& primitive : stage.primitives) {
        const auto& id = primitive.id;
        const bool baseLayer = id.find("outer_turf") != std::string::npos ||
                               id.find("outer_ground") != std::string::npos ||
                               id.find("ground_base") != std::string::npos;
        const bool surfaceLayer = id.find("grass_surface") != std::string::npos ||
                                  id.find("field_tile_") == 0 ||
                                  id.find("field_center_") == 0;
        if (baseLayer) baseCanvas.primitive(primitive);
        else if (surfaceLayer) surfaceCanvas.primitive(primitive);
        else sceneryCanvas.primitive(primitive);
    }
    // Reduced software rendering uses painter layers. These mirror the Legacy
    // stage order (slab -> grass/markings -> props) and avoid a large slab side
    // face cutting diagonally across the playable surface without a depth buffer.
    baseCanvas.flush();
    surfaceCanvas.flush();
    sceneryCanvas.flush();

    // Legacy renders fighters/interactive props after the stage. Keeping that
    // layer order also avoids large ground triangles hiding a character in the
    // reduced painter-sorted Linux renderer (the Mac renderer has a depth buffer).
    SoftwareWorldCanvas actorCanvas(renderer_, stage, focusX, focusZ);

    if (view.ambientActors.empty()) {
        for (const auto& actor : stage.ambientActors) {
            const auto& binding = characters_.get(actor.characterId);
            actorCanvas.fallback(binding, {actor.transform.x, actor.transform.z}, actor.transform.y,
                                 actor.transform.yawDegrees, false);
        }
    } else {
        for (const auto& actor : view.ambientActors) {
            const auto& binding = characters_.get(actor.presentationId);
            actorCanvas.fallback(binding, actor.position, 0.0f, actor.yawDegrees, actor.interactable);
        }
    }
    for (const auto& marker : view.worldMarkers) {
        const PresentationColor markerColor = marker.complete
            ? PresentationColor{.94f, .75f, .28f, .56f}
            : PresentationColor{.42f, .89f, 1.0f, .62f};
        if (marker.kind == "swap-rock")
            actorCanvas.box({marker.position.x, 23.0f, marker.position.z, 56.0f, 42.0f, 50.0f, 12.0f},
                            {.46f, .47f, .43f, 1.0f});
        else if (marker.kind == "fire-blast")
            actorCanvas.cylinder({marker.position.x, 72.0f, marker.position.z, 22.0f, 52.0f, 22.0f, 0.0f},
                                 {1.0f, .28f, .08f, .90f}, 14);
        else if (marker.kind == "object-swap-fx")
            actorCanvas.cylinder({marker.position.x, 5.0f, marker.position.z, 82.0f, 8.0f, 82.0f, 0.0f},
                                 {.50f, .94f, 1.0f, .52f}, 18);
        else if (marker.kind == "lens-fx")
            actorCanvas.cylinder({marker.position.x, 92.0f, marker.position.z, 48.0f, 5.0f, 48.0f, 0.0f},
                                 {1.0f, .74f, .18f, .65f}, 18);
        else if (marker.kind == "bird")
            actorCanvas.box({marker.position.x, 145.0f, marker.position.z, 22.0f, 5.0f, 10.0f, 12.0f},
                            {.91f, .94f, 1.0f, .82f});
        else
            actorCanvas.cylinder({marker.position.x, 31.0f, marker.position.z,
                                  marker.complete ? 17.0f : 24.0f, 62.0f,
                                  marker.complete ? 17.0f : 24.0f, 0.0f}, markerColor, 12);
    }
    const bool playerFocus = view.dialogueVisible && view.dialogueFocusActorId == "rrvvfo";
    const bool playerSpeaking = view.dialogueVisible && (view.dialogueFocusActorId == "rrvvfo" ||
        view.dialoguePortraitId == "rrvvfo" || view.dialogueSpeaker == "RRVVFO");
    const auto faceExpression = resolveRrvvfoFaceExpression(
        view.playerAnimation, playerAnimation_.time(), view.playerFaceSeconds,
        view.dialogueExpression, playerSpeaking);
    const auto& rrvvfo = characters_.get("rrvvfo");
    actorCanvas.shadow(view.playerPosition, rrvvfo.worldHeight * .58f);
    const auto* model = characterModels_.find("rrvvfo");
    if (model && model->valid())
        actorCanvas.model(*model, rrvvfo, view.playerPosition, view.playerHeight, view.playerYawDegrees,
                          playerFocus, &playerAnimation_, faceExpression);

    if (view.opponentVisible || view.mode == GameMode::ArenaCombat) {
        const auto& sage = characters_.get("sage");
        actorCanvas.fallback(sage, view.opponentPosition, view.opponentHeight, view.opponentYawDegrees,
                             view.dialogueVisible && view.dialogueFocusActorId == "sage");
        if (view.opponentAttackTelegraphed)
            actorCanvas.cylinder({view.opponentPosition.x, 3.0f, view.opponentPosition.z, 180.0f, 5.0f, 180.0f, 0.0f},
                                 {1.0f, .18f, .08f, .44f}, 20);
    }
    actorCanvas.flush();
}

void PresentationRenderer::drawHud(const RuntimeView& view) {
    const auto& theme = ui_.theme();
    const auto& layout = ui_.layout();
    if (!view.objective.empty()) {
        const UiRect objective = view.mode == GameMode::ArenaCombat ? UiRect{392, 74, 496, 54} : layout.objective;
        fill(objective, {12, 10, 14, 224});
        fill({objective.x, objective.y, 9, objective.h}, theme.crimson);
        outline(objective, theme.offWhite, 2);
        text("CURRENT OBJECTIVE", objective.x + 24, objective.y + 9, 1, theme.warmGold);
        wrapped(view.objective, {objective.x + 24, objective.y + 28, objective.w - 48, 21}, 1, theme.offWhite, 2);
        if (!view.objectiveDetail.empty() && view.mode != GameMode::ArenaCombat)
            wrapped(view.objectiveDetail, {objective.x + 24, objective.y + objective.h + 7,
                    objective.w - 48, 34}, 1, theme.mutedText, 2);
    }
    if (view.mode == GameMode::ArenaCombat) {
        if (view.showPlayerHealth) bar({28, 28, 410, 34}, view.player.hp / std::max(1.0f, view.player.maxHp), theme.fireRed, "RRVVFO HP");
        if (view.showEnergy) bar({28, 67, 300, 25}, view.player.energy / 100.0f, {54, 133, 224, 255}, "ENERGY");
        if (view.showGuard) bar({28, 98, 300, 25}, view.player.guard / 100.0f, theme.warmGold, "GUARD");
        if (view.showOpponentHealth) bar({842, 28, 410, 34}, view.opponent.hp / std::max(1.0f, view.opponent.maxHp), {187, 198, 204, 255}, "SAGE HP");
    }
    if (view.hotbarVisible && !view.hotbar.empty()) {
        const UiRect hotbar = layout.hotbar;
        fill(hotbar, {10, 8, 12, 230});
        outline(hotbar, theme.offWhite, 2);
        int x = hotbar.x + 12;
        int index = 1;
        for (const auto& ability : view.hotbar) {
            const UiRect slot{x, hotbar.y + 12, 124, 88};
            fill(slot, {39, 31, 40, 255});
            const bool trick = ability.id == "objectSwap" || ability.id == "lensOfTruth";
            outline(slot, trick ? theme.warmGold : theme.fireRed, 3);
            text(std::to_string(index), slot.x + 8, slot.y + 8, 2, theme.warmGold);
            wrapped(ability.label, {slot.x + 8, slot.y + 38, slot.w - 16, 42}, 1, theme.offWhite, 2);
            x += 134;
            ++index;
        }
    }
    if (!view.nearbyInteractionLabel.empty()) {
        const std::string prompt = "E / A  TALK TO " + view.nearbyInteractionLabel;
        fill({430, 454, 420, 44}, theme.nearBlack);
        outline({430, 454, 420, 44}, theme.warmGold, 2);
        centered(prompt, 640, 468, 2, theme.offWhite);
    }
    if (view.trainingStepCount > 0 && !view.trainingPrompt.empty()) {
        fill({392, 138, 496, 70}, {10, 8, 12, 230});
        outline({392, 138, 496, 70}, view.opponentAttackTelegraphed ? theme.fireRed : theme.warmGold, 3);
        centered(view.trainingPrompt, 640, 158, 2, theme.offWhite);
    }
    if (!view.gameplayNotice.empty() && !view.dialogueVisible) {
        fill({410, 420, 460, 42}, {10, 8, 12, 238});
        outline({410, 420, 460, 42}, view.flowCancelReady ? theme.fireRed : theme.warmGold, 2);
        centered(view.gameplayNotice, 640, 434, 1, theme.offWhite);
    }
}

void PresentationRenderer::drawDialogue(const RuntimeView& view) {
    if (!view.dialogueVisible) return;
    const auto& theme = ui_.theme();
    const auto box = ui_.layout().dialogue;
    fill(box, {12, 10, 14, 244});
    outline(box, theme.offWhite, 4);
    const UiColor speakerAccent = view.dialogueSpeaker == "RRVVFO" ? theme.fireRed : theme.warmGold;
    fill({box.x, box.y, 190, box.h}, theme.panel);
    if (view.dialogueSpeaker == "RRVVFO") {
        const auto expression = resolveRrvvfoFaceExpression(
            playerAnimation_.state(), playerAnimation_.time(), view.playerFaceSeconds,
            view.dialogueExpression, true);
        if (!characterModel("rrvvfo", box.x + 94, box.y + box.h - 8, 142, 0.0f, true, expression))
            text("MODEL REQUIRED", box.x + 32, box.y + 76, 1, theme.fireRed);
    } else {
        fighter(box.x + 94, box.y + box.h - 14, 1, speakerAccent, theme.nearBlack, true);
    }
    fill({box.x + 174, box.y, 16, box.h}, speakerAccent);
    text(view.dialogueSpeaker, box.x + 214, box.y + 18, 3, speakerAccent);
    wrapped(view.dialogueText, {box.x + 214, box.y + 58, box.w - 244, 76}, 2, theme.offWhite, 5);
    text("ENTER / A  CONTINUE", box.x + box.w - 260, box.y + box.h - 26, 1, theme.mutedText);
}

void PresentationRenderer::drawPause(const RuntimeView& view) {
    const auto& theme = ui_.theme();
    const UiColor panel = view.highContrastHud ? UiColor{0, 0, 0, 252} : UiColor{12, 10, 14, 246};
    const UiColor ink = view.highContrastHud ? UiColor{255, 255, 255, 255} : theme.offWhite;
    fill({0, 0, 1280, 720}, {3, 3, 5, 190});
    fill({145, 45, 990, 630}, panel);
    outline({145, 45, 990, 630}, view.highContrastHud ? ink : theme.warmGold, 4);
    text(view.pausePageTitle, 184, 76, view.largerText ? 4 : 3, theme.warmGold, true);

    int y = 132;
    for (const auto& section : view.pauseSections) {
        wrapped(section, {184, y, 912, view.largerText ? 42 : 32}, view.largerText ? 2 : 1, ink, 3);
        y += view.largerText ? 46 : 34;
    }
    if (!view.pauseOptions.empty()) {
        y = std::max(y + 8, 205);
        const int rowHeight = view.largerText ? 48 : 42;
        for (std::size_t i = 0; i < view.pauseOptions.size(); ++i) {
            const bool selected = i == view.pauseSelection;
            const UiRect row{184, y, 912, rowHeight - 4};
            fill(row, selected ? UiColor{95, 19, 24, 255} : UiColor{31, 28, 35, 244});
            outline(row, selected ? theme.warmGold : UiColor{85, 82, 90, 255}, selected ? 3 : 1);
            text((selected ? "> " : "  ") + view.pauseOptions[i], row.x + 18, row.y + 11,
                 view.largerText ? 2 : 1, ink);
            y += rowHeight;
        }
    }
    if (!view.saveStatus.empty()) {
        fill({365, 625, 550, 34}, {18, 17, 21, 255});
        outline({365, 625, 550, 34}, theme.warmGold, 2);
        centered(view.saveStatus, 640, 635, 1, ink);
    }
}

void PresentationRenderer::renderRuntime(const RuntimeView& view) {
    drawWorld(view);
    if (view.pauseVisible) {
        drawPause(view);
        return;
    }
    if (view.impactFlash > 0.0f) {
        const auto alpha = static_cast<Uint8>(std::clamp(view.impactFlash * 3.2f, 0.0f, .48f) * 255.0f);
        fill({0, 0, 1280, 720}, {255, 238, 205, alpha});
    }
    drawHud(view);
    if (!view.combatFeedback.empty() && !view.dialogueVisible) {
        const auto& theme = ui_.theme();
        const UiColor accent = view.finalHitFlash ? theme.fireRed : view.perfectBlockFlash ? theme.warmGold : theme.offWhite;
        fill({455, 226, 370, 58}, {10, 8, 12, 226});
        outline({455, 226, 370, 58}, accent, view.finalHitFlash ? 4 : 3);
        centered(view.combatFeedback, 640, 244, 2, accent);
    }
    drawDialogue(view);
}

void PresentationRenderer::renderModelReview(const RuntimeView& view) {
    drawWorld(view);
}

void PresentationRenderer::renderIdlePoseReview(int legacyFrame, float sampleTime) {
    for (int y = 0; y < 720; ++y) {
        const float amount = static_cast<float>(y) / 719.0f;
        fill({0, y, 1280, 1}, {
            static_cast<Uint8>(42 + amount * 25),
            static_cast<Uint8>(48 + amount * 31),
            static_cast<Uint8>(55 + amount * 34), 255});
    }
    fill({72, 55, 1136, 610}, {29, 33, 38, 255});
    outline({72, 55, 1136, 610}, {221, 225, 216, 255}, 3);

    WorldPresentationDefinition reviewStage;
    reviewStage.camera.yawDegrees = 0.0f;
    reviewStage.camera.fovDegrees = 31.0f;
    reviewStage.camera.baseDistance = 375.0f;
    reviewStage.camera.height = 94.0f;
    reviewStage.camera.targetHeight = 80.0f;
    reviewStage.camera.nearPlane = 8.0f;
    reviewStage.camera.farPlane = 900.0f;
    SoftwareWorldCanvas canvas(renderer_, reviewStage, 0.0f, 0.0f);
    const auto* model = characterModels_.find("rrvvfo");
    if (model && model->valid()) {
        auto binding = characters_.get("rrvvfo");
        binding.worldHeight = 158.0f;
        canvas.shadow({0.0f, 0.0f}, binding.worldHeight * .54f);
        canvas.model(*model, binding, {0.0f, 0.0f}, 0.0f, 0.0f, true, &playerAnimation_,
                     resolveRrvvfoFaceExpression(playerAnimation_.state(), playerAnimation_.time(), 0.0f));
        canvas.flush();
    }

    text("LEGACY IDLE SILHOUETTE", 100, 82, 3, {242, 239, 224, 255});
    text("KEY POSE " + std::to_string(legacyFrame), 100, 122, 2, {230, 46, 40, 255});
    std::ostringstream timing;
    timing.setf(std::ios::fixed);
    timing.precision(3);
    timing << sampleTime << " S / 0.750 S LOOP";
    text(timing.str(), 100, 151, 2, {185, 192, 188, 255});
    text("FRONT THREE-QUARTER REVIEW", 895, 624, 1, {185, 192, 188, 255});
}

void PresentationRenderer::renderAnimationReview(const std::string& label, float sampleTime, float duration) {
    for (int y = 0; y < 720; ++y) {
        const float amount = static_cast<float>(y) / 719.0f;
        fill({0, y, 1280, 1}, {
            static_cast<Uint8>(34 + amount * 21),
            static_cast<Uint8>(39 + amount * 27),
            static_cast<Uint8>(46 + amount * 31), 255});
    }
    fill({72, 55, 1136, 610}, {25, 29, 35, 255});
    outline({72, 55, 1136, 610}, {221, 225, 216, 255}, 3);

    WorldPresentationDefinition reviewStage;
    reviewStage.camera.yawDegrees = 0.0f;
    reviewStage.camera.fovDegrees = 31.0f;
    reviewStage.camera.baseDistance = 375.0f;
    reviewStage.camera.height = 94.0f;
    reviewStage.camera.targetHeight = 80.0f;
    reviewStage.camera.nearPlane = 8.0f;
    reviewStage.camera.farPlane = 900.0f;
    SoftwareWorldCanvas canvas(renderer_, reviewStage, 0.0f, 0.0f);
    const auto* model = characterModels_.find("rrvvfo");
    if (model && model->valid()) {
        auto binding = characters_.get("rrvvfo");
        binding.worldHeight = 158.0f;
        canvas.shadow({0.0f, 0.0f}, binding.worldHeight * .54f);
        canvas.model(*model, binding, {0.0f, 0.0f}, 0.0f, 0.0f, true, &playerAnimation_,
                     resolveRrvvfoFaceExpression(playerAnimation_.state(), playerAnimation_.time(), 0.0f));
        canvas.flush();
    }

    text("RRVVFO LEGACY-POSE TRANSLATION", 100, 82, 3, {242, 239, 224, 255});
    text(label, 100, 122, 2, {230, 46, 40, 255});
    std::ostringstream timing;
    timing.setf(std::ios::fixed);
    timing.precision(3);
    timing << sampleTime << " S / " << duration << " S";
    text(timing.str(), 100, 151, 2, {185, 192, 188, 255});
    text("DETERMINISTIC GOLDEN-SLICE REVIEW", 850, 624, 1, {185, 192, 188, 255});
}

void PresentationRenderer::renderLegacyComparison(const RuntimeView& view) {
    drawWorld(view);
    drawDialogue(view);
}

bool PresentationRenderer::writePpm(const std::string& path) const {
    constexpr int width = 1280;
    constexpr int height = 720;
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(width * height));
    if (SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_ARGB8888,
                             pixels.data(), width * static_cast<int>(sizeof(std::uint32_t))) != 0)
        return false;
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto pixel : pixels) {
        const char rgb[3]{
            static_cast<char>((pixel >> 16u) & 0xffu),
            static_cast<char>((pixel >> 8u) & 0xffu),
            static_cast<char>(pixel & 0xffu)
        };
        out.write(rgb, 3);
    }
    return static_cast<bool>(out);
}

} // namespace px::linux
