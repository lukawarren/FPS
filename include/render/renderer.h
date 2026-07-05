#pragma once
#include "common.h"
#include "world.h"
#include "render/quad.h"
#include "render/device.h"
#include "render/pipeline_factory.h"
#include "render/texture_manager.h"
#include "render/passes/shadow_pass.h"
#include "render/passes/depth_pass.h"
#include "render/passes/diffuse_pass.h"
#include "render/passes/sprite_pass.h"
#include "render/passes/bloom_pass.h"
#include "render/passes/composite_pass.h"
#include "render/debug_renderer.h"
#include "audio.h"
#include "model.h"

class Renderer
{
public:
    Renderer(const std::string& title, const u32 width, const u32 height, Audio& audio);
    ~Renderer();

    bool update();
    void render();

    float delta;

private:
    // Per-light data gathered once per frame and shared between the shadow
    // pass and the diffuse pass.
    struct LightingState
    {
        std::array<glm::mat4, QUALITY_SETTINGS.max_spotlights> matrices;
        DiffusePass::FragmentUniforms fragment_uniforms;
        u32 n_lights;
    };

    void init_imgui();

    LightingState collect_lights() const;

    static Quad* create_quad(Device& device);

    std::unordered_map<Model::ID, Model*> models;
    std::unordered_map<Decal::ID, Texture*> sprites;

    Device device;
    TextureManager texture_manager;
    PipelineFactory pipeline_factory;
    Quad* quad;

    ShadowPass shadow_pass;
    DepthPass depth_pass;
    DiffusePass diffuse_pass;
    SpritePass sprite_pass;
    BloomPass bloom_pass;
    CompositePass composite_pass;
    DebugRenderer debug_renderer;

    World* world;

    u64 last_time;
};
