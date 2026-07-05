#pragma once
#include "common.h"
#include "camera.h"

class DebugRenderer : public JPH::DebugRendererSimple
{
public:
    DebugRenderer();
    virtual ~DebugRenderer();

    void execute(
        const glm::mat4& view,
        const glm::mat4& projection,
        const float width,
        const float height
    );

    virtual void DrawLine(
        JPH::RVec3Arg from,
        JPH::RVec3Arg to,
        JPH::ColorArg colour
    ) override;

    virtual void DrawText3D(
        JPH::RVec3Arg position,
        const std::string_view& string,
        JPH::ColorArg colour,
        float height
    ) override;

    static DebugRenderer* debug_renderer;

private:
    glm::mat4 view;
    glm::mat4 projection;

    struct Line
    {
        glm::vec3 from;
        glm::vec3 to;
    };
    std::vector<Line> lines_this_frame;
};