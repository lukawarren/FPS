#include "render/device.h"
#include "io.h"

Device::Device(const std::string& title, u32 width, u32 height)
{
    // Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        throw std::runtime_error(
            "Failed to initialise SDL: " + std::string(SDL_GetError())
        );

    if (!SDL_ShaderCross_Init())
        throw std::runtime_error("Failed to initialise SDL_ShaderCross: " + std::string(SDL_GetError()));

#ifdef __APPLE__
    if (is_debug())
    {
        SDL_setenv_unsafe("MTL_HUD_ENABLED", "1", true);
        SDL_setenv_unsafe("MTL_DEBUG_LAYER", "1", true);
    #if 0
        SDL_setenv_unsafe("MTL_SHADER_VALIDATION", "1", true);
    #endif
    }
#endif

    device = SDL_CreateGPUDevice(get_shader_format(), is_debug(), get_backend());
    if (!device)
        throw std::runtime_error(
            "Failed to create GPU device: " + std::string(SDL_GetError())
        );

    // Fetch true width and height after window creation (e.g. for high-DPI)
    int w, h;
    window = new Window(title, width, height, device);
    SDL_GetWindowSizeInPixels(window->get_window(), &w, &h);
    this->swapchain_width = w;
    this->swapchain_height = h;
    swapchain_format = SDL_GetGPUSwapchainTextureFormat(device, window->get_window());
}

Device::~Device()
{
    SDL_DestroyGPUDevice(device);
    SDL_ShaderCross_Quit();
}

SDL_GPUCommandBuffer* Device::acquire_command_buffer() const
{
    return SDL_AcquireGPUCommandBuffer(device);
}

void Device::submit_command_buffer(SDL_GPUCommandBuffer* buffer) const
{
    SDL_SubmitGPUCommandBuffer(buffer);
}

std::optional<SDL_GPUTexture*> Device::get_swapchain_texture(SDL_GPUCommandBuffer* command_buffer)
{
    SDL_GPUTexture* texture;
    u32 width, height;
    SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window->get_window(), &texture, &width, &height);
    if (texture == nullptr)
        return std::nullopt;

    SDL_GPUTextureFormat format = SDL_GetGPUSwapchainTextureFormat(device, window->get_window());

    if (width != swapchain_width || height != swapchain_height || format != swapchain_format)
        swapchain_did_change = true;
    else
        swapchain_did_change = false;

    swapchain_width = width;
    swapchain_height = height;
    swapchain_format = format;

    return { texture };
}

bool Device::did_swapchain_format_change() const
{
    return swapchain_did_change;
}

SDL_GPUShader* Device::compile_shader(
    const std::string& path,
    const SDL_ShaderCross_ShaderStage stage
) const
{
    u8* source = io_read_file(SHADER_ROOT + path);

    SDL_ShaderCross_HLSL_Info hlsl_info = {};
    hlsl_info.source = (const char*)source;
    hlsl_info.entrypoint = "main";
    hlsl_info.include_dir = SHADER_ROOT.c_str();
    hlsl_info.defines = NULL;
    hlsl_info.shader_stage = stage;
    hlsl_info.props = 0;

    size_t bytecode_size;
    void* bytecode = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &bytecode_size);
    delete[] source;

    if (bytecode == NULL)
        throw std::runtime_error("Failed to compile shader " + std::string(path) + ": " + std::string(SDL_GetError()));

    SDL_ShaderCross_SPIRV_Info spirv_info = {};
    spirv_info.bytecode = (const u8*)bytecode;
    spirv_info.bytecode_size = bytecode_size;
    spirv_info.entrypoint = "main";
    spirv_info.shader_stage = stage;
    spirv_info.props = 0;

    SDL_ShaderCross_GraphicsShaderMetadata* reflect_info = SDL_ShaderCross_ReflectGraphicsSPIRV(
        (const u8*)bytecode,
        bytecode_size,
        0
    );

    if (reflect_info == NULL)
    {
        SDL_free(bytecode);
        throw std::runtime_error("Failed to reflect shader " + std::string(path) + ": " + std::string(SDL_GetError()));
    }

    SDL_GPUShader* shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
        device,
        &spirv_info,
        &reflect_info->resource_info,
        0
    );

    if (shader == NULL)
    {
        SDL_free(bytecode);
        SDL_free(reflect_info);
        throw std::runtime_error("Failed to compile shader " + std::string(path) + " from SPIRV: " + std::string(SDL_GetError()));
    }

    SDL_free(bytecode);
    SDL_free(reflect_info);
    return shader;
}

void Device::wait_for_idle() const
{
    SDL_WaitForGPUIdle(device);
}

bool Device::should_close() const
{
    return window->should_close();
}

SDL_GPUShaderFormat Device::get_shader_format()
{
#ifdef __APPLE__
    return SDL_GPU_SHADERFORMAT_MSL;
#else
    return SDL_GPU_SHADERFORMAT_SPIRV;
#endif
}

const char* Device::get_backend()
{
#ifdef __APPLE__
    return "metal";
#else
    return "vulkan";
#endif
}

bool Device::is_debug()
{
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}
