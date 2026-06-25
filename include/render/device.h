#pragma once
#include "common.h"
#include "window.h"

class Device
{
public:
    Device(const std::string& title, u32 width, u32 height);
    ~Device();

    SDL_GPUCommandBuffer* acquire_command_buffer() const;
    void submit_command_buffer(SDL_GPUCommandBuffer* buffer) const;

    std::optional<SDL_GPUTexture*> get_swapchain_texture(SDL_GPUCommandBuffer* command_buffer);
    bool did_swapchain_format_change() const;

    SDL_GPUShader* compile_shader(
        const std::string& path,
        const SDL_ShaderCross_ShaderStage stage
    ) const;

    void wait_for_idle() const;
    bool should_close() const;

    SDL_GPUDevice* device;
    u32 swapchain_width, swapchain_height;
    SDL_GPUTextureFormat swapchain_format;
    Window* window;

private:
    bool swapchain_did_change = false;

    SDL_GPUShaderFormat get_shader_format();
    const char* get_backend();
    bool is_debug();
};