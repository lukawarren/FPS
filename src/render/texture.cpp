#include "render/texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

Texture::Texture(
    const std::string& filename,
    const bool use_nearest_filtering,
    const bool is_srgb
)
{
    // Load from disk
    int channels;
    uint8_t* data = stbi_load(("../assets/textures/" + filename).c_str(), &width, &height, &channels, STBI_rgb);
    if (!data) throw std::runtime_error("failed to load texture " + filename);

    // Create and bind texture
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Upload data
    const auto format = GL_RGB;
    const auto formatInternal = is_srgb ? GL_SRGB : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, formatInternal, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, use_nearest_filtering ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, use_nearest_filtering ? GL_NEAREST : GL_LINEAR);

    // Texture wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Do anisotropic filtering (if we can)
    if (!use_nearest_filtering)
    {
        if (GLAD_GL_EXT_texture_filter_anisotropic)
        {
            float max_anisotropy;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anisotropy);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(max_anisotropy, 16.0f));
        }
        else std::cerr << "anisotropic filtering not supported" << std::endl;
    }

    // Unbind and free image from normal memory
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

Texture::Texture(
    const unsigned int width,
    const unsigned int height,
    const unsigned int internal_format,
    const unsigned int format,
    const unsigned int type,
    const bool use_nearest_filtering,
    const char* data
)
{
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, use_nearest_filtering ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, use_nearest_filtering ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set_max_mipmap_level(const int max_mipmap_level) const
{
    // Limit mip-mapping so sub-textures don't go smaller than 1x1
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_mipmap_level);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind(const unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
}

void Texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    glDeleteTextures(1, &texture_id);
}
