#pragma once
#include "pch.h"

// Macros to help with verbosity
#define SHADER_UNIFORM(x) void Shader::set_uniform(const std::string& name, x)
#define DEFINE_SHADER_UNIFORM(x) void set_uniform(const std::string& name, x)
#define NAME get_uniform_location(name)

class Shader
{
public:
    Shader(const std::string& filename, bool include_geometry_shader = false);
    Shader(const Shader&) = delete;
    ~Shader();

    void bind() const;
    void unbind() const;

    DEFINE_SHADER_UNIFORM(const glm::mat4& matrix);
    DEFINE_SHADER_UNIFORM(const glm::vec4& vector);
    DEFINE_SHADER_UNIFORM(const glm::vec3& vector);
    DEFINE_SHADER_UNIFORM(const glm::vec2& vector);
    DEFINE_SHADER_UNIFORM(const float value);
    DEFINE_SHADER_UNIFORM(const int value);

private:
    void handle_error(auto f, auto f2, const unsigned int id,
        const unsigned int type, const std::string& error, const std::string& filename);

    int get_uniform_location(const std::string& name);

    // OpenGL state
    unsigned int program;
    std::unordered_map<std::string, int> uniforms;
};

// Shader classes
#define SHADER(x, y, z) class x : public Shader {\
public:\
    x() : Shader(y, z) {}\
};

SHADER(DiffuseShader, "diffuse", false)
SHADER(PointLightShader, "point_light", true)
