#pragma once
#include "common.h"

class RenderPass
{
public:
    RenderPass(
        SDL_GPUCommandBuffer* command_buffer,
        const SDL_GPUColorTargetInfo* color_targets,
        u32 num_color_targets,
        const SDL_GPUDepthStencilTargetInfo* depth_target
    ) : pass(SDL_BeginGPURenderPass(command_buffer, color_targets, num_color_targets, depth_target)),
        command_buffer(command_buffer) {}

    ~RenderPass()
    {
        SDL_EndGPURenderPass(pass);
    }

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    operator SDL_GPURenderPass*() const { return pass; }

    inline void set_viewport(const float width, const float height) const
    {
        SDL_SetGPUViewport(pass, &(SDL_GPUViewport) {
            .x = 0.0f,
            .y = 0.0f,
            .w = width,
            .h = height,
            .min_depth = 0.0f,
            .max_depth = 1.0f
        });
    }

    inline void push_view_projection(
        const glm::mat4& view,
        const glm::mat4& projection
    ) const
    {
        const glm::mat4 m[2] = { view, projection };
        SDL_PushGPUVertexUniformData(
            command_buffer,
            0,
            (void*)glm::value_ptr(m[0]),
            sizeof(float) * 32
        );
    }

    inline void push_model_matrix(const glm::mat4& model) const
    {
        SDL_PushGPUVertexUniformData(
            command_buffer,
            1,
            (void*)glm::value_ptr(model),
            sizeof(float) * 16
        );
    }

private:
    SDL_GPURenderPass* pass;
    SDL_GPUCommandBuffer* command_buffer;
};

