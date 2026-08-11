#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>

#include "content/chapter_registry.hpp"
#include "content/adventure_registry.hpp"
#include "content/character_face.hpp"
#include "content/character_model_asset.hpp"
#include "content/skeletal_animation.hpp"
#include "content/character_presentation_registry.hpp"
#include "content/cutscene_registry.hpp"
#include "content/dialogue_registry.hpp"
#include "content/exploration_registry.hpp"
#include "content/map_registry.hpp"
#include "content/menu_registry.hpp"
#include "content/story_recap_registry.hpp"
#include "content/story_route_registry.hpp"
#include "content/training_registry.hpp"
#include "content/ui_presentation_registry.hpp"
#include "content/world_presentation_registry.hpp"
#include "core/input.hpp"
#include "core/menu_state.hpp"
#include "core/runtime.hpp"
#include "core/save.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct Vertex {
    float px, py, pz, pw;
    float nx, ny, nz, nw;
    float r, g, b, a;

    Vertex(float positionX, float positionY, float positionZ,
           float normalX, float normalY, float normalZ,
           float red, float green, float blue, float alpha)
        : px(positionX), py(positionY), pz(positionZ), pw(1.0f),
          nx(normalX), ny(normalY), nz(normalZ), nw(0.0f),
          r(red), g(green), b(blue), a(alpha) {}
};
static_assert(sizeof(Vertex) == sizeof(float) * 12, "Metal vertex layout must stay tightly matched");

struct SceneUniforms {
    matrix_float4x4 viewProjection;
    vector_float4 lightDirection;
    vector_float4 cameraPosition;
    vector_float4 fogParameters;
    vector_float4 fogColor;
};

struct Point3 {
    float x, y, z;
};

static NSString* macSavePath() {
    NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;
    return [[support stringByAppendingPathComponent:@"ParallelsX/ClashOfSouls"] stringByAppendingPathComponent:@"save-v5.txt"];
}

static NSString* legacyMacSavePath() {
    NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;
    return [[support stringByAppendingPathComponent:@"ParallelsX/ClashOfSouls"] stringByAppendingPathComponent:@"save-v4.txt"];
}

static NSString* olderLegacyMacSavePath() {
    NSString* support=NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,NSUserDomainMask,YES).firstObject;
    return [[support stringByAppendingPathComponent:@"ParallelsX/ClashOfSouls"] stringByAppendingPathComponent:@"save-v3.txt"];
}

static bool validMacSaveLocation(const px::SaveData& save) {
    if(save.story.chapterId.empty())return true;
    px::ChapterRegistry chapters;
    if(!chapters.has(save.story.chapterId))return false;
    const auto& chapter=chapters.get(save.story.chapterId);
    if(!save.story.checkpointId.empty()){
        const auto checkpoint=std::find_if(chapter.openingFlow.begin(),chapter.openingFlow.end(),[&](const px::SceneStep& step){return step.checkpointId==save.story.checkpointId;});
        if(checkpoint!=chapter.openingFlow.end())return true;
    }
    return save.story.sceneIndex<chapter.openingFlow.size();
}

static bool readMacSave(NSString* path, px::SaveData& out) {
    NSError* error=nil;
    NSString* encoded=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&error];
    if(!encoded)return false;
    try{
        out=px::SaveCodec::deserialize(std::string(encoded.UTF8String?:""));
        return validMacSaveLocation(out);
    }
    catch(const std::exception& exception){NSLog(@"Ignoring unreadable Parallels X save at %@: %s",path,exception.what());return false;}
}

static px::SaveData loadMacSave() {
    px::SaveData save;NSString* path=macSavePath();
    if(readMacSave(path,save))return save;
    if(readMacSave([path stringByAppendingString:@".bak"],save))return save;
    path=legacyMacSavePath();
    if(readMacSave(path,save))return save;
    if(readMacSave([path stringByAppendingString:@".bak"],save))return save;
    path=olderLegacyMacSavePath();
    if(readMacSave(path,save))return save;
    if(readMacSave([path stringByAppendingString:@".bak"],save))return save;
    return {};
}

static bool writeMacSave(const px::SaveData& save) {
    NSString* path=macSavePath();NSString* directory=[path stringByDeletingLastPathComponent];
    [NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil];
    NSString* backup=[path stringByAppendingString:@".bak"];
    px::SaveData validPrevious;
    if([NSFileManager.defaultManager fileExistsAtPath:path]&&readMacSave(path,validPrevious)){
        [NSFileManager.defaultManager removeItemAtPath:backup error:nil];
        [NSFileManager.defaultManager copyItemAtPath:path toPath:backup error:nil];
    }
    NSString* encoded=[NSString stringWithUTF8String:px::SaveCodec::serialize(save).c_str()];
    NSError* error=nil;const BOOL ok=[encoded writeToFile:path atomically:YES encoding:NSUTF8StringEncoding error:&error];
    if(!ok)NSLog(@"Could not save Parallels X: %@",error);
    return ok;
}

struct AppState {
    px::ChapterRegistry chapters;
    px::MapRegistry maps;
    px::CutsceneRegistry cutscenes;
    px::DialogueRegistry dialogue;
    px::ExplorationRegistry exploration;
    px::TrainingRegistry training;
    px::AdventureRegistry adventures;
    px::WorldPresentationRegistry worldPresentation;
    px::CharacterPresentationRegistry characterPresentation;
    px::CharacterModelRepository characterModels;
    px::SkeletalAnimationPlayer playerAnimation;
    px::MenuRegistry menuRegistry;
    px::StoryRouteRegistry storyRoutes;
    px::StoryRecapRegistry storyRecap;
    px::CombatManualRegistry combatManual;
    px::UiPresentationRegistry uiPresentation;
    px::SaveData save;
    px::RuntimeSession session;
    px::MenuState menu;
    px::InputState input;
    std::set<px::Action> held;
    bool gameplay{false};
    bool rrvvfoReady{false};
    std::chrono::steady_clock::time_point lastTick{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point lastPersist{lastTick};

    AppState()
        : save(loadMacSave()),
          session(chapters,maps,cutscenes,dialogue,exploration,training,adventures),
          menu(menuRegistry,storyRoutes,storyRecap,save) {
        menu.setReducedMotion(save.qol.reducedMotion);
        const auto& rrvvfo=characterPresentation.get("rrvvfo");
        NSString* bundled=[NSBundle.mainBundle pathForResource:@"rrvvfo-dev" ofType:@"pxskel"
                                                   inDirectory:@"assets/characters/rrvvfo"];
        std::string modelPath=bundled?std::string(bundled.UTF8String):rrvvfo.desktopCookedAsset;
        std::string error;
        rrvvfoReady=characterModels.load(rrvvfo.characterId,modelPath,&error);
        if(!rrvvfoReady)
            NSLog(@"Required Rrvvfo model failed to load: %s",error.c_str());
        playerAnimation.bind(characterModels.find(rrvvfo.characterId));
        playerAnimation.setState("idle");
    }

    void processMenuOutcome(){
        const auto outcome=menu.outcome();
        if(outcome==px::MenuOutcome::None)return;
        if(!rrvvfoReady&&(outcome==px::MenuOutcome::BeginStory||outcome==px::MenuOutcome::ContinueStory||
                          outcome==px::MenuOutcome::ReplayChapter||outcome==px::MenuOutcome::LaunchMode)){
            menu.clearOutcome();
            return;
        }
        if(outcome==px::MenuOutcome::BeginStory){session.startChapter("rrvvfo_ch1");session.setQolSettings(save.qol);}
        else if(outcome==px::MenuOutcome::ReplayChapter)session.startReplayChapter(save);
        else if(outcome==px::MenuOutcome::ContinueStory){
            if(save.story.chapterId.empty())session.startChapter("rrvvfo_ch1");else session.loadSnapshot(save);
        }
        else if(outcome==px::MenuOutcome::LaunchMode){
            const auto mode=menu.snapshot().selectedMode.id;
            if(mode==px::MenuModeId::ArenaBattle)session.startCpuFight();
            else if(mode==px::MenuModeId::Training)session.startStandaloneTraining();
            session.setQolSettings(save.qol);
        }
        if(outcome==px::MenuOutcome::BeginStory||outcome==px::MenuOutcome::ContinueStory||
           outcome==px::MenuOutcome::ReplayChapter||outcome==px::MenuOutcome::LaunchMode)gameplay=true;
        menu.clearOutcome();persist();
    }

    bool persist(){
        if(gameplay&&!session.standaloneMode()){
            auto runtimeSave=session.saveSnapshot();
            runtimeSave.frontend.discoveredStoryRoutes=save.frontend.discoveredStoryRoutes;
            runtimeSave.frontend.selectedStoryRoute=save.frontend.selectedStoryRoute;
            runtimeSave.frontend.storySoFarSection=save.frontend.storySoFarSection;
            runtimeSave.frontend.pendingStoryUnlocks=save.frontend.pendingStoryUnlocks;
            save=std::move(runtimeSave);
        } else if(gameplay) {
            save.qol=session.qolSettings();
        }
        const bool ok=writeMacSave(save);lastPersist=std::chrono::steady_clock::now();return ok;
    }
};

static NSString* ns(const std::string& value) {
    return [NSString stringWithUTF8String:value.c_str()];
}

static NSColor* color(const px::PresentationColor& c) {
    return [NSColor colorWithCalibratedRed:c.r green:c.g blue:c.b alpha:c.a];
}

static NSColor* uiColor(const px::UiColor& c) {
    return [NSColor colorWithCalibratedRed:c.r/255.0 green:c.g/255.0 blue:c.b/255.0 alpha:c.a/255.0];
}

static Point3 subtract(Point3 a, Point3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }

static Point3 cross(Point3 a, Point3 b) {
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

static Point3 normalize(Point3 v) {
    const float length = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (length <= 0.0001f) return {0.0f, 1.0f, 0.0f};
    return {v.x/length, v.y/length, v.z/length};
}

static void pushTriangle(std::vector<Vertex>& out, Point3 a, Point3 b, Point3 c,
                         const px::PresentationColor& colorValue) {
    const Point3 normal = normalize(cross(subtract(b,a), subtract(c,a)));
    out.push_back({a.x,a.y,a.z,normal.x,normal.y,normal.z,colorValue.r,colorValue.g,colorValue.b,colorValue.a});
    out.push_back({b.x,b.y,b.z,normal.x,normal.y,normal.z,colorValue.r,colorValue.g,colorValue.b,colorValue.a});
    out.push_back({c.x,c.y,c.z,normal.x,normal.y,normal.z,colorValue.r,colorValue.g,colorValue.b,colorValue.a});
}

static void pushQuad(std::vector<Vertex>& out, Point3 a, Point3 b, Point3 c, Point3 d,
                     const px::PresentationColor& colorValue) {
    pushTriangle(out,a,b,c,colorValue);
    pushTriangle(out,a,c,d,colorValue);
}

static Point3 transformPoint(Point3 local, const px::PresentationTransform& transform) {
    const float yaw = transform.yawDegrees * kPi / 180.0f;
    const float x = local.x * transform.scaleX;
    const float y = local.y * transform.scaleY;
    const float z = local.z * transform.scaleZ;
    return {
        transform.x + x*std::cos(yaw) - z*std::sin(yaw),
        transform.y + y,
        transform.z + x*std::sin(yaw) + z*std::cos(yaw)
    };
}

static void pushBox(std::vector<Vertex>& out, const px::PresentationTransform& transform,
                    const px::PresentationColor& colorValue) {
    const std::array<Point3,8> p{{
        transformPoint({-.5f,-.5f,-.5f},transform), transformPoint({ .5f,-.5f,-.5f},transform),
        transformPoint({ .5f, .5f,-.5f},transform), transformPoint({-.5f, .5f,-.5f},transform),
        transformPoint({-.5f,-.5f, .5f},transform), transformPoint({ .5f,-.5f, .5f},transform),
        transformPoint({ .5f, .5f, .5f},transform), transformPoint({-.5f, .5f, .5f},transform)
    }};
    pushQuad(out,p[4],p[5],p[6],p[7],colorValue);
    pushQuad(out,p[1],p[0],p[3],p[2],colorValue);
    pushQuad(out,p[0],p[4],p[7],p[3],colorValue);
    pushQuad(out,p[5],p[1],p[2],p[6],colorValue);
    pushQuad(out,p[3],p[7],p[6],p[2],colorValue);
    pushQuad(out,p[0],p[1],p[5],p[4],colorValue);
}

static void pushCylinder(std::vector<Vertex>& out, const px::PresentationTransform& transform,
                         const px::PresentationColor& colorValue, int segments = 10) {
    const Point3 bottomCenter = transformPoint({0.0f,-0.5f,0.0f},transform);
    const Point3 topCenter = transformPoint({0.0f,0.5f,0.0f},transform);
    for (int i=0; i<segments; ++i) {
        const float a0 = 2.0f*kPi*static_cast<float>(i)/static_cast<float>(segments);
        const float a1 = 2.0f*kPi*static_cast<float>(i+1)/static_cast<float>(segments);
        const Point3 b0=transformPoint({std::cos(a0)*.5f,-.5f,std::sin(a0)*.5f},transform);
        const Point3 b1=transformPoint({std::cos(a1)*.5f,-.5f,std::sin(a1)*.5f},transform);
        const Point3 t0=transformPoint({std::cos(a0)*.5f, .5f,std::sin(a0)*.5f},transform);
        const Point3 t1=transformPoint({std::cos(a1)*.5f, .5f,std::sin(a1)*.5f},transform);
        pushQuad(out,b0,b1,t1,t0,colorValue);
        pushTriangle(out,topCenter,t0,t1,colorValue);
        pushTriangle(out,bottomCenter,b1,b0,colorValue);
    }
}

static void pushCone(std::vector<Vertex>& out, const px::PresentationTransform& transform,
                     const px::PresentationColor& colorValue, int segments = 10) {
    const Point3 bottomCenter = transformPoint({0.0f,-0.5f,0.0f},transform);
    const Point3 top = transformPoint({0.0f,0.5f,0.0f},transform);
    for (int i=0; i<segments; ++i) {
        const float a0 = 2.0f*kPi*static_cast<float>(i)/static_cast<float>(segments);
        const float a1 = 2.0f*kPi*static_cast<float>(i+1)/static_cast<float>(segments);
        const Point3 b0=transformPoint({std::cos(a0)*.5f,-.5f,std::sin(a0)*.5f},transform);
        const Point3 b1=transformPoint({std::cos(a1)*.5f,-.5f,std::sin(a1)*.5f},transform);
        pushTriangle(out,b0,b1,top,colorValue);
        pushTriangle(out,bottomCenter,b1,b0,colorValue);
    }
}

static px::PresentationColor scaled(px::PresentationColor c, float multiplier, float alpha = 1.0f) {
    c.r = std::min(1.0f,c.r*multiplier);
    c.g = std::min(1.0f,c.g*multiplier);
    c.b = std::min(1.0f,c.b*multiplier);
    c.a *= alpha;
    return c;
}

// This renderer consumes CharacterPresentation definitions only. Replacing the
// fallback with a cooked mesh never changes RuntimeSession or chapter content.
static void pushCharacterFallback(std::vector<Vertex>& out,
                                  const px::CharacterPresentationDefinition& binding,
                                  px::Vec2 position, float worldY, float yawDegrees, bool focused) {
    const float h=binding.worldHeight;
    const auto primary=focused?scaled(binding.primaryColor,1.18f):binding.primaryColor;
    const auto secondary=binding.secondaryColor;
    const px::PresentationTransform base{position.x,worldY,position.z,1,1,1,yawDegrees};
    auto part=[&](float lx,float ly,float lz,float sx,float sy,float sz,
                  const px::PresentationColor& c,float localYaw=0.0f) {
        const float yaw=base.yawDegrees*kPi/180.0f;
        const float wx=base.x+lx*std::cos(yaw)-lz*std::sin(yaw);
        const float wz=base.z+lx*std::sin(yaw)+lz*std::cos(yaw);
        pushBox(out,{wx,base.y+ly,wz,sx,sy,sz,base.yawDegrees+localYaw},c);
    };
    if(binding.fallback==px::CharacterFallbackKind::ProceduralMentor){
        const px::PresentationColor coat{.79f,.85f,.87f,1};
        const px::PresentationColor coatShade{.54f,.64f,.69f,1};
        const px::PresentationColor skin{.54f,.39f,.27f,1};
        part(-h*.09f,h*.18f,0,h*.13f,h*.34f,h*.14f,secondary);
        part( h*.09f,h*.18f,0,h*.13f,h*.34f,h*.14f,secondary);
        part(0,h*.49f,0,h*.34f,h*.46f,h*.22f,coat);
        part(-h*.12f,h*.34f,h*.03f,h*.14f,h*.38f,h*.15f,coatShade,-4.0f);
        part( h*.12f,h*.34f,h*.03f,h*.14f,h*.38f,h*.15f,coatShade,4.0f);
        part(-h*.24f,h*.53f,0,h*.12f,h*.37f,h*.12f,coatShade,-12.0f);
        part( h*.24f,h*.53f,0,h*.12f,h*.37f,h*.12f,coatShade,12.0f);
        pushCylinder(out,{position.x,worldY+h*.80f,position.z,h*.23f,h*.24f,h*.23f,yawDegrees},skin,12);
        part(0,h*.94f,0,h*.30f,h*.13f,h*.28f,coat);
        part(-h*.13f,h*.99f,0,h*.11f,h*.16f,h*.14f,coatShade,-18.0f);
        part( h*.13f,h*.99f,0,h*.11f,h*.16f,h*.14f,coatShade,18.0f);
    }else{
        part(0,h*.49f,0,h*.30f,h*.43f,h*.18f,primary);
        part(0,h*.22f,-h*.055f,h*.12f,h*.34f,h*.12f,scaled(secondary,1.15f));
        part(0,h*.22f, h*.055f,h*.12f,h*.34f,h*.12f,scaled(secondary,1.15f));
        part(-h*.20f,h*.48f,0,h*.10f,h*.38f,h*.10f,scaled(primary,.84f),-8.0f);
        part( h*.20f,h*.48f,0,h*.10f,h*.38f,h*.10f,scaled(primary,.84f),8.0f);
        pushCylinder(out,{position.x,worldY+h*.82f,position.z,h*.24f,h*.24f,h*.24f,yawDegrees},scaled(primary,1.08f),12);
        part(0,h*.96f,0,h*.29f,h*.12f,h*.27f,scaled(secondary,.75f));
    }
    if (focused) {
        pushCylinder(out,{position.x,worldY+2.0f,position.z,h*.62f,3.0f,h*.62f,0.0f},
                     {1.0f,.76f,.24f,.42f},18);
    }
}

static bool pushCharacterModel(std::vector<Vertex>& out,
                               const px::CharacterModelAsset* asset,
                               const px::SkeletalAnimationPlayer* animation,
                               px::RrvvfoFaceExpression faceExpression,
                               const px::CharacterPresentationDefinition& binding,
                               px::Vec2 position,float worldY,float yawDegrees,bool focused) {
    if(!asset||!asset->valid()||asset->height()<=.0001f)return false;
    const auto& minimum=asset->boundsMin();const auto& maximum=asset->boundsMax();
    const float centerX=(minimum[0]+maximum[0])*.5f,centerZ=(minimum[2]+maximum[2])*.5f;
    const float scale=binding.worldHeight/asset->height();
    const float yaw=(yawDegrees+binding.modelYawOffsetDegrees)*kPi/180.0f;
    const float cosine=std::cos(yaw),sine=std::sin(yaw);
    auto point=[&](const px::CharacterModelVertex& vertex){
        const auto positionValue=animation&&animation->asset()==asset?animation->skinPosition(vertex):vertex.position;
        const float localX=(positionValue[0]-centerX)*scale;
        const float localZ=(positionValue[2]-centerZ)*scale;
        return Point3{position.x+localX*cosine-localZ*sine,
                      worldY+(positionValue[1]-minimum[1])*scale,
                      position.z+localX*sine+localZ*cosine};
    };
    for(const auto& submesh:asset->submeshes()){
        auto material=asset->materials()[submesh.materialIndex].color;
        px::PresentationColor colorValue{material[0],material[1],material[2],material[3]};
        if(focused)colorValue=scaled(colorValue,1.10f);
        for(std::uint32_t offset=0;offset<submesh.indexCount;offset+=3){
            const auto a=asset->indices()[submesh.firstIndex+offset];
            const auto b=asset->indices()[submesh.firstIndex+offset+1];
            const auto c=asset->indices()[submesh.firstIndex+offset+2];
            pushTriangle(out,point(asset->vertices()[a]),point(asset->vertices()[b]),point(asset->vertices()[c]),colorValue);
        }
    }
    if(animation&&animation->asset()==asset&&asset->joints().size()>px::kRrvvfoFaceJointIndex){
        for(const auto& face: px::rrvvfoFaceTriangles(faceExpression)){
            px::PresentationColor faceColor{face.color[0],face.color[1],face.color[2],face.color[3]};
            pushTriangle(out,
                         point(px::rrvvfoFaceVertex(face.positions[0])),
                         point(px::rrvvfoFaceVertex(face.positions[1])),
                         point(px::rrvvfoFaceVertex(face.positions[2])),faceColor);
        }
    }
    if(focused)pushCylinder(out,{position.x,worldY+2.0f,position.z,binding.worldHeight*.62f,3.0f,binding.worldHeight*.62f,0.0f},
                            {1.0f,.76f,.24f,.42f},18);
    return true;
}

static void pushPrimitive(std::vector<Vertex>& out, const px::WorldPrimitiveDefinition& primitive) {
    switch (primitive.kind) {
        case px::PresentationPrimitiveKind::Box: pushBox(out,primitive.transform,primitive.color); break;
        case px::PresentationPrimitiveKind::Cylinder: pushCylinder(out,primitive.transform,primitive.color); break;
        case px::PresentationPrimitiveKind::Cone: pushCone(out,primitive.transform,primitive.color); break;
    }
}

static matrix_float4x4 perspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
    const float yScale=1.0f/std::tan(fovDegrees*kPi/360.0f);
    const float xScale=yScale/aspect;
    const float zScale=farPlane/(nearPlane-farPlane);
    return (matrix_float4x4){
        (vector_float4){xScale,0,0,0},
        (vector_float4){0,yScale,0,0},
        (vector_float4){0,0,zScale,-1},
        (vector_float4){0,0,zScale*nearPlane,0}
    };
}

static matrix_float4x4 lookAt(vector_float3 eye, vector_float3 target) {
    const vector_float3 z=simd_normalize(eye-target);
    const vector_float3 x=simd_normalize(simd_cross((vector_float3){0,1,0},z));
    const vector_float3 y=simd_cross(z,x);
    return (matrix_float4x4){
        (vector_float4){x.x,y.x,z.x,0},
        (vector_float4){x.y,y.y,z.y,0},
        (vector_float4){x.z,y.z,z.z,0},
        (vector_float4){-simd_dot(x,eye),-simd_dot(y,eye),-simd_dot(z,eye),1}
    };
}

static std::array<px::Action,20> allActions() {
    return {px::Action::MoveUp,px::Action::MoveDown,px::Action::MoveLeft,px::Action::MoveRight,
            px::Action::Jump,px::Action::Light,px::Action::Heavy,px::Action::Launcher,px::Action::Grab,
            px::Action::Block,px::Action::Counter,px::Action::Breaker,px::Action::Charge,px::Action::Dash,
            px::Action::Interact,px::Action::Ability1,px::Action::Ability2,px::Action::Ability3,px::Action::Ability4,px::Action::Ability5};
}

static bool actionForKey(unsigned short keyCode, px::Action& out) {
    switch (keyCode) {
        case 13: out=px::Action::MoveUp; return true;
        case 1: out=px::Action::MoveDown; return true;
        case 0: out=px::Action::MoveLeft; return true;
        case 2: out=px::Action::MoveRight; return true;
        case 49: out=px::Action::Jump; return true;
        case 38: out=px::Action::Light; return true;
        case 40: out=px::Action::Heavy; return true;
        case 34: out=px::Action::Launcher; return true;
        case 32: out=px::Action::Grab; return true;
        case 37: out=px::Action::Block; return true;
        case 12: out=px::Action::Counter; return true;
        case 15: out=px::Action::Breaker; return true;
        case 8: out=px::Action::Charge; return true;
        case 56: case 60: out=px::Action::Dash; return true;
        case 14: out=px::Action::Interact; return true;
        case 18: out=px::Action::Ability1; return true;
        case 19: out=px::Action::Ability2; return true;
        case 20: out=px::Action::Ability3; return true;
        case 21: out=px::Action::Ability4; return true;
        case 23: out=px::Action::Ability5; return true;
        case 36: out=px::Action::Confirm; return true;
        case 53: out=px::Action::Pause; return true;
        default: return false;
    }
}

} // namespace

@interface PXGamePanel : NSView
@property(copy) NSString* kicker;
@property(copy) NSString* body;
@property(strong) NSColor* accent;
@property BOOL portraitVisible;
@property BOOL advanceVisible;
@property BOOL largerText;
@property BOOL highContrast;
@end

@implementation PXGamePanel
- (instancetype)initWithFrame:(NSRect)frame {
    self=[super initWithFrame:frame];
    if(self){self.wantsLayer=YES;self.layer.backgroundColor=NSColor.clearColor.CGColor;_kicker=@"";_body=@"";_accent=NSColor.systemRedColor;}
    return self;
}
- (BOOL)isOpaque{return NO;}
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    NSRect shadow=NSOffsetRect(NSInsetRect(self.bounds,7,7),5,-5);
    [NSColor.blackColor setFill];NSRectFill(shadow);
    NSRect bounds=NSInsetRect(self.bounds,7,7);
    [(self.highContrast?NSColor.whiteColor:[NSColor colorWithCalibratedRed:.96 green:.94 blue:.86 alpha:.97]) setFill];NSRectFill(bounds);
    [NSColor.blackColor setStroke];NSFrameRectWithWidth(bounds,self.highContrast?5:3);
    [self.accent setFill];NSRectFill(NSMakeRect(NSMinX(bounds),NSMaxY(bounds)-7,NSWidth(bounds),7));
    NSRectFill(NSMakeRect(NSMinX(bounds),NSMinY(bounds),7,NSHeight(bounds)));

    CGFloat left=18;
    if(self.portraitVisible){
        NSRect portrait=NSMakeRect(18,18,104,std::max(88.0,self.bounds.size.height-36));
        [[NSColor colorWithCalibratedRed:.05 green:.36 blue:.71 alpha:1] setFill];NSRectFill(portrait);
        [NSColor.blackColor setStroke];NSFrameRectWithWidth(portrait,3);
        const CGFloat cx=NSMidX(portrait), base=NSMinY(portrait)+20;
        NSBezierPath* body=[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(cx-23,base,46,52) xRadius:15 yRadius:15];
        [[self.accent colorWithAlphaComponent:.8] setFill];[body fill];
        NSBezierPath* head=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(cx-19,base+48,38,38)];[head fill];
        left=138;
    }
    NSMutableParagraphStyle* paragraph=[NSMutableParagraphStyle new];paragraph.lineBreakMode=NSLineBreakByWordWrapping;
    NSDictionary* kickerStyle=@{NSFontAttributeName:[NSFont systemFontOfSize:self.largerText?15:12 weight:NSFontWeightHeavy],NSForegroundColorAttributeName:self.accent};
    const BOOL compact=self.bounds.size.height<72;
    CGFloat bodySize=self.portraitVisible?19:(compact?13:15);if(self.largerText)bodySize+=3;
    NSDictionary* bodyStyle=@{NSFontAttributeName:[NSFont systemFontOfSize:bodySize weight:NSFontWeightSemibold],NSForegroundColorAttributeName:NSColor.blackColor,NSParagraphStyleAttributeName:paragraph};
    [self.kicker drawInRect:NSMakeRect(left,self.bounds.size.height-(compact?25:34),self.bounds.size.width-left-20,20) withAttributes:kickerStyle];
    [self.body drawInRect:NSMakeRect(left,compact?8:17,self.bounds.size.width-left-24,self.bounds.size.height-(compact?36:54)) withAttributes:bodyStyle];
    if(self.advanceVisible){
        const CGFloat x=self.bounds.size.width-26,y=17;
        NSBezierPath* arrow=[NSBezierPath new];[arrow moveToPoint:NSMakePoint(x-8,y+8)];[arrow lineToPoint:NSMakePoint(x+8,y+8)];[arrow lineToPoint:NSMakePoint(x,y)];[arrow closePath];
        [self.accent setFill];[arrow fill];
    }
}
@end

@interface PXCombatHUD : NSView
@property float playerHP;
@property float opponentHP;
@property float energy;
@property float guard;
@property BOOL showPlayerHealth;
@property BOOL showOpponentHealth;
@property BOOL showEnergy;
@property BOOL showGuard;
@end

@implementation PXCombatHUD
- (BOOL)isOpaque{return NO;}
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    auto drawBar=^(NSRect rect,float value,NSColor* fill){
        [NSColor.blackColor setFill];NSRectFill(rect);
        NSRect inner=NSInsetRect(rect,4,4);inner.size.width*=std::clamp(value,0.0f,1.0f);
        [fill setFill];NSRectFill(inner);
        [NSColor.whiteColor setStroke];NSFrameRectWithWidth(rect,2);
    };
    const CGFloat half=(self.bounds.size.width-36)/2;
    NSDictionary* attrs=@{NSFontAttributeName:[NSFont boldSystemFontOfSize:10],NSForegroundColorAttributeName:NSColor.whiteColor};
    if(self.showPlayerHealth){[@"HP" drawAtPoint:NSMakePoint(0,60) withAttributes:attrs];drawBar(NSMakeRect(0,34,half,24),self.playerHP,[NSColor colorWithCalibratedRed:.86 green:.12 blue:.10 alpha:1]);}
    if(self.showOpponentHealth){[@"SAGE" drawAtPoint:NSMakePoint(half+36,60) withAttributes:attrs];drawBar(NSMakeRect(half+36,34,half,24),self.opponentHP,[NSColor colorWithCalibratedRed:.72 green:.80 blue:.84 alpha:1]);}
    if(self.showEnergy){[@"ENERGY" drawAtPoint:NSMakePoint(0,22) withAttributes:attrs];drawBar(NSMakeRect(60,20,half-60,14),self.energy,[NSColor colorWithCalibratedRed:.12 green:.54 blue:.91 alpha:1]);}
    if(self.showGuard){[@"GUARD" drawAtPoint:NSMakePoint(0,3) withAttributes:attrs];drawBar(NSMakeRect(60,1,half-60,14),self.guard,[NSColor colorWithCalibratedRed:.90 green:.70 blue:.18 alpha:1]);}
}
@end

@interface PXFrontEndView : NSView
- (instancetype)initWithFrame:(NSRect)frame state:(AppState*)state;
@end

@implementation PXFrontEndView {
    AppState* _state;
}
- (instancetype)initWithFrame:(NSRect)frame state:(AppState*)state {
    self=[super initWithFrame:frame];if(self){_state=state;self.wantsLayer=YES;}return self;
}
- (BOOL)isFlipped{return YES;}
- (BOOL)isOpaque{return NO;}
- (void)label:(NSString*)value x:(CGFloat)x y:(CGFloat)y size:(CGFloat)size color:(NSColor*)ink weight:(NSFontWeight)weight {
    NSDictionary* style=@{NSFontAttributeName:[NSFont systemFontOfSize:size weight:weight],NSForegroundColorAttributeName:ink};
    [value drawAtPoint:NSMakePoint(x,y) withAttributes:style];
}
- (void)center:(NSString*)value y:(CGFloat)y size:(CGFloat)size color:(NSColor*)ink weight:(NSFontWeight)weight {
    NSDictionary* style=@{NSFontAttributeName:[NSFont systemFontOfSize:size weight:weight],NSForegroundColorAttributeName:ink};
    const CGFloat width=[value sizeWithAttributes:style].width;[value drawAtPoint:NSMakePoint(640-width/2,y) withAttributes:style];
}
- (void)fighterAt:(NSPoint)point accent:(NSColor*)accent mentor:(BOOL)mentor {
    [NSColor.blackColor setFill];NSBezierPath* edge=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(point.x-34,point.y-136,68,68)];[edge fill];
    [accent setFill];NSBezierPath* head=[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(point.x-28,point.y-130,56,56)];[head fill];
    if(mentor){[NSColor.blackColor setFill];NSRectFill(NSMakeRect(point.x-52,point.y-143,104,10));[accent setFill];NSRectFill(NSMakeRect(point.x-23,point.y-160,46,20));}
    [NSColor.blackColor setFill];NSRectFill(NSMakeRect(point.x-42,point.y-70,84,100));
    [accent setFill];NSRectFill(NSMakeRect(point.x-34,point.y-62,68,84));
    [NSColor.blackColor setStroke];NSBezierPath* limbs=[NSBezierPath new];limbs.lineWidth=13;
    [limbs moveToPoint:NSMakePoint(point.x-28,point.y-48)];[limbs lineToPoint:NSMakePoint(point.x-68,point.y+12)];
    [limbs moveToPoint:NSMakePoint(point.x+28,point.y-48)];[limbs lineToPoint:NSMakePoint(point.x+70,point.y+5)];
    [limbs moveToPoint:NSMakePoint(point.x-20,point.y+22)];[limbs lineToPoint:NSMakePoint(point.x-34,point.y+92)];
    [limbs moveToPoint:NSMakePoint(point.x+20,point.y+22)];[limbs lineToPoint:NSMakePoint(point.x+38,point.y+92)];[limbs stroke];
}
- (void)backdrop:(NSColor*)accent {
    const auto& theme=_state->uiPresentation.theme();[uiColor(theme.charcoal) setFill];NSRectFill(NSMakeRect(0,0,1280,720));
    NSColor* legacyBlue=uiColor(theme.panel);
    for(int i=0;i<8;++i){NSBezierPath* stripe=[NSBezierPath new];[stripe moveToPoint:NSMakePoint(-220+i*210,0)];[stripe lineToPoint:NSMakePoint(-145+i*210,0)];[stripe lineToPoint:NSMakePoint(65+i*210,720)];[stripe lineToPoint:NSMakePoint(-10+i*210,720)];[stripe closePath];[[legacyBlue colorWithAlphaComponent:.28] setFill];[stripe fill];}
    [uiColor(theme.warmGold) setFill];NSRectFill(NSMakeRect(0,0,1280,12));NSRectFill(NSMakeRect(0,708,1280,12));
    [accent setFill];NSRectFill(NSMakeRect(0,12,7,696));
    [[NSColor colorWithCalibratedRed:.02 green:.12 blue:.28 alpha:.70] setStroke];NSBezierPath* x=[NSBezierPath new];x.lineWidth=24;[x moveToPoint:NSMakePoint(800,120)];[x lineToPoint:NSMakePoint(1240,600)];[x moveToPoint:NSMakePoint(1240,120)];[x lineToPoint:NSMakePoint(800,600)];[x stroke];
}
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;[NSGraphicsContext saveGraphicsState];NSAffineTransform* scale=[NSAffineTransform transform];[scale scaleXBy:self.bounds.size.width/1280.0 yBy:self.bounds.size.height/720.0];[scale concat];
    const px::MenuSnapshot snapshot=_state->menu.snapshot();const auto& theme=_state->uiPresentation.theme();NSColor* red=uiColor(theme.crimson);NSColor* fire=uiColor(theme.fireRed);NSColor* gold=uiColor(theme.warmGold);NSColor* white=uiColor(theme.offWhite);NSColor* muted=uiColor(theme.mutedText);
    if(snapshot.screen==px::MenuScreen::Title){
        [self backdrop:red];[self center:@"PARALLELS X" y:188 size:92 color:white weight:NSFontWeightBlack];[self center:@"CLASH OF SOULS" y:316 size:42 color:gold weight:NSFontWeightHeavy];
        [NSColor.blackColor setFill];NSRectFill(NSMakeRect(338,444,604,68));[white setStroke];NSFrameRectWithWidth(NSMakeRect(338,444,604,68),3);
        [self center:@"PRESS ANY BUTTON" y:461 size:32 color:white weight:NSFontWeightBlack];[self center:@"KEYBOARD  /  CONTROLLER  /  PHYSICAL BUTTONS" y:548 size:18 color:muted weight:NSFontWeightSemibold];
    }else if(snapshot.screen==px::MenuScreen::ModeSelect){
        NSColor* accent=uiColor(px::UiPresentationRegistry::accent(theme,snapshot.selectedMode.accentId));[self backdrop:accent];
        const CGFloat motion=snapshot.transitionProgress*snapshot.transitionDirection*170.0;
        [self label:@"PARALLELS X  /  CLASH OF SOULS" x:56 y:34 size:18 color:accent weight:NSFontWeightBold];[self label:@"MODE SELECT" x:56 y:64 size:42 color:white weight:NSFontWeightBlack];
        [uiColor(theme.panel) setFill];NSRectFill(NSMakeRect(80,150,1120,360));[white setStroke];NSFrameRectWithWidth(NSMakeRect(80,150,1120,360),4);[gold setFill];NSRectFill(NSMakeRect(80,150,8,360));[self label:@"‹" x:62 y:278 size:90 color:gold weight:NSFontWeightBlack];[self label:@"›" x:1156 y:278 size:90 color:gold weight:NSFontWeightBlack];
        [self label:ns(snapshot.selectedMode.kicker) x:126+motion y:196 size:22 color:accent weight:NSFontWeightHeavy];[self label:ns(snapshot.selectedMode.label) x:120+motion y:234 size:snapshot.selectedMode.label.size()>14?48:64 color:white weight:NSFontWeightBlack];
        NSMutableParagraphStyle* p=[NSMutableParagraphStyle new];p.lineBreakMode=NSLineBreakByWordWrapping;NSDictionary* copy=@{NSFontAttributeName:[NSFont systemFontOfSize:18 weight:NSFontWeightSemibold],NSForegroundColorAttributeName:white,NSParagraphStyleAttributeName:p};[ns(snapshot.selectedMode.description) drawInRect:NSMakeRect(128,340,500,92) withAttributes:copy];
        if(snapshot.selectedMode.id==px::MenuModeId::Story){NSRect liveBay=NSMakeRect(730,156,458,348);NSRectFillUsingOperation(liveBay,NSCompositingOperationClear);[white setStroke];NSFrameRectWithWidth(liveBay,3);[self fighterAt:NSMakePoint(1080,408) accent:[NSColor colorWithCalibratedRed:.72 green:.78 blue:.82 alpha:1] mentor:YES];if(!_state->rrvvfoReady)[self label:@"RRVVFO MODEL REQUIRED" x:758 y:310 size:19 color:fire weight:NSFontWeightBlack];}
        else{[self fighterAt:NSMakePoint(866,390) accent:fire mentor:NO];[self fighterAt:NSMakePoint(1062,406) accent:[NSColor colorWithCalibratedRed:.72 green:.78 blue:.82 alpha:1] mentor:YES];}
        [self label:ns((snapshot.modeIndex+1<10?"0":"")+std::to_string(snapshot.modeIndex+1)+" / 10") x:128 y:455 size:22 color:accent weight:NSFontWeightHeavy];
        if(snapshot.selectedMode.id==px::MenuModeId::Story){NSRect box=NSMakeRect(432,535,416,58);[(snapshot.storySoFarSelected?gold:NSColor.blackColor) setFill];NSRectFill(box);[(snapshot.storySoFarSelected?white:accent) setStroke];NSFrameRectWithWidth(box,3);[self center:@"STORY SO FAR" y:548 size:26 color:snapshot.storySoFarSelected?NSColor.blackColor:white weight:NSFontWeightBlack];}
        else if(!snapshot.selectedMode.implemented){[accent setFill];NSRectFill(NSMakeRect(430,532,420,56));[self center:@"COMING LATER" y:546 size:27 color:NSColor.blackColor weight:NSFontWeightBlack];}
        [self label:@"LEFT / RIGHT  CHANGE MODE" x:70 y:662 size:18 color:muted weight:NSFontWeightSemibold];[self label:@"ENTER / A  CONFIRM" x:990 y:662 size:18 color:white weight:NSFontWeightBold];
    }else if(snapshot.screen==px::MenuScreen::StoryCharacterSelect||snapshot.screen==px::MenuScreen::StoryComingLater){
        NSColor* accent=uiColor(px::UiPresentationRegistry::accent(theme,snapshot.selectedRoute.accentId));[self backdrop:accent];[self label:@"PARALLELS X  /  STORY MODE" x:58 y:34 size:19 color:gold weight:NSFontWeightHeavy];[self label:@"ROUTE SELECT" x:58 y:68 size:42 color:white weight:NSFontWeightBlack];
        const CGFloat motion=snapshot.transitionProgress*snapshot.transitionDirection*150.0;NSRect card=NSMakeRect(84,146,690,362);[uiColor(theme.panel) setFill];NSRectFill(card);[white setStroke];NSFrameRectWithWidth(card,4);[gold setFill];NSRectFill(NSMakeRect(84,146,10,362));NSRectFill(NSMakeRect(84,146,690,6));[self label:@"NEW STORY" x:124 y:174 size:18 color:fire weight:NSFontWeightHeavy];[self label:ns(snapshot.selectedRoute.characterName) x:120+motion y:212 size:62 color:white weight:NSFontWeightBlack];[self label:ns(snapshot.selectedRoute.title) x:122+motion y:286 size:24 color:gold weight:NSFontWeightHeavy];NSMutableParagraphStyle* routeParagraph=[NSMutableParagraphStyle new];routeParagraph.lineBreakMode=NSLineBreakByWordWrapping;NSDictionary* routeCopy=@{NSFontAttributeName:[NSFont systemFontOfSize:18 weight:NSFontWeightSemibold],NSForegroundColorAttributeName:white,NSParagraphStyleAttributeName:routeParagraph};[ns(snapshot.selectedRoute.description) drawInRect:NSMakeRect(124,340,590,92) withAttributes:routeCopy];[self label:snapshot.selectedRoute.id=="rrvvfo"?@"LIVE REPAIRED MODEL + EXPRESSIVE FACE":@"ROUTE ART PENDING" x:124 y:464 size:16 color:muted weight:NSFontWeightBold];[self label:@"‹" x:38 y:282 size:92 color:gold weight:NSFontWeightBlack];[self label:@"›" x:1212 y:282 size:92 color:gold weight:NSFontWeightBlack];
        if(snapshot.selectedRoute.id=="rrvvfo"){NSRect liveBay=NSMakeRect(806,120,394,430);NSRectFillUsingOperation(liveBay,NSCompositingOperationClear);[white setStroke];NSFrameRectWithWidth(liveBay,3);if(!_state->rrvvfoReady)[self label:@"RRVVFO MODEL REQUIRED" x:840 y:328 size:20 color:fire weight:NSFontWeightBlack];}else{[self fighterAt:NSMakePoint(1004,416) accent:accent mentor:snapshot.selectedRoute.id=="bark"];}
        NSString* action=snapshot.screen==px::MenuScreen::StoryComingLater?@"STORY COMING LATER":ns(snapshot.routeActions.empty()?"STORY COMING LATER":snapshot.routeActions[std::min(snapshot.routeActionIndex,snapshot.routeActions.size()-1)]);[uiColor(theme.ember) setFill];NSRectFill(NSMakeRect(390,550,500,70));[NSColor.blackColor setStroke];NSFrameRectWithWidth(NSMakeRect(390,550,500,70),3);[gold setFill];NSRectFill(NSMakeRect(390,550,500,5));[self center:action y:567 size:28 color:NSColor.blackColor weight:NSFontWeightBlack];
    }else if(snapshot.screen==px::MenuScreen::StorySoFar){
        [self backdrop:gold];[self label:@"STORY MODE  /  STORY SO FAR" x:54 y:30 size:20 color:gold weight:NSFontWeightBold];[self label:ns(snapshot.recapSection.title) x:54 y:66 size:40 color:white weight:NSFontWeightBlack];
        [[NSColor colorWithCalibratedWhite:.04 alpha:.95] setFill];NSRectFill(NSMakeRect(54,120,760,480));[white setStroke];NSFrameRectWithWidth(NSMakeRect(54,120,760,480),4);[self fighterAt:NSMakePoint(320,390) accent:fire mentor:NO];[self fighterAt:NSMakePoint(570,400) accent:[NSColor colorWithCalibratedRed:.45 green:.31 blue:.54 alpha:1] mentor:NO];
        [[NSColor colorWithCalibratedWhite:.04 alpha:.95] setFill];NSRectFill(NSMakeRect(838,120,388,480));[gold setStroke];NSFrameRectWithWidth(NSMakeRect(838,120,388,480),4);[self label:ns("SECTION "+std::to_string(snapshot.recapSectionIndex+1)+" / 10") x:862 y:146 size:18 color:gold weight:NSFontWeightBold];[self label:ns(snapshot.recapFrame.caption) x:862 y:184 size:25 color:white weight:NSFontWeightHeavy];
        NSMutableParagraphStyle* p=[NSMutableParagraphStyle new];p.lineBreakMode=NSLineBreakByWordWrapping;NSDictionary* body=@{NSFontAttributeName:[NSFont systemFontOfSize:18 weight:NSFontWeightMedium],NSForegroundColorAttributeName:white,NSParagraphStyleAttributeName:p};[ns(snapshot.recapFrame.body) drawInRect:NSMakeRect(862,236,338,230) withAttributes:body];[self label:@"LEFT / RIGHT  PREVIOUS / NEXT" x:54 y:668 size:17 color:white weight:NSFontWeightBold];[self label:@"ESC / B  EXIT     X  SKIP" x:910 y:668 size:17 color:muted weight:NSFontWeightSemibold];
    }else{
        [self backdrop:gold];[self center:@"NEW STORY UNLOCKED" y:220 size:44 color:gold weight:NSFontWeightBlack];[self center:ns(snapshot.unlockRouteId) y:318 size:76 color:white weight:NSFontWeightBlack];
    }
    [NSGraphicsContext restoreGraphicsState];
}
@end

@interface PXManualView : NSView
- (void)updateWithRuntime:(const px::RuntimeView&)view;
@end

@implementation PXManualView {
    px::CombatManualPage _page;
    std::vector<std::string> _options;
    std::size_t _selection;
    std::size_t _pageIndex;
    std::size_t _pageCount;
}
- (BOOL)isFlipped{return YES;}
- (BOOL)isOpaque{return NO;}
- (void)updateWithRuntime:(const px::RuntimeView&)view {_page=view.trainingManualPage;_options=view.trainingManualOptions;_selection=view.trainingManualSelection;_pageIndex=view.trainingManualPageIndex;_pageCount=view.trainingManualPageCount;[self setNeedsDisplay:YES];}
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;[[NSColor colorWithCalibratedRed:.96 green:.92 blue:.80 alpha:.98] setFill];NSRectFill(self.bounds);[[NSColor colorWithCalibratedWhite:.04 alpha:1] setStroke];NSFrameRectWithWidth(NSInsetRect(self.bounds,5,5),5);
    NSDictionary* red=@{NSFontAttributeName:[NSFont systemFontOfSize:15 weight:NSFontWeightHeavy],NSForegroundColorAttributeName:[NSColor colorWithCalibratedRed:.68 green:.05 blue:.08 alpha:1]};NSDictionary* black=@{NSFontAttributeName:[NSFont systemFontOfSize:25 weight:NSFontWeightBlack],NSForegroundColorAttributeName:NSColor.blackColor};NSDictionary* copy=@{NSFontAttributeName:[NSFont systemFontOfSize:14 weight:NSFontWeightMedium],NSForegroundColorAttributeName:[NSColor colorWithCalibratedWhite:.16 alpha:1]};
    [@"THE SAGE'S COMBAT MANUAL" drawAtPoint:NSMakePoint(26,20) withAttributes:red];[[NSString stringWithFormat:@"%lu / %lu",(unsigned long)_pageIndex+1,(unsigned long)_pageCount] drawAtPoint:NSMakePoint(self.bounds.size.width-90,20) withAttributes:red];[ns(_page.title) drawAtPoint:NSMakePoint(26,54) withAttributes:black];[ns(_page.category+"  /  "+_page.kicker) drawAtPoint:NSMakePoint(28,91) withAttributes:red];[ns(_page.summary) drawInRect:NSMakeRect(28,116,self.bounds.size.width-56,48) withAttributes:copy];
    CGFloat y=174;for(const auto& entry:_page.entries){NSRect row=NSMakeRect(28,y,self.bounds.size.width-56,54);[[NSColor colorWithCalibratedRed:.90 green:.84 blue:.68 alpha:1] setFill];NSRectFill(row);[NSColor.blackColor setStroke];NSFrameRectWithWidth(row,1);[ns(entry.label) drawAtPoint:NSMakePoint(40,y+7) withAttributes:@{NSFontAttributeName:[NSFont systemFontOfSize:15 weight:NSFontWeightBold],NSForegroundColorAttributeName:NSColor.blackColor}];std::string prompts=entry.keyboardPrompt+(entry.keyboardPrompt.empty()||entry.controllerPrompt.empty()?"":"  /  ")+entry.controllerPrompt;[ns(prompts) drawAtPoint:NSMakePoint(40,y+31) withAttributes:red];[ns(entry.description) drawInRect:NSMakeRect(285,y+7,self.bounds.size.width-325,40) withAttributes:copy];y+=61;if(y>self.bounds.size.height-118)break;}
    const CGFloat optionWidth=(self.bounds.size.width-56)/std::max<std::size_t>(1,_options.size());for(std::size_t i=0;i<_options.size();++i){NSRect option=NSMakeRect(28+i*optionWidth,self.bounds.size.height-78,optionWidth-8,46);[(i==_selection?[NSColor colorWithCalibratedRed:.96 green:.69 blue:.18 alpha:1]:NSColor.blackColor) setFill];NSRectFill(option);NSDictionary* style=@{NSFontAttributeName:[NSFont systemFontOfSize:12 weight:NSFontWeightBold],NSForegroundColorAttributeName:i==_selection?NSColor.blackColor:NSColor.whiteColor};[ns(_options[i]) drawAtPoint:NSMakePoint(NSMinX(option)+12,NSMinY(option)+15) withAttributes:style];}
}
@end

@interface PXGameView : MTKView <MTKViewDelegate>
@end


@implementation PXGameView {
    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLDepthStencilState> _depthState;
    id<MTLBuffer> _vertexBuffer;
    AppState* _state;
    PXGamePanel* _objectivePanel;
    PXGamePanel* _dialoguePanel;
    PXGamePanel* _hotbarPanel;
    PXGamePanel* _interactionPanel;
    PXGamePanel* _noticePanel;
    PXCombatHUD* _combatHUD;
    PXFrontEndView* _frontEnd;
    PXManualView* _manualView;
    std::set<px::Action> _controllerHeld;
    std::vector<Vertex> _worldVertices;
    std::vector<Vertex> _previewVertices;
    bool _controllerWasConnected;
    bool _mouseLight;
    bool _mouseBlock;
}

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    self=[super initWithFrame:frameRect device:device];
    if(!self)return nil;
    _state=new AppState();
    _queue=[device newCommandQueue];
    _worldVertices.reserve(32000);
    _previewVertices.reserve(18000);
    self.delegate=self;self.preferredFramesPerSecond=60;self.enableSetNeedsDisplay=NO;self.paused=NO;
    self.colorPixelFormat=MTLPixelFormatBGRA8Unorm_sRGB;
    self.depthStencilPixelFormat=MTLPixelFormatDepth32Float;

    NSString* shader=[NSString stringWithUTF8String:R"(
#include <metal_stdlib>
using namespace metal;
struct V { float4 position; float4 normal; float4 color; };
struct U { float4x4 viewProjection; float4 lightDirection; float4 cameraPosition; float4 fogParameters; float4 fogColor; };
struct O { float4 position [[position]]; float3 normal; float4 color; float distanceToCamera; };
vertex O vmain(uint id [[vertex_id]],const device V* vertices [[buffer(0)]],constant U& u [[buffer(1)]]){
    O o;float3 world=vertices[id].position.xyz;o.position=u.viewProjection*float4(world,1);
    o.normal=vertices[id].normal.xyz;o.color=vertices[id].color;o.distanceToCamera=distance(world,u.cameraPosition.xyz);return o;
}
fragment float4 fmain(O in [[stage_in]],constant U& u [[buffer(1)]]){
    float diffuse=max(dot(normalize(in.normal),normalize(-u.lightDirection.xyz)),0.0);
    float light=diffuse>.62?1.0:(diffuse>.20?.78:.58);float fog=saturate((in.distanceToCamera-u.fogParameters.x)/max(1.0,u.fogParameters.y-u.fogParameters.x));
    float3 lit=in.color.rgb*light;return float4(mix(lit,u.fogColor.rgb,fog),in.color.a);
}
)" ];
    NSError* error=nil;id<MTLLibrary> library=[device newLibraryWithSource:shader options:nil error:&error];
    if(!library){NSLog(@"Metal shader compile failed: %@",error);return self;}
    MTLRenderPipelineDescriptor* desc=[MTLRenderPipelineDescriptor new];
    desc.vertexFunction=[library newFunctionWithName:@"vmain"];desc.fragmentFunction=[library newFunctionWithName:@"fmain"];
    desc.colorAttachments[0].pixelFormat=self.colorPixelFormat;desc.depthAttachmentPixelFormat=self.depthStencilPixelFormat;
    desc.colorAttachments[0].blendingEnabled=YES;desc.colorAttachments[0].sourceRGBBlendFactor=MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor=MTLBlendFactorOneMinusSourceAlpha;
    _pipeline=[device newRenderPipelineStateWithDescriptor:desc error:&error];
    if(!_pipeline)NSLog(@"Metal pipeline failed: %@",error);
    MTLDepthStencilDescriptor* depth=[MTLDepthStencilDescriptor new];depth.depthCompareFunction=MTLCompareFunctionLess;depth.depthWriteEnabled=YES;
    _depthState=[device newDepthStencilStateWithDescriptor:depth];
    [self installHUD];
    return self;
}

- (void)dealloc{_state->persist();delete _state;}
- (BOOL)acceptsFirstResponder{return YES;}

- (void)installHUD {
    _objectivePanel=[[PXGamePanel alloc]initWithFrame:NSZeroRect];_objectivePanel.accent=NSColor.systemYellowColor;
    _dialoguePanel=[[PXGamePanel alloc]initWithFrame:NSZeroRect];_dialoguePanel.portraitVisible=YES;_dialoguePanel.advanceVisible=YES;
    _hotbarPanel=[[PXGamePanel alloc]initWithFrame:NSZeroRect];_hotbarPanel.accent=NSColor.systemRedColor;
    _interactionPanel=[[PXGamePanel alloc]initWithFrame:NSZeroRect];_interactionPanel.accent=NSColor.systemYellowColor;
    _noticePanel=[[PXGamePanel alloc]initWithFrame:NSZeroRect];_noticePanel.accent=[NSColor colorWithCalibratedRed:.92 green:.30 blue:.12 alpha:1];
    _combatHUD=[[PXCombatHUD alloc]initWithFrame:NSZeroRect];_combatHUD.wantsLayer=YES;
    _manualView=[[PXManualView alloc]initWithFrame:NSZeroRect];_manualView.hidden=YES;
    _frontEnd=[[PXFrontEndView alloc]initWithFrame:self.bounds state:_state];
    [self addSubview:_objectivePanel];[self addSubview:_dialoguePanel];[self addSubview:_hotbarPanel];[self addSubview:_interactionPanel];[self addSubview:_noticePanel];[self addSubview:_combatHUD];[self addSubview:_manualView];[self addSubview:_frontEnd];
}

- (void)layout {
    [super layout];const CGFloat w=self.bounds.size.width,h=self.bounds.size.height;
    _objectivePanel.frame=NSMakeRect(22,h-144,std::min(620.0,w-44),120);
    _dialoguePanel.frame=NSMakeRect(28,26,w-56,166);
    _hotbarPanel.frame=NSMakeRect(28,22,w-56,76);
    _interactionPanel.frame=NSMakeRect(std::max(28.0,w-360),110,332,58);
    _noticePanel.frame=NSMakeRect(std::max(28.0,(w-460)/2),116,std::min(460.0,w-56),54);
    _combatHUD.frame=NSMakeRect(std::max(28.0,(w-820)/2),h-96,std::min(820.0,w-56),82);
    _manualView.frame=NSMakeRect(18,18,w-36,h-36);_frontEnd.frame=self.bounds;
}

- (void)keyDown:(NSEvent*)event{
    if(event.isARepeat)return;
    if(!_state->gameplay){
        NSString* characters=event.charactersIgnoringModifiers.lowercaseString;
        if(_state->menu.screen()==px::MenuScreen::StorySoFar&&[characters isEqualToString:@"x"])_state->menu.skipStorySoFar();
        else{px::Action action;if(actionForKey(event.keyCode,action))_state->menu.handle(action);}
        _state->processMenuOutcome();[_frontEnd setNeedsDisplay:YES];return;
    }
    px::Action action;if(actionForKey(event.keyCode,action))_state->held.insert(action);
}
- (void)keyUp:(NSEvent*)event{if(!_state->gameplay)return;px::Action action;if(actionForKey(event.keyCode,action))_state->held.erase(action);}
- (void)mouseDown:(NSEvent*)event{(void)event;if(!_state->gameplay){_state->menu.confirm();_state->processMenuOutcome();[_frontEnd setNeedsDisplay:YES];return;}_mouseLight=true;}
- (void)mouseUp:(NSEvent*)event{(void)event;_mouseLight=false;}
- (void)rightMouseDown:(NSEvent*)event{(void)event;_mouseBlock=true;}
- (void)rightMouseUp:(NSEvent*)event{(void)event;_mouseBlock=false;}

- (std::set<px::Action>)controllerActions {
    std::set<px::Action> actions;GCController* controller=GCController.controllers.firstObject;GCExtendedGamepad* pad=controller.extendedGamepad;if(!pad)return actions;
    if(!_state->gameplay){
        if(pad.dpad.up.isPressed||pad.leftThumbstick.yAxis.value>.55f)actions.insert(px::Action::MoveUp);
        if(pad.dpad.down.isPressed||pad.leftThumbstick.yAxis.value<-.55f)actions.insert(px::Action::MoveDown);
        if(pad.dpad.left.isPressed||pad.leftThumbstick.xAxis.value<-.55f)actions.insert(px::Action::MoveLeft);
        if(pad.dpad.right.isPressed||pad.leftThumbstick.xAxis.value>.55f)actions.insert(px::Action::MoveRight);
        if(pad.buttonA.isPressed)actions.insert(px::Action::Confirm);if(pad.buttonB.isPressed)actions.insert(px::Action::Cancel);
        if(pad.buttonMenu.isPressed)actions.insert(px::Action::Pause);return actions;
    }
    if(pad.leftThumbstick.yAxis.value>.32f)actions.insert(px::Action::MoveUp);
    if(pad.leftThumbstick.yAxis.value<-.32f)actions.insert(px::Action::MoveDown);
    if(pad.leftThumbstick.xAxis.value<-.32f)actions.insert(px::Action::MoveLeft);
    if(pad.leftThumbstick.xAxis.value>.32f)actions.insert(px::Action::MoveRight);
    const bool abilityLayer=pad.leftShoulder.isPressed;
    if(pad.buttonX.isPressed)actions.insert(abilityLayer?px::Action::Ability1:px::Action::Light);
    if(pad.buttonY.isPressed)actions.insert(abilityLayer?px::Action::Ability2:px::Action::Heavy);
    if(pad.buttonA.isPressed){actions.insert(abilityLayer?px::Action::Ability3:px::Action::Jump);if(!abilityLayer)actions.insert(px::Action::Confirm);}
    if(pad.buttonB.isPressed){actions.insert(abilityLayer?px::Action::Ability4:px::Action::Grab);if(!abilityLayer){actions.insert(px::Action::Interact);actions.insert(px::Action::Cancel);}}
    if(pad.dpad.up.isPressed)actions.insert(abilityLayer?px::Action::Ability5:px::Action::Dash);
    if(!abilityLayer&&pad.dpad.down.isPressed)actions.insert(px::Action::Charge);
    if(!abilityLayer&&pad.dpad.left.isPressed)actions.insert(px::Action::Counter);
    if(!abilityLayer&&pad.dpad.right.isPressed)actions.insert(px::Action::Breaker);
    if(pad.rightShoulder.isPressed)actions.insert(px::Action::Block);
    if(pad.rightTrigger.isPressed)actions.insert(px::Action::Launcher);
    if(pad.leftTrigger.isPressed&&!abilityLayer)actions.insert(px::Action::Dash);
    if(pad.buttonMenu.isPressed)actions.insert(px::Action::Pause);return actions;
}

- (void)syncInput {
    _state->input.beginFrame();
    for(auto action:allActions()){
        bool active=_state->held.count(action)!=0||_controllerHeld.count(action)!=0;
        if(action==px::Action::Light)active=active||_mouseLight;
        if(action==px::Action::Block)active=active||_mouseBlock;
        _state->input.set(action,active);
    }
    for(auto action:{px::Action::Confirm,px::Action::Pause,px::Action::Cancel})
        _state->input.set(action,_state->held.count(action)!=0||_controllerHeld.count(action)!=0);
}

- (void)updateHUD {
    _frontEnd.hidden=_state->gameplay;
    if(!_state->gameplay){_objectivePanel.hidden=YES;_dialoguePanel.hidden=YES;_hotbarPanel.hidden=YES;_interactionPanel.hidden=YES;_noticePanel.hidden=YES;_combatHUD.hidden=YES;_manualView.hidden=YES;[_frontEnd setNeedsDisplay:YES];return;}
    const auto& v=_state->session.view();
    for(PXGamePanel* panel in @[_objectivePanel,_dialoguePanel,_hotbarPanel,_interactionPanel,_noticePanel]){
        panel.largerText=v.largerText;panel.highContrast=v.highContrastHud;
    }
    _manualView.hidden=!v.trainingManualVisible;
    const CGFloat panelHeight=v.pauseVisible?std::min(620.0,self.bounds.size.height-48):v.trainingManualVisible?std::min(330.0,self.bounds.size.height-48):166.0;
    _dialoguePanel.frame=NSMakeRect(28,26,self.bounds.size.width-56,panelHeight);
    _objectivePanel.frame=NSMakeRect(22,v.mode==px::GameMode::ArenaCombat?self.bounds.size.height-226:self.bounds.size.height-144,std::min(620.0,self.bounds.size.width-44),120);
    _objectivePanel.hidden=v.dialogueVisible||v.trainingManualVisible||v.choiceVisible||v.qteVisible||v.pauseVisible;
    _objectivePanel.kicker=ns("CHAPTER 1 • "+v.currentArea);
    _objectivePanel.body=ns(v.objective+(v.objectiveDetail.empty()?"":"\n"+v.objectiveDetail));
    [_objectivePanel setNeedsDisplay:YES];

    if(v.pauseVisible){
        _dialoguePanel.hidden=NO;_hotbarPanel.hidden=YES;_dialoguePanel.portraitVisible=NO;_dialoguePanel.advanceVisible=NO;
        _dialoguePanel.accent=[NSColor colorWithCalibratedRed:.32 green:.66 blue:.82 alpha:1];
        _dialoguePanel.kicker=ns(v.pausePageTitle);
        std::string body;for(const auto& section:v.pauseSections){if(!body.empty())body+="\n";body+=section;}
        if(!v.pauseOptions.empty()){
            body+="\n\n";
            for(std::size_t i=0;i<v.pauseOptions.size();++i){if(i)body+="\n";body+=(i==v.pauseSelection?"▶ ":"  ")+v.pauseOptions[i];}
        }
        if(!v.saveStatus.empty())body+="\n\n"+v.saveStatus;
        _dialoguePanel.body=ns(body);[_dialoguePanel setNeedsDisplay:YES];
    }else if(v.trainingManualVisible){
        _dialoguePanel.hidden=YES;_hotbarPanel.hidden=YES;_manualView.hidden=NO;[_manualView updateWithRuntime:v];
    }else if(v.choiceVisible){
        _dialoguePanel.hidden=NO;_hotbarPanel.hidden=YES;_dialoguePanel.portraitVisible=NO;_dialoguePanel.advanceVisible=NO;
        _dialoguePanel.accent=[NSColor colorWithCalibratedRed:.86 green:.67 blue:.20 alpha:1];
        _dialoguePanel.kicker=ns(v.choiceTitle);
        std::string options;
        for(std::size_t i=0;i<v.choiceOptions.size();++i){if(i)options+="     ";options+=(i==v.choiceIndex?"▶ ":"  ")+v.choiceOptions[i];}
        _dialoguePanel.body=ns(options+"\n\nWASD SELECT  •  ENTER / E CONFIRM");[_dialoguePanel setNeedsDisplay:YES];
    }else if(v.qteVisible){
        _dialoguePanel.hidden=NO;_hotbarPanel.hidden=YES;_dialoguePanel.portraitVisible=NO;_dialoguePanel.advanceVisible=NO;
        _dialoguePanel.accent=[NSColor colorWithCalibratedRed:.95 green:.48 blue:.18 alpha:1];
        _dialoguePanel.kicker=ns(v.qteTitle+" • ATTEMPT "+std::to_string(v.qteAttempt));
        const auto label=[](px::Action action){return action==px::Action::MoveRight?"D":action==px::Action::MoveLeft?"A":"JUMP";};
        std::string sequence;
        for(std::size_t i=0;i<v.qteSequence.size();++i){if(i)sequence+="  •  ";sequence+=(i<v.qteIndex?"✓ ":i==v.qteIndex?"▶ ":"")+std::string(label(v.qteSequence[i]));}
        _dialoguePanel.body=ns(sequence+"\n\n"+std::to_string((int)std::ceil(v.qteSecondsRemaining))+" SECONDS");[_dialoguePanel setNeedsDisplay:YES];
    }else if(v.dialogueVisible){
        _dialoguePanel.hidden=NO;_hotbarPanel.hidden=YES;_dialoguePanel.portraitVisible=v.dialogueSpeaker!="RRVVFO";_dialoguePanel.advanceVisible=YES;
        _dialoguePanel.accent=v.dialogueSpeaker=="RRVVFO"?[NSColor colorWithCalibratedRed:.88 green:.10 blue:.08 alpha:1]:[NSColor colorWithCalibratedRed:.55 green:.72 blue:.80 alpha:1];
        _dialoguePanel.kicker=ns(v.dialogueSpeaker+" • "+v.dialogueExpression+" • "+std::to_string(v.dialogueIndex+1)+"/"+std::to_string(v.dialogueCount));
        _dialoguePanel.body=ns(v.dialogueText+"\n\nENTER / E  •  CONTINUE");[_dialoguePanel setNeedsDisplay:YES];
    }else{
        _manualView.hidden=YES;
        _dialoguePanel.hidden=YES;_hotbarPanel.hidden=!v.hotbarVisible;
        std::string slots;
        for(const auto& slot:v.hotbar){if(!slots.empty())slots+="     ";slots+="["+std::to_string(slot.displaySlot)+"] "+slot.label;}
        _hotbarPanel.kicker=v.mode==px::GameMode::ArenaCombat?@"COMBAT HOTBAR • CURRENT STORY TECHNIQUES":@"FIELD CONTROLS • CURRENT STORY TECHNIQUES";
        _hotbarPanel.body=ns(slots);[_hotbarPanel setNeedsDisplay:YES];
    }
    _interactionPanel.hidden=v.nearbyInteractionLabel.empty()||v.dialogueVisible||v.trainingManualVisible||v.choiceVisible||v.qteVisible||v.pauseVisible;
    _interactionPanel.kicker=@"INTERACT";_interactionPanel.body=ns("E • "+v.nearbyInteractionLabel);[_interactionPanel setNeedsDisplay:YES];
    const bool hasCombatFeedback=!v.combatFeedback.empty();
    _noticePanel.hidden=(v.gameplayNotice.empty()&&!hasCombatFeedback)||v.dialogueVisible||v.trainingManualVisible||v.choiceVisible||v.qteVisible||v.pauseVisible;
    _noticePanel.kicker=hasCombatFeedback?@"COMBAT IMPACT":(v.flowCancelReady?@"COMBAT TIMING":@"ROAD MOMENT");_noticePanel.body=ns(hasCombatFeedback?v.combatFeedback:v.gameplayNotice);[_noticePanel setNeedsDisplay:YES];
    _combatHUD.hidden=v.mode!=px::GameMode::ArenaCombat||v.dialogueVisible||v.trainingManualVisible||v.choiceVisible||v.qteVisible||v.pauseVisible;
    _combatHUD.playerHP=v.player.hp/std::max(1.0f,v.player.maxHp);_combatHUD.opponentHP=v.opponent.hp/std::max(1.0f,v.opponent.maxHp);_combatHUD.energy=v.player.energy/100.0f;_combatHUD.guard=v.player.guard/100.0f;
    _combatHUD.showPlayerHealth=v.showPlayerHealth;_combatHUD.showOpponentHealth=v.showOpponentHealth;_combatHUD.showEnergy=v.showEnergy;_combatHUD.showGuard=v.showGuard;
    [_combatHUD setNeedsDisplay:YES];
}

- (void)drawMenuRrvvfo:(id<MTLRenderCommandEncoder>)encoder {
    if(!_state->rrvvfoReady)return;
    const auto snapshot=_state->menu.snapshot();
    const bool routeScreen=snapshot.screen==px::MenuScreen::StoryCharacterSelect||snapshot.screen==px::MenuScreen::StoryComingLater;
    const bool storyMode=snapshot.screen==px::MenuScreen::ModeSelect&&snapshot.selectedMode.id==px::MenuModeId::Story;
    if((routeScreen&&snapshot.selectedRoute.id!="rrvvfo")||(!routeScreen&&!storyMode))return;

    auto& vertices=_previewVertices;vertices.clear();
    const auto& binding=_state->characterPresentation.get("rrvvfo");
    const px::Vec2 position={routeScreen?105.0f:70.0f,0.0f};
    if(!pushCharacterModel(vertices,_state->characterModels.find("rrvvfo"),&_state->playerAnimation,
                           px::RrvvfoFaceExpression::Confident,binding,position,0.0f,0.0f,true)||vertices.empty())return;

    const float aspect=std::max(.5f,static_cast<float>(self.drawableSize.width)/std::max(1.0f,static_cast<float>(self.drawableSize.height)));
    const vector_float3 target={0.0f,77.0f,0.0f};
    const vector_float3 eye={0.0f,82.0f,560.0f};
    SceneUniforms uniforms{};
    uniforms.viewProjection=simd_mul(perspective(31.0f,aspect,1.0f,2200.0f),lookAt(eye,target));
    uniforms.lightDirection=(vector_float4){-.45f,-1.0f,-.35f,0};
    uniforms.cameraPosition=(vector_float4){eye.x,eye.y,eye.z,1};
    uniforms.fogParameters=(vector_float4){1800.0f,2200.0f,0,0};
    uniforms.fogColor=(vector_float4){.07f,.36f,.71f,1};
    const NSUInteger bytes=vertices.size()*sizeof(Vertex);
    if(!_vertexBuffer||_vertexBuffer.length<bytes)_vertexBuffer=[self.device newBufferWithLength:std::max<NSUInteger>(bytes,1024*1024) options:MTLResourceStorageModeShared];
    std::memcpy(_vertexBuffer.contents,vertices.data(),bytes);
    [encoder setRenderPipelineState:_pipeline];[encoder setDepthStencilState:_depthState];
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];[encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
    [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertices.size()];
}

- (void)drawWorld:(id<MTLRenderCommandEncoder>)encoder {
    const auto& v=_state->session.view();
    if(!_state->worldPresentation.has(v.presentationStageId))return;
    const auto& stage=_state->worldPresentation.get(v.presentationStageId);
    auto& vertices=_worldVertices;vertices.clear();
    const auto blockerDisabled=[&](const std::string& id){const auto& disabled=_state->session.disabledBlockers();return std::find(disabled.begin(),disabled.end(),id)!=disabled.end();};
    for(const auto& primitive:stage.primitives){if(!primitive.visibleWhileBlockerEnabled.empty()&&blockerDisabled(primitive.visibleWhileBlockerEnabled))continue;pushPrimitive(vertices,primitive);}
    if(v.ambientActors.empty())for(const auto& actor:stage.ambientActors){
        const auto& binding=_state->characterPresentation.get(actor.characterId);
        pushCharacterFallback(vertices,binding,{actor.transform.x,actor.transform.z},actor.transform.y,actor.transform.yawDegrees,false);
    }else for(const auto& actor:v.ambientActors){
        const auto& binding=_state->characterPresentation.get(actor.presentationId);
        pushCharacterFallback(vertices,binding,actor.position,0.0f,actor.yawDegrees,actor.interactable);
    }
    for(const auto& marker:v.worldMarkers){
        if(marker.kind=="swap-rock"){
            pushBox(vertices,{marker.position.x,24.0f,marker.position.z,58.0f,42.0f,52.0f,14.0f},{.48f,.47f,.44f,1.0f});
        }else if(marker.kind=="fire-blast"){
            pushCylinder(vertices,{marker.position.x,72.0f,marker.position.z,22.0f,52.0f,22.0f,0.0f},{1.0f,.28f,.08f,.90f},14);
        }else if(marker.kind=="object-swap-fx"){
            pushCylinder(vertices,{marker.position.x,5.0f,marker.position.z,82.0f,8.0f,82.0f,0.0f},{.50f,.94f,1.0f,.52f},18);
        }else if(marker.kind=="lens-fx"){
            pushCylinder(vertices,{marker.position.x,92.0f,marker.position.z,48.0f,5.0f,48.0f,0.0f},{1.0f,.74f,.18f,.65f},18);
        }else if(marker.kind=="pursuit-lock"){
            pushCylinder(vertices,{marker.position.x,10.0f,marker.position.z,marker.complete?90.0f:70.0f,5.0f,marker.complete?90.0f:70.0f,0.0f},{1.0f,.67f,.12f,.72f},18);
            pushBox(vertices,{marker.position.x,94.0f,marker.position.z,10.0f,58.0f,10.0f,0.0f},{1.0f,.80f,.28f,.82f});
        }else if(marker.kind=="flow-cancel-fx"){
            pushCylinder(vertices,{marker.position.x,6.0f,marker.position.z,96.0f,6.0f,96.0f,0.0f},{.42f,.94f,1.0f,.72f},18);
        }else if(marker.kind=="dash-dust"){
            pushCylinder(vertices,{marker.position.x,4.0f,marker.position.z,54.0f,4.0f,36.0f,0.0f},{.74f,.69f,.59f,.48f},10);
        }else if(marker.kind=="landing-dust"){
            pushCylinder(vertices,{marker.position.x,4.0f,marker.position.z,76.0f,4.0f,76.0f,0.0f},{.74f,.69f,.59f,.52f},12);
        }else if(marker.kind=="cliff-jump"){
            pushBox(vertices,{marker.position.x,18.0f,marker.position.z,54.0f,18.0f,58.0f,10.0f},marker.complete?px::PresentationColor{.35f,.50f,.34f,1}:px::PresentationColor{.55f,.41f,.26f,1});
        }else if(marker.kind=="swap-relay"){
            pushCylinder(vertices,{marker.position.x,42.0f,marker.position.z,marker.complete?22.0f:34.0f,84.0f,marker.complete?22.0f:34.0f,0.0f},marker.complete?px::PresentationColor{.36f,.48f,.48f,.45f}:px::PresentationColor{.45f,.91f,1.0f,.78f},14);
        }else if(marker.kind=="transport-wheel"){
            pushCylinder(vertices,{marker.position.x,38.0f,marker.position.z,72.0f,24.0f,72.0f,90.0f},{.24f,.20f,.16f,1.0f},16);
        }else if(marker.kind=="return-anchor"){
            pushCylinder(vertices,{marker.position.x,34.0f,marker.position.z,44.0f,68.0f,44.0f,0.0f},{.45f,.91f,1.0f,.82f},14);
        }else if(marker.kind=="work-lane"){
            pushBox(vertices,{marker.position.x,12.0f,marker.position.z,54.0f,18.0f,54.0f,0.0f},marker.complete?px::PresentationColor{.35f,.52f,.34f,.72f}:px::PresentationColor{.92f,.60f,.18f,.88f});
        }else if(marker.kind=="blue-bell"){
            pushCylinder(vertices,{marker.position.x,48.0f,marker.position.z,26.0f,70.0f,26.0f,0.0f},marker.complete?px::PresentationColor{.28f,.46f,.52f,.55f}:px::PresentationColor{.20f,.62f,1.0f,.92f},12);
        }else if(marker.kind=="bird"){
            pushBox(vertices,{marker.position.x,145.0f,marker.position.z,24.0f,5.0f,11.0f,12.0f},{.91f,.94f,1.0f,.82f});
        }else if(marker.kind=="delivery-cart"||marker.kind=="parked-cart"){
            const float scale=marker.kind=="parked-cart"?1.35f:1.0f;
            pushBox(vertices,{marker.position.x,24.0f,marker.position.z,72.0f*scale,38.0f*scale,52.0f*scale,0.0f},{.61f,.42f,.25f,1.0f});
            pushCylinder(vertices,{marker.position.x-28.0f*scale,8.0f,marker.position.z+28.0f*scale,24.0f*scale,10.0f,24.0f*scale,90.0f},{.16f,.13f,.11f,1.0f},12);
            pushCylinder(vertices,{marker.position.x+28.0f*scale,8.0f,marker.position.z+28.0f*scale,24.0f*scale,10.0f,24.0f*scale,90.0f},{.16f,.13f,.11f,1.0f},12);
        }
    }
    const bool playerFocused=v.dialogueVisible&&v.dialogueFocusActorId=="rrvvfo";
    const bool opponentFocused=v.dialogueVisible&&v.dialogueFocusActorId=="sage";
    const bool playerSpeaking=v.dialogueVisible&&(v.dialogueFocusActorId=="rrvvfo"||
        v.dialoguePortraitId=="rrvvfo"||v.dialogueSpeaker=="RRVVFO");
    const auto faceExpression=px::resolveRrvvfoFaceExpression(
        v.playerAnimation,_state->playerAnimation.time(),v.playerFaceSeconds,
        v.dialogueExpression,playerSpeaking);
    const auto& rrvvfoBinding=_state->characterPresentation.get("rrvvfo");
    pushCharacterModel(vertices,_state->characterModels.find("rrvvfo"),&_state->playerAnimation,faceExpression,rrvvfoBinding,
                       v.playerPosition,v.playerHeight,v.playerYawDegrees,playerFocused||!v.dialogueVisible);
    if(v.playerDashing){
        pushBox(vertices,{v.playerPosition.x-48.0f,v.playerHeight+68.0f,v.playerPosition.z,72.0f,20.0f,28.0f,v.playerYawDegrees},{.95f,.18f,.08f,.30f});
    }
    if(v.opponentVisible){
        pushCharacterFallback(vertices,_state->characterPresentation.get("sage"),v.opponentPosition,v.opponentHeight,v.opponentYawDegrees,opponentFocused);
        if(v.opponentAttackTelegraphed)pushCylinder(vertices,{v.opponentPosition.x,3.0f,v.opponentPosition.z,180.0f,5.0f,180.0f,0.0f},{1.0f,.18f,.08f,.45f},20);
    }
    if(vertices.empty())return;

    const float aspect=std::max(.5f,static_cast<float>(self.drawableSize.width)/std::max(1.0f,static_cast<float>(self.drawableSize.height)));
    float requestedFocusX=stage.camera.focusCenterX,requestedFocusZ=stage.camera.focusCenterZ;
    if(stage.camera.followPlayer){
        requestedFocusX=v.playerPosition.x;requestedFocusZ=v.playerPosition.z;
        if(v.opponentVisible){requestedFocusX=(v.playerPosition.x+v.opponentPosition.x)*.5f;requestedFocusZ=(v.playerPosition.z+v.opponentPosition.z)*.5f;}
    }
    const float shakeSign=std::sin((v.playerPosition.x+v.playerPosition.z)*.017f)>=0.0f?1.0f:-1.0f;const float shake=std::min(8.0f,v.cameraImpulse*1.15f)*shakeSign;
    const float focusX=std::clamp(requestedFocusX+shake,stage.camera.focusCenterX-stage.camera.focusClampX,stage.camera.focusCenterX+stage.camera.focusClampX);
    const float focusZ=std::clamp(requestedFocusZ-shake*.45f,stage.camera.focusCenterZ-stage.camera.focusClampZ,stage.camera.focusCenterZ+stage.camera.focusClampZ);
    const float yaw=stage.camera.yawDegrees*kPi/180.0f;
    const vector_float3 target={(float)focusX,stage.camera.targetHeight,(float)focusZ};
    const vector_float3 eye={focusX+std::sin(yaw)*stage.camera.baseDistance,stage.camera.height,focusZ+std::cos(yaw)*stage.camera.baseDistance};
    SceneUniforms uniforms{};uniforms.viewProjection=simd_mul(perspective(stage.camera.fovDegrees,aspect,stage.camera.nearPlane,stage.camera.farPlane),lookAt(eye,target));
    uniforms.lightDirection=(vector_float4){-.45f,-1.0f,-.35f,0};uniforms.cameraPosition=(vector_float4){eye.x,eye.y,eye.z,1};
    uniforms.fogParameters=(vector_float4){stage.fogNear,stage.fogFar,0,0};uniforms.fogColor=(vector_float4){stage.fogColor.r,stage.fogColor.g,stage.fogColor.b,1};
    const NSUInteger bytes=vertices.size()*sizeof(Vertex);
    if(!_vertexBuffer||_vertexBuffer.length<bytes){_vertexBuffer=[self.device newBufferWithLength:std::max<NSUInteger>(bytes,1024*1024) options:MTLResourceStorageModeShared];}
    std::memcpy(_vertexBuffer.contents,vertices.data(),bytes);
    [encoder setRenderPipelineState:_pipeline];[encoder setDepthStencilState:_depthState];
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];[encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
    [encoder setFragmentBytes:&uniforms length:sizeof(uniforms) atIndex:1];
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertices.size()];
}

- (void)drawInMTKView:(MTKView*)view {
    (void)view;if(!_pipeline||!self.currentDrawable)return;
    const auto now=std::chrono::steady_clock::now();float dt=std::chrono::duration<float>(now-_state->lastTick).count();_state->lastTick=now;
    if(dt<=0||dt>.1f)dt=1.0f/60.0f;
    const bool controllerConnected=GCController.controllers.firstObject.extendedGamepad!=nil;
    auto controller=[self controllerActions];
    if(_state->gameplay&&_controllerWasConnected&&!controllerConnected&&!_state->session.view().pauseVisible)controller.insert(px::Action::Pause);
    _controllerWasConnected=controllerConnected;
    if(!_state->gameplay){for(auto action:controller)if(!_controllerHeld.count(action))_state->menu.handle(action);_controllerHeld=controller;_state->menu.tick(dt);if(_state->playerAnimation.state()!="idle")_state->playerAnimation.setState("idle");_state->playerAnimation.setPlaybackSpeed(1.0f);_state->playerAnimation.update(dt);_state->processMenuOutcome();}
    else{
        _controllerHeld=controller;[self syncInput];_state->session.tick(_state->input,dt);
        if(_state->session.consumeManualSaveRequest())_state->session.notifyManualSaveResult(_state->persist());
        if(_state->session.consumeReturnToTitleRequest()){
            _state->persist();_state->gameplay=false;_state->menu.openTitle();
        }
        const auto& runtimeView=_state->session.view();if(_state->playerAnimation.state()!=runtimeView.playerAnimation)_state->playerAnimation.setState(runtimeView.playerAnimation);_state->playerAnimation.setPlaybackSpeed(runtimeView.playerAnimationSpeed);if(runtimeView.hitFreezeSeconds<=0.0f)_state->playerAnimation.update(dt);
        if(_state->session.canManualSave()&&!runtimeView.pauseVisible&&std::chrono::duration<float>(now-_state->lastPersist).count()>2.0f)_state->persist();
    }
    [self updateHUD];const auto& displayView=_state->session.view();
    if(_state->gameplay&&_state->worldPresentation.has(displayView.presentationStageId)){
        const auto& stage=_state->worldPresentation.get(displayView.presentationStageId);
        self.clearColor=MTLClearColorMake(stage.clearColor.r,stage.clearColor.g,stage.clearColor.b,stage.clearColor.a);
    }else self.clearColor=MTLClearColorMake(.98,.72,.12,1.0);
    MTLRenderPassDescriptor* pass=self.currentRenderPassDescriptor;if(!pass)return;
    pass.depthAttachment.clearDepth=1.0;pass.depthAttachment.loadAction=MTLLoadActionClear;pass.depthAttachment.storeAction=MTLStoreActionDontCare;
    id<MTLCommandBuffer> command=[_queue commandBuffer];id<MTLRenderCommandEncoder> encoder=[command renderCommandEncoderWithDescriptor:pass];
    if(_state->gameplay)[self drawWorld:encoder];else [self drawMenuRrvvfo:encoder];[encoder endEncoding];[command presentDrawable:self.currentDrawable];[command commit];
}
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size{(void)view;(void)size;}
@end

@interface PXAppDelegate : NSObject <NSApplicationDelegate>
@property(strong)NSWindow* window;
@end

@implementation PXAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;id<MTLDevice> device=MTLCreateSystemDefaultDevice();
    if(!device){NSAlert* alert=[NSAlert new];alert.messageText=@"Metal is unavailable on this Mac.";[alert runModal];[NSApp terminate:nil];return;}
    NSRect frame=NSMakeRect(0,0,1280,720);self.window=[[NSWindow alloc]initWithContentRect:frame styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
    self.window.title=@"Parallels X: Clash of Souls 3.0R — 0.4H GOLDEN GATE QOL";self.window.minSize=NSMakeSize(960,540);
    PXGameView* gameView=[[PXGameView alloc]initWithFrame:frame device:device];self.window.contentView=gameView;[self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:gameView];[NSApp activateIgnoringOtherApps:YES];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender{(void)sender;return YES;}
@end

int main(int argc,const char* argv[]){(void)argc;(void)argv;@autoreleasepool{NSApplication* app=NSApplication.sharedApplication;app.activationPolicy=NSApplicationActivationPolicyRegular;NSMenu* menuBar=[NSMenu new];NSMenuItem* appMenuItem=[NSMenuItem new];[menuBar addItem:appMenuItem];app.mainMenu=menuBar;NSMenu* appMenu=[NSMenu new];[appMenu addItemWithTitle:@"Quit Parallels X" action:@selector(terminate:) keyEquivalent:@"q"];appMenuItem.submenu=appMenu;PXAppDelegate* delegate=[PXAppDelegate new];app.delegate=delegate;[app run];}return 0;}
