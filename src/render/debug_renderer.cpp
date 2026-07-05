#include "render/debug_renderer.h"

DebugRenderer* DebugRenderer::debug_renderer = nullptr;

DebugRenderer::DebugRenderer()
{
    this->Initialize();
    debug_renderer = this;
}

DebugRenderer::~DebugRenderer() {}

void DebugRenderer::execute(
    const glm::mat4& view,
    const glm::mat4& projection,
    const float width,
    const float height
)
{
    auto draw = ImGui::GetBackgroundDrawList();

    const auto project = [&](const glm::vec3 x) -> std::optional<glm::vec2>
    {
        const glm::vec4 clip = projection * view * glm::vec4(x, 1.0f);

        // Behind the camera
        if (clip.w <= 0.0f)
            return std::nullopt;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        glm::vec2 screen = {
            (ndc.x * 0.5f + 0.5f) * width,
            (1.0f - (ndc.y * 0.5f + 0.5f)) * height
        };

        return screen;
    };

    srand(0);
    for (const auto& line : lines_this_frame)
    {
        const auto a = project(line.from);
        const auto b = project(line.to);

        if (!a || !b) continue;

        float cr = (float)rand() / (float)RAND_MAX * 255.0f;
        float cg = (float)rand() / (float)RAND_MAX * 255.0f;
        float cb = (float)rand() / (float)RAND_MAX * 255.0f;

        draw->AddLine(
            ImVec2(a->x, a->y),
            ImVec2(b->x, b->y),
            IM_COL32(cr, cg, cb, 0xff)
        );
    }

    lines_this_frame.clear();
}

void DebugRenderer::DrawLine(
    JPH::RVec3Arg from,
    JPH::RVec3Arg to,
    JPH::ColorArg colour
)
{
    lines_this_frame.emplace_back(Line {
        .from = { from.GetX(), from.GetY(), from.GetZ() },
        .to = { to.GetX(), to.GetY(), to.GetZ() }
    });
}

void DebugRenderer::DrawText3D(
    JPH::RVec3Arg position,
    const std::string_view& string,
    JPH::ColorArg colour,
    float height
)
{
    (void)position;
    (void)string;
    (void)colour;
    (void)height;
}