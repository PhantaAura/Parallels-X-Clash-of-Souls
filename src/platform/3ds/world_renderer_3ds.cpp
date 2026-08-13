#include "platform/3ds/world_renderer_3ds.hpp"
#include "core/camera_policy.hpp"

#include "world_shbin.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace px::platform3ds {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr std::size_t kMaximumFrameVertices = 65520;
constexpr std::size_t kMaximumStaticWorldVertices = 36000;
constexpr int kWorldCylinderSegments = 8;
constexpr int kCharacterCylinderSegments = 7;

PresentationColor scaled(PresentationColor color, float multiplier, float alpha = 1.0f) {
    color.r = std::min(1.0f, color.r * multiplier);
    color.g = std::min(1.0f, color.g * multiplier);
    color.b = std::min(1.0f, color.b * multiplier);
    color.a *= alpha;
    return color;
}

} // namespace

struct WorldRenderer3ds::Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct WorldRenderer3ds::Point3 {
    float x, y, z;
};

WorldRenderer3ds::WorldRenderer3ds() = default;
WorldRenderer3ds::~WorldRenderer3ds() { shutdown(); }

bool WorldRenderer3ds::init(std::string* error) {
    if (ready_) return true;

    shaderBinary_ = DVLB_ParseFile(const_cast<u32*>(reinterpret_cast<const u32*>(world_shbin)), world_shbin_size);
    if (!shaderBinary_) {
        if (error) *error = "Could not load the native 3DS world shader";
        return false;
    }
    shaderProgramInit(&shaderProgram_);
    shaderProgramSetVsh(&shaderProgram_, &shaderBinary_->DVLE[0]);
    projectionUniform_ = shaderInstanceGetUniformLocation(shaderProgram_.vertexShader, "projection");
    viewUniform_ = shaderInstanceGetUniformLocation(shaderProgram_.vertexShader, "view");
    if (projectionUniform_ < 0 || viewUniform_ < 0) {
        if (error) *error = "The native 3DS world shader is missing camera uniforms";
        shaderProgramFree(&shaderProgram_);
        DVLB_Free(shaderBinary_);
        shaderBinary_ = nullptr;
        return false;
    }

    gpuVertices_ = static_cast<Vertex*>(linearAlloc(sizeof(Vertex) * kMaximumFrameVertices));
    if (!gpuVertices_) {
        if (error) *error = "Not enough linear memory for the native 3DS world buffer";
        shaderProgramFree(&shaderProgram_);
        DVLB_Free(shaderBinary_);
        shaderBinary_ = nullptr;
        return false;
    }

    staticWorld_.reserve(28000);
    frameVertices_.reserve(kMaximumFrameVertices);
    skinnedPositions_.reserve(3000);
    activeVertexLimit_ = kMaximumFrameVertices;
    staticGpuDirty_ = true;
    ready_ = true;
    return true;
}

void WorldRenderer3ds::shutdown() {
    if (gpuVertices_) {
        linearFree(gpuVertices_);
        gpuVertices_ = nullptr;
    }
    if (shaderBinary_) {
        shaderProgramFree(&shaderProgram_);
        DVLB_Free(shaderBinary_);
        shaderBinary_ = nullptr;
    }
    staticWorld_.clear();
    frameVertices_.clear();
    skinnedPositions_.clear();
    staticWorldStageId_.clear();
    staticWorldDisabledBlockers_.clear();
    staticGpuDirty_ = true;
    activeVertexLimit_ = kMaximumFrameVertices;
    submittedVertexCount_ = 0;
    ready_ = false;
}

bool WorldRenderer3ds::staticWorldMatches(
    const WorldPresentationDefinition& stage,
    const std::vector<std::string>& disabledBlockers) const {
    return staticWorldStageId_ == stage.id && staticWorldDisabledBlockers_ == disabledBlockers;
}

void WorldRenderer3ds::uploadStaticWorldIfNeeded() {
    if (!staticGpuDirty_) return;
    const std::size_t count = std::min(staticWorld_.size(), kMaximumFrameVertices);
    if (count > 0) {
        const std::size_t bytes = count * sizeof(Vertex);
        std::memcpy(gpuVertices_, staticWorld_.data(), bytes);
        GSPGPU_FlushDataCache(gpuVertices_, bytes);
    }
    staticGpuDirty_ = false;
}

bool WorldRenderer3ds::blockerDisabled(const std::vector<std::string>& disabledBlockers,
                                       const std::string& id) const {
    return !id.empty() && std::find(disabledBlockers.begin(), disabledBlockers.end(), id) != disabledBlockers.end();
}

void WorldRenderer3ds::rebuildStaticWorld(const WorldPresentationDefinition& stage,
                                          const std::vector<std::string>& disabledBlockers) {
    if (staticWorldMatches(stage, disabledBlockers)) return;

    activeVertexLimit_ = kMaximumFrameVertices;
    staticWorld_.clear();
    // Gameplay-bearing geography is submitted first and can never be crowded
    // out by decoration.
    for (const auto& primitive : stage.primitives) {
        if (primitive.detail != PresentationDetailTier::Essential) continue;
        if (blockerDisabled(disabledBlockers, primitive.visibleWhileBlockerEnabled)) continue;
        appendPrimitive(staticWorld_, primitive);
    }
    // Then retain as much of the shared authored place-making layer as the
    // fixed Old 3DS vertex budget allows. Curves are still tessellated more
    // cheaply than desktop, so this is a visual reduction rather than a new
    // layout or a barren Essential-only map.
    for (const auto& primitive : stage.primitives) {
        if (primitive.detail != PresentationDetailTier::Full) continue;
        if (staticWorld_.size() >= kMaximumStaticWorldVertices) break;
        if (blockerDisabled(disabledBlockers, primitive.visibleWhileBlockerEnabled)) continue;
        appendPrimitive(staticWorld_, primitive);
    }
    if (staticWorld_.size() > kMaximumStaticWorldVertices)
        staticWorld_.resize(kMaximumStaticWorldVertices - kMaximumStaticWorldVertices % 3);
    staticWorldStageId_ = stage.id;
    staticWorldDisabledBlockers_ = disabledBlockers;
    staticGpuDirty_ = true;
}

WorldRenderer3ds::Point3 WorldRenderer3ds::transformPoint(Point3 local,
                                                           const PresentationTransform& transform) const {
    const float yaw = transform.yawDegrees * kPi / 180.0f;
    const float x = local.x * transform.scaleX;
    const float y = local.y * transform.scaleY;
    const float z = local.z * transform.scaleZ;
    return {
        transform.x + x * std::cos(yaw) - z * std::sin(yaw),
        transform.y + y,
        transform.z + x * std::sin(yaw) + z * std::cos(yaw)
    };
}

void WorldRenderer3ds::appendTriangle(std::vector<Vertex>& destination,
                                      Point3 a, Point3 b, Point3 c,
                                      PresentationColor color) {
    if (destination.size() + 3 > activeVertexLimit_) return;
    const Point3 ab{b.x-a.x,b.y-a.y,b.z-a.z};
    const Point3 ac{c.x-a.x,c.y-a.y,c.z-a.z};
    Point3 normal{ab.y*ac.z-ab.z*ac.y,ab.z*ac.x-ab.x*ac.z,ab.x*ac.y-ab.y*ac.x};
    const float length = std::sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
    if (length > .0001f) {
        normal.x /= length; normal.y /= length; normal.z /= length;
    }
    // Match the desktop renderer's three-band cel response without enabling
    // the more expensive PICA fragment-lighting path.
    constexpr float lightX = .391f, lightY = .869f, lightZ = .304f;
    const float diffuse = std::max(0.0f, normal.x*lightX + normal.y*lightY + normal.z*lightZ);
    const float light = diffuse > .62f ? 1.0f : diffuse > .20f ? .78f : .58f;
    color.r *= light; color.g *= light; color.b *= light;
    const auto push = [&](Point3 point) {
        destination.push_back({point.x,point.y,point.z,color.r,color.g,color.b,color.a});
    };
    push(a); push(b); push(c);
}

void WorldRenderer3ds::appendQuad(std::vector<Vertex>& destination,
                                  Point3 a, Point3 b, Point3 c, Point3 d,
                                  PresentationColor color) {
    appendTriangle(destination, a, b, c, color);
    appendTriangle(destination, a, c, d, color);
}

void WorldRenderer3ds::appendBox(std::vector<Vertex>& destination,
                                 const PresentationTransform& transform,
                                 PresentationColor color) {
    const std::array<Point3,8> p{{
        transformPoint({-.5f,-.5f,-.5f},transform), transformPoint({ .5f,-.5f,-.5f},transform),
        transformPoint({ .5f, .5f,-.5f},transform), transformPoint({-.5f, .5f,-.5f},transform),
        transformPoint({-.5f,-.5f, .5f},transform), transformPoint({ .5f,-.5f, .5f},transform),
        transformPoint({ .5f, .5f, .5f},transform), transformPoint({-.5f, .5f, .5f},transform)
    }};
    appendQuad(destination,p[4],p[5],p[6],p[7],color);
    appendQuad(destination,p[1],p[0],p[3],p[2],color);
    appendQuad(destination,p[0],p[4],p[7],p[3],color);
    appendQuad(destination,p[5],p[1],p[2],p[6],color);
    appendQuad(destination,p[3],p[7],p[6],p[2],color);
    appendQuad(destination,p[0],p[1],p[5],p[4],color);
}

void WorldRenderer3ds::appendCylinder(std::vector<Vertex>& destination,
                                      const PresentationTransform& transform,
                                      PresentationColor color,
                                      int segments) {
    const Point3 bottom = transformPoint({0,-.5f,0},transform);
    const Point3 top = transformPoint({0,.5f,0},transform);
    for (int i=0;i<segments;++i) {
        const float a0=2.0f*kPi*static_cast<float>(i)/segments;
        const float a1=2.0f*kPi*static_cast<float>(i+1)/segments;
        const Point3 b0=transformPoint({std::cos(a0)*.5f,-.5f,std::sin(a0)*.5f},transform);
        const Point3 b1=transformPoint({std::cos(a1)*.5f,-.5f,std::sin(a1)*.5f},transform);
        const Point3 t0=transformPoint({std::cos(a0)*.5f, .5f,std::sin(a0)*.5f},transform);
        const Point3 t1=transformPoint({std::cos(a1)*.5f, .5f,std::sin(a1)*.5f},transform);
        appendQuad(destination,b0,b1,t1,t0,color);
        appendTriangle(destination,top,t0,t1,color);
        appendTriangle(destination,bottom,b1,b0,color);
    }
}

void WorldRenderer3ds::appendCone(std::vector<Vertex>& destination,
                                  const PresentationTransform& transform,
                                  PresentationColor color,
                                  int segments) {
    const Point3 bottom=transformPoint({0,-.5f,0},transform);
    const Point3 top=transformPoint({0,.5f,0},transform);
    for(int i=0;i<segments;++i){
        const float a0=2.0f*kPi*static_cast<float>(i)/segments;
        const float a1=2.0f*kPi*static_cast<float>(i+1)/segments;
        const Point3 b0=transformPoint({std::cos(a0)*.5f,-.5f,std::sin(a0)*.5f},transform);
        const Point3 b1=transformPoint({std::cos(a1)*.5f,-.5f,std::sin(a1)*.5f},transform);
        appendTriangle(destination,b0,b1,top,color);
        appendTriangle(destination,bottom,b1,b0,color);
    }
}

void WorldRenderer3ds::appendPrimitive(std::vector<Vertex>& destination,
                                       const WorldPrimitiveDefinition& primitive) {
    switch (primitive.kind) {
        case PresentationPrimitiveKind::Box:
            appendBox(destination,primitive.transform,primitive.color); break;
        case PresentationPrimitiveKind::Cylinder:
            appendCylinder(destination,primitive.transform,primitive.color,kWorldCylinderSegments); break;
        case PresentationPrimitiveKind::Cone:
            appendCone(destination,primitive.transform,primitive.color,kWorldCylinderSegments); break;
    }
}

void WorldRenderer3ds::appendCharacterFallback(std::vector<Vertex>& destination,
                                               const CharacterPresentationDefinition& binding,
                                               Vec2 position,float worldY,float yawDegrees,bool focused) {
    const float h=binding.worldHeight;
    const auto primary=focused?scaled(binding.primaryColor,1.18f):binding.primaryColor;
    const auto secondary=binding.secondaryColor;
    const float yaw=yawDegrees*kPi/180.0f;
    const auto part=[&](float lx,float ly,float lz,float sx,float sy,float sz,
                        PresentationColor color,float localYaw=0.0f){
        const float wx=position.x+lx*std::cos(yaw)-lz*std::sin(yaw);
        const float wz=position.z+lx*std::sin(yaw)+lz*std::cos(yaw);
        appendBox(destination,{wx,worldY+ly,wz,sx,sy,sz,yawDegrees+localYaw},color);
    };
    if(binding.fallback==CharacterFallbackKind::ProceduralMentor){
        const PresentationColor coat{.79f,.85f,.87f,1};
        const PresentationColor shade{.54f,.64f,.69f,1};
        const PresentationColor skin{.54f,.39f,.27f,1};
        part(-h*.09f,h*.18f,0,h*.13f,h*.34f,h*.14f,secondary);
        part( h*.09f,h*.18f,0,h*.13f,h*.34f,h*.14f,secondary);
        part(0,h*.49f,0,h*.34f,h*.46f,h*.22f,coat);
        part(-h*.24f,h*.53f,0,h*.12f,h*.37f,h*.12f,shade,-12);
        part( h*.24f,h*.53f,0,h*.12f,h*.37f,h*.12f,shade,12);
        appendCylinder(destination,{position.x,worldY+h*.80f,position.z,h*.23f,h*.24f,h*.23f,yawDegrees},skin,kCharacterCylinderSegments);
        part(0,h*.94f,0,h*.30f,h*.13f,h*.28f,coat);
    }else if(binding.fallback==CharacterFallbackKind::LegacyTrainingDummy){
        part(0,h*.48f,0,h*.16f,h*.70f,h*.16f,primary);
        part(0,h*.60f,0,h*.72f,h*.12f,h*.12f,secondary);
        appendCylinder(destination,{position.x,worldY+h*.88f,position.z,h*.28f,h*.22f,h*.28f,yawDegrees},scaled(primary,1.08f),8);
        part(0,h*.08f,0,h*.56f,h*.10f,h*.56f,secondary);
    }else{
        const bool heavy=binding.fallback==CharacterFallbackKind::LegacyHeavy;
        const bool sturdy=binding.fallback==CharacterFallbackKind::LegacySturdy;
        const bool swift=binding.fallback==CharacterFallbackKind::LegacySwift;
        const float bodyWidth=h*(heavy?.44f:sturdy?.37f:swift?.26f:.31f);
        const float armX=h*(heavy?.28f:sturdy?.24f:swift?.18f:.20f);
        const float head=h*(heavy?.29f:sturdy?.25f:.23f);
        part(0,h*.49f,0,bodyWidth,h*(heavy?.48f:.43f),h*(heavy?.25f:.18f),primary);
        part(-h*.075f,h*.22f,0,h*(heavy?.15f:.12f),h*.34f,h*.12f,scaled(secondary,1.15f));
        part( h*.075f,h*.22f,0,h*(heavy?.15f:.12f),h*.34f,h*.12f,scaled(secondary,1.15f));
        part(-armX,h*.48f,0,h*(heavy?.14f:.10f),h*.38f,h*.10f,scaled(primary,.84f),-8);
        part( armX,h*.48f,0,h*(heavy?.14f:.10f),h*.38f,h*.10f,scaled(primary,.84f),8);
        appendCylinder(destination,{position.x,worldY+h*.82f,position.z,head,head,h*.24f,yawDegrees},scaled(primary,1.08f),kCharacterCylinderSegments);
        if(swift){
            part(0,h*.97f,0,h*.32f,h*.14f,h*.25f,secondary);
            part(-h*.17f,h*1.03f,0,h*.10f,h*.18f,h*.12f,secondary,-18);
        }else if(binding.fallback==CharacterFallbackKind::LegacyDisguise){
            part(0,h*.90f,0,h*.36f,h*.30f,h*.31f,secondary);
            part(0,h*.65f,h*.105f,h*.22f,h*.08f,h*.05f,scaled(primary,1.18f));
        }else if(binding.fallback==CharacterFallbackKind::LegacyCasual){
            part(0,h*.52f,h*.10f,bodyWidth*.72f,h*.34f,h*.04f,secondary);
        }else part(0,h*.96f,0,h*.29f,h*.12f,h*.27f,scaled(secondary,.75f));
    }
    if(focused)appendCylinder(destination,{position.x,worldY+2.0f,position.z,h*.62f,3.0f,h*.62f,0},
                              {1.0f,.76f,.24f,.42f},10);
}

bool WorldRenderer3ds::appendCharacterModel(std::vector<Vertex>& destination,
                                            const CharacterModelAsset* asset,
                                            const SkeletalAnimationPlayer* animation,
                                            RrvvfoFaceExpression faceExpression,
                                            const CharacterPresentationDefinition& binding,
                                            Vec2 position,float worldY,float yawDegrees,bool focused) {
    if(!asset||!asset->valid()||asset->height()<=.0001f)return false;
    const auto& minimum=asset->boundsMin();
    const auto& maximum=asset->boundsMax();
    const float centerX=(minimum[0]+maximum[0])*.5f;
    const float centerZ=(minimum[2]+maximum[2])*.5f;
    const float scale=binding.worldHeight/asset->height();
    const float yaw=(yawDegrees+binding.modelYawOffsetDegrees)*kPi/180.0f;
    const float cosine=std::cos(yaw),sine=std::sin(yaw);

    skinnedPositions_.resize(asset->vertices().size());
    for(std::size_t index=0;index<asset->vertices().size();++index){
        const auto& vertex=asset->vertices()[index];
        const auto p=animation&&animation->asset()==asset?animation->skinPosition(vertex):vertex.position;
        const float x=(p[0]-centerX)*scale;
        const float z=(p[2]-centerZ)*scale;
        skinnedPositions_[index]={position.x+x*cosine-z*sine,
                                  worldY+(p[1]-minimum[1])*scale,
                                  position.z+x*sine+z*cosine};
    }
    for(const auto& submesh:asset->submeshes()){
        const auto material=asset->materials()[submesh.materialIndex].color;
        PresentationColor color{material[0],material[1],material[2],material[3]};
        if(focused)color=scaled(color,1.10f);
        for(std::uint32_t offset=0;offset<submesh.indexCount;offset+=3){
            appendTriangle(destination,
                           skinnedPositions_[asset->indices()[submesh.firstIndex+offset]],
                           skinnedPositions_[asset->indices()[submesh.firstIndex+offset+1]],
                           skinnedPositions_[asset->indices()[submesh.firstIndex+offset+2]],color);
        }
    }
    if(animation&&animation->asset()==asset&&asset->joints().size()>kRrvvfoFaceJointIndex){
        const auto facePoint=[&](const std::array<float,3>& local){
            const auto p=animation->skinPosition(rrvvfoFaceVertex(local));
            const float x=(p[0]-centerX)*scale;
            const float z=(p[2]-centerZ)*scale;
            return Point3{position.x+x*cosine-z*sine,
                          worldY+(p[1]-minimum[1])*scale,
                          position.z+x*sine+z*cosine};
        };
        for(const auto& face:rrvvfoFaceTriangles(faceExpression)){
            appendTriangle(destination,facePoint(face.positions[0]),facePoint(face.positions[1]),facePoint(face.positions[2]),
                           {face.color[0],face.color[1],face.color[2],face.color[3]});
        }
    }
    if(focused)appendCylinder(destination,{position.x,worldY+2.0f,position.z,binding.worldHeight*.62f,3.0f,binding.worldHeight*.62f,0},
                              {1.0f,.76f,.24f,.42f},10);
    return true;
}

void WorldRenderer3ds::appendRuntimeActors(const WorldPresentationDefinition& stage,
                                           const RuntimeView& view,
                                           const CharacterPresentationRegistry& characters) {
    if(view.ambientActors.empty()){
        for(const auto& actor:stage.ambientActors){
            const auto& binding=characters.get(actor.characterId);
            appendCharacterFallback(frameVertices_,binding,{actor.transform.x,actor.transform.z},
                                    actor.transform.y,actor.transform.yawDegrees,false);
        }
        return;
    }
    for(const auto& actor:view.ambientActors){
        // Old 3DS keeps the same authored cast and positions, but does not spend
        // vertices on background extras that cannot affect the current shot.
        // Named/important contestants get the wider persistence radius.
        const float radius=actor.important?2200.0f:1250.0f;
        if(distance(actor.position,view.playerPosition)>radius)continue;
        appendCharacterFallback(frameVertices_,characters.get(actor.presentationId),actor.position,0.0f,
                                actor.yawDegrees,actor.interactable);
    }
}

void WorldRenderer3ds::appendRuntimeMarkers(const RuntimeView& view) {
    for(const auto& marker:view.worldMarkers){
        if(marker.kind=="swap-rock")appendBox(frameVertices_,{marker.position.x,24,marker.position.z,58,42,52,14},{.48f,.47f,.44f,1});
        else if(marker.kind=="fire-blast")appendCylinder(frameVertices_,{marker.position.x,72,marker.position.z,22,52,22,0},{1,.28f,.08f,.9f},8);
        else if(marker.kind=="object-swap-fx")appendCylinder(frameVertices_,{marker.position.x,5,marker.position.z,82,8,82,0},{1,.78f,.12f,.56f},10);
        else if(marker.kind=="lens-fx")appendCylinder(frameVertices_,{marker.position.x,92,marker.position.z,48,5,48,0},{.63f,.22f,.92f,.68f},10);
        else if(marker.kind=="energy-charge")appendCylinder(frameVertices_,{marker.position.x,78,marker.position.z,marker.complete?68.0f:52.0f,150,marker.complete?68.0f:52.0f,0},{.22f,.55f,1,.38f},10);
        else if(marker.kind=="energy-beam")appendBox(frameVertices_,{marker.position.x,82,marker.position.z,170,28,28,view.playerYawDegrees},{.22f,.65f,1,.82f});
        else if(marker.kind=="solar-weave")appendBox(frameVertices_,{marker.position.x,82,marker.position.z,190,38,38,view.playerYawDegrees},{.68f,.90f,1,.88f});
        else if(marker.kind=="fire-awakening")appendCylinder(frameVertices_,{marker.position.x,82,marker.position.z,72,164,72,0},{1,.25f,.06f,.34f},10);
        else if(marker.kind=="unstable-awakening")appendCylinder(frameVertices_,{marker.position.x,80,marker.position.z,marker.complete?48.0f:78.0f,158,marker.complete?48.0f:78.0f,0},{1,.32f,.08f,.28f},8);
        else if(marker.kind=="beam-clash"){
            appendCylinder(frameVertices_,{marker.position.x,84,marker.position.z,marker.complete?58.0f:44.0f,108,marker.complete?58.0f:44.0f,0},{.72f,.90f,1,.82f},8);
            appendCylinder(frameVertices_,{marker.position.x,84,marker.position.z,marker.complete?31.0f:40.0f,120,marker.complete?31.0f:40.0f,0},{1,.36f,.10f,.56f},8);
        }
        else if(marker.kind=="pursuit-lock"){
            appendCylinder(frameVertices_,{marker.position.x,10,marker.position.z,marker.complete?88.0f:68.0f,5,marker.complete?88.0f:68.0f,0},{1,.67f,.12f,.72f},12);
            appendBox(frameVertices_,{marker.position.x,94,marker.position.z,10,56,10,0},{1,.80f,.28f,.82f});
        }
        else if(marker.kind=="flow-cancel-fx")appendCylinder(frameVertices_,{marker.position.x,6,marker.position.z,96,6,96,0},{.42f,.94f,1,.72f},12);
        else if(marker.kind=="dash-dust")appendCylinder(frameVertices_,{marker.position.x,4,marker.position.z,54,4,36,0},{.74f,.69f,.59f,.48f},8);
        else if(marker.kind=="landing-dust")appendCylinder(frameVertices_,{marker.position.x,4,marker.position.z,76,4,76,0},{.74f,.69f,.59f,.52f},10);
        else if(marker.kind=="cliff-jump")appendBox(frameVertices_,{marker.position.x,18,marker.position.z,54,18,58,10},marker.complete?PresentationColor{.35f,.50f,.34f,1}:PresentationColor{.55f,.41f,.26f,1});
        else if(marker.kind=="swap-relay")appendCylinder(frameVertices_,{marker.position.x,42,marker.position.z,marker.complete?22.0f:34.0f,84,marker.complete?22.0f:34.0f,0},marker.complete?PresentationColor{.36f,.48f,.48f,.45f}:PresentationColor{.45f,.91f,1,.78f},8);
        else if(marker.kind=="transport-wheel")appendCylinder(frameVertices_,{marker.position.x,38,marker.position.z,72,24,72,90},{.24f,.20f,.16f,1},10);
        else if(marker.kind=="return-anchor")appendCylinder(frameVertices_,{marker.position.x,34,marker.position.z,44,68,44,0},{.45f,.91f,1,.82f},10);
        else if(marker.kind=="work-lane")appendBox(frameVertices_,{marker.position.x,12,marker.position.z,54,18,54,0},marker.complete?PresentationColor{.35f,.52f,.34f,.72f}:PresentationColor{.92f,.60f,.18f,.88f});
        else if(marker.kind=="blue-bell")appendCylinder(frameVertices_,{marker.position.x,48,marker.position.z,26,70,26,0},marker.complete?PresentationColor{.28f,.46f,.52f,.55f}:PresentationColor{.20f,.62f,1,.92f},10);
        else if(marker.kind=="bird")appendBox(frameVertices_,{marker.position.x,145,marker.position.z,24,5,11,12},{.91f,.94f,1,.82f});
        else if(marker.kind=="delivery-cart"||marker.kind=="parked-cart"){
            const float scale=marker.kind=="parked-cart"?1.35f:1.0f;
            appendBox(frameVertices_,{marker.position.x,24,marker.position.z,72*scale,38*scale,52*scale,0},{.61f,.42f,.25f,1});
            appendCylinder(frameVertices_,{marker.position.x-28*scale,8,marker.position.z+28*scale,24*scale,10,24*scale,90},{.16f,.13f,.11f,1},8);
            appendCylinder(frameVertices_,{marker.position.x+28*scale,8,marker.position.z+28*scale,24*scale,10,24*scale,90},{.16f,.13f,.11f,1},8);
        }
    }
}

void WorldRenderer3ds::appendPlayer(const RuntimeView& view,
                                    const CharacterPresentationDefinition& binding,
                                    const CharacterModelAsset* model,
                                    const SkeletalAnimationPlayer* animation,
                                    RrvvfoFaceExpression faceExpression) {
    const bool focused=view.dialogueVisible&&view.dialogueFocusActorId=="rrvvfo";
    if(!appendCharacterModel(frameVertices_,model,animation,faceExpression,binding,view.playerPosition,
                             view.playerHeight,view.playerYawDegrees,focused||!view.dialogueVisible))
        appendCharacterFallback(frameVertices_,binding,view.playerPosition,view.playerHeight,
                                view.playerYawDegrees,focused||!view.dialogueVisible);
    if(view.playerDashing)appendBox(frameVertices_,{view.playerPosition.x-48,view.playerHeight+68,view.playerPosition.z,72,20,28,view.playerYawDegrees},{.95f,.18f,.08f,.30f});
}

void WorldRenderer3ds::appendOpponent(const RuntimeView& view,
                                      const CharacterPresentationRegistry& characters) {
    if(!view.opponentVisible)return;
    const bool focused=view.dialogueVisible&&(view.dialogueFocusActorId==view.opponent.id||view.dialogueFocusActorId=="sage");
    appendCharacterFallback(frameVertices_,characters.get(view.opponent.id),view.opponentPosition,view.opponentHeight,
                            view.opponentYawDegrees,focused);
    if(view.opponentAttackTelegraphed)appendCylinder(frameVertices_,{view.opponentPosition.x,3,view.opponentPosition.z,180,5,180,0},{1,.18f,.08f,.45f},12);
}

void WorldRenderer3ds::configureCamera(const WorldPresentationDefinition& stage,
                                       const RuntimeView& view) {
    const float gameplayScale=view.opponentVisible?.92f:.80f;
    const float gameplayFovOffset=view.opponentVisible?-1.0f:-4.0f;
    const auto camera=resolveRuntimeCamera(stage,view,gameplayScale,gameplayFovOffset);
    const float yaw=camera.yawDegrees*kPi/180.0f;
    const C3D_FVec target=FVec4_New(camera.focus.x,camera.targetHeight,camera.focus.z,1.0f);
    const C3D_FVec eye=FVec4_New(camera.focus.x+std::sin(yaw)*camera.distance,
                                 camera.height,
                                 camera.focus.z+std::cos(yaw)*camera.distance,1.0f);
    const C3D_FVec up=FVec4_New(0,1,0,0);
    Mtx_PerspTilt(&projection_,C3D_AngleFromDegrees(camera.fovDegrees),C3D_AspectRatioTop,
                  camera.nearPlane,camera.farPlane,false);
    Mtx_LookAt(&view_,eye,target,up,false);
}

void WorldRenderer3ds::render(const WorldPresentationDefinition& stage,
                              const RuntimeView& view,
                              const std::vector<std::string>& disabledBlockers,
                              const CharacterPresentationRegistry& characters,
                              const CharacterModelAsset* playerModel,
                              const SkeletalAnimationPlayer* playerAnimation,
                              RrvvfoFaceExpression faceExpression) {
    if(!ready_)return;
    rebuildStaticWorld(stage,disabledBlockers);

    uploadStaticWorldIfNeeded();
    frameVertices_.clear();
    const std::size_t staticCount=std::min(staticWorld_.size(),kMaximumFrameVertices);
    activeVertexLimit_=kMaximumFrameVertices-staticCount;
    appendRuntimeActors(stage,view,characters);
    appendRuntimeMarkers(view);
    appendPlayer(view,characters.get("rrvvfo"),playerModel,playerAnimation,faceExpression);
    appendOpponent(view,characters);

    const std::size_t dynamicCount=std::min(frameVertices_.size(),activeVertexLimit_);
    submittedVertexCount_=staticCount+dynamicCount;
    if(submittedVertexCount_==0)return;
    if(dynamicCount>0){
        const std::size_t bytes=dynamicCount*sizeof(Vertex);
        std::memcpy(gpuVertices_+staticCount,frameVertices_.data(),bytes);
        GSPGPU_FlushDataCache(gpuVertices_+staticCount,bytes);
    }
    configureCamera(stage,view);

    submitVertices();
}

void WorldRenderer3ds::renderPlayerPreview(C3D_RenderTarget* target,
                                            const CharacterModelAsset& playerModel,
                                            const SkeletalAnimationPlayer& playerAnimation,
                                            RrvvfoFaceExpression faceExpression,
                                            const CharacterPresentationDefinition& binding,
                                            CharacterPreview3ds kind) {
    if(!ready_||!target||!playerModel.valid())return;
    frameVertices_.clear();
    activeVertexLimit_=kMaximumFrameVertices;
    const bool bust=kind==CharacterPreview3ds::DialogueBust;
    // Keep the full head and face inside the route-select character bay.  The
    // previous framing centered the torso under an opaque header, which made
    // the face appear detached from the model on the real 3DS screen.
    const Vec2 position{bust?-48.0f:165.0f,0.0f};
    if(!appendCharacterModel(frameVertices_,&playerModel,&playerAnimation,faceExpression,
                             binding,position,0.0f,0.0f,true))return;
    submittedVertexCount_=std::min(frameVertices_.size(),kMaximumFrameVertices);
    if(submittedVertexCount_==0)return;
    std::memcpy(gpuVertices_,frameVertices_.data(),submittedVertexCount_*sizeof(Vertex));
    GSPGPU_FlushDataCache(gpuVertices_,submittedVertexCount_*sizeof(Vertex));
    staticGpuDirty_=true;

    const float focusHeight=bust?131.0f:110.0f;
    const float distance=bust?185.0f:520.0f;
    const float fov=bust?34.0f:31.0f;
    Mtx_PerspTilt(&projection_,C3D_AngleFromDegrees(fov),
                  bust?C3D_AspectRatioBot:C3D_AspectRatioTop,2.0f,900.0f,false);
    const C3D_FVec targetPoint=FVec4_New(0.0f,focusHeight,0.0f,1.0f);
    const C3D_FVec eye=FVec4_New(0.0f,focusHeight,distance,1.0f);
    const C3D_FVec up=FVec4_New(0.0f,1.0f,0.0f,0.0f);
    Mtx_LookAt(&view_,eye,targetPoint,up,false);
    C3D_FrameDrawOn(target);
    submitVertices();
}

void WorldRenderer3ds::submitVertices() {
    if(submittedVertexCount_==0)return;

    C3D_BindProgram(&shaderProgram_);
    C3D_AttrInfo* attributes=C3D_GetAttrInfo();
    AttrInfo_Init(attributes);
    AttrInfo_AddLoader(attributes,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attributes,1,GPU_FLOAT,4);
    C3D_BufInfo* buffers=C3D_GetBufInfo();
    BufInfo_Init(buffers);
    BufInfo_Add(buffers,gpuVertices_,sizeof(Vertex),2,0x10);

    C3D_TexEnv* environment=C3D_GetTexEnv(0);
    C3D_TexEnvInit(environment);
    C3D_TexEnvSrc(environment,C3D_Both,GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(environment,C3D_Both,GPU_REPLACE);
    C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ONE_MINUS_SRC_ALPHA);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,projectionUniform_,&projection_);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,viewUniform_,&view_);
    C3D_DrawArrays(GPU_TRIANGLES,0,static_cast<int>(submittedVertexCount_));
}

} // namespace px::platform3ds
