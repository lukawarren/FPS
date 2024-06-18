#include "render/shader.h"

struct ShaderType
{
    std::string extension;
    unsigned int identifier;
};

static std::array<ShaderType, 2> shader_types =
{{
    { .extension = ".vert", .identifier = GL_VERTEX_SHADER   },
    { .extension = ".frag", .identifier = GL_FRAGMENT_SHADER },
}};

Shader::Shader(const std::string& filename)
{
    const auto read_file = [](const std::string path)
    {
        std::ifstream in("../shaders/" + path);
        std::string contents((std::istreambuf_iterator<char>)(in), std::istreambuf_iterator<char>());
        if (contents == "") throw std::runtime_error("unable to load shader " + path);
        return contents;
    };

    struct ShaderTarget
    {
        std::string source;
        unsigned int shader = 0;
        ShaderType& shader_type;
    };

    std::array<ShaderTarget, 2> targets =
    {
        // Vertex
        ShaderTarget
        {
            .source = read_file(filename + shader_types[0].extension),
            .shader_type = shader_types[0]
        },

        // Fragment
        ShaderTarget
        {
            .source = read_file(filename + shader_types[1].extension),
            .shader_type = shader_types[1]
        }
    };

    // Upload source code
    for (auto& target : targets)
    {
        const char* c_str = target.source.c_str();
        target.shader = glCreateShader(target.shader_type.identifier);
        glShaderSource(target.shader, 1, &c_str, NULL);
    }

    // Compile and check for errors
    for (auto& target : targets)
    {
        const std::string& extension = target.shader_type.extension;
        glCompileShader(target.shader);
        handle_error(
            glGetShaderiv,
            glGetShaderInfoLog,
            target.shader,
            GL_COMPILE_STATUS,
            extension + " shader failed to compile",
            filename
        );
    }

    // Link into one "program"
    program = glCreateProgram();
    for (auto& target : targets)
        glAttachShader(program, target.shader);
    glLinkProgram(program);
    handle_error(glGetProgramiv, glGetProgramInfoLog, program, GL_LINK_STATUS, "shader failed to link", filename);

    // Shaders themselves no longer needed - all we need is the final program
    for (auto& target : targets)
        glDeleteShader(target.shader);
}

void Shader::bind() const
{
    glUseProgram(program);
}

void Shader::unbind() const
{
    glUseProgram(0);
}

SHADER_UNIFORM(const glm::mat4& matrix) { glUniformMatrix4fv(NAME, 1, GL_FALSE, glm::value_ptr(matrix)); }
SHADER_UNIFORM(const glm::vec4& vector) { glUniform4f(NAME, vector.x, vector.y, vector.z, vector.w); }
SHADER_UNIFORM(const glm::vec3& vector) { glUniform3f(NAME, vector.x, vector.y, vector.z); }
SHADER_UNIFORM(const glm::vec2& vector) { glUniform2f(NAME, vector.x, vector.y); }
SHADER_UNIFORM(const float value)       { glUniform1f(NAME, value); }
SHADER_UNIFORM(const int value)         { glUniform1i(NAME, value); }

void Shader::handle_error(auto f, auto f2, const unsigned int id,
        const unsigned int type, const std::string& error, const std::string& filename)
{
    int success;
    f(id, type, &success);
    if (!success)
    {
        char info[512] = {};
        f2(id, 512, NULL, info);
        throw std::runtime_error(filename + ": " + error + "\n" + info);
    }
}

int Shader::get_uniform_location(const std::string& name)
{
    // Use cached version...
    if (uniforms.contains(name))
        return uniforms.at(name);

    // ...or get from OpenGL then store for later
    int result = glGetUniformLocation(program, name.c_str());
    if (result < 0) std::cerr << "failed to create uniform "  << name << std::endl;
    uniforms.emplace(name, result);
    return result;
}

Shader::~Shader()
{
    glDeleteProgram(program);
}