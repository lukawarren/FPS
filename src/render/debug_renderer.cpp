#include "render/debug_renderer.h"

DebugRenderer* DebugRenderer::debug_renderer = nullptr;

DebugRenderer::DebugRenderer()
{
    this->Initialize();
    debug_renderer = this;
}

DebugRenderer::~DebugRenderer() {}

static bool clip_line_to_near_plane(glm::vec3 &a, glm::vec3 &b, float nearPlane)
{
    const float plane_z = -nearPlane;
    const bool a_inside = a.z <= plane_z;
    const bool b_inside = b.z <= plane_z;

    if (a_inside && b_inside)
        return true;

    if (!a_inside && !b_inside)
        return false;

    const float t = (plane_z - a.z) / (b.z - a.z);
    glm::vec3 intersection = glm::mix(a, b, t);

    if (!a_inside) a = intersection;
    else b = intersection;
    return true;
}

static u32 get_hash(u32 id)
{
    id ^= id >> 16;
    id *= 0x7feb352dU;
    id ^= id >> 15;
    id *= 0x846ca68bU;
    id ^= id >> 16;
    return id;
}

void DebugRenderer::execute(
    const glm::mat4& view,
    const glm::mat4& projection,
    const float width,
    const float height
)
{
    auto draw = ImGui::GetBackgroundDrawList();

    const float near_plane = 0.01f;
    u32 i = 0;

    for (const auto &line : lines_this_frame)
    {
        const glm::vec4 va = view * glm::vec4(line.from, 1.0f);
        const glm::vec4 vb = view * glm::vec4(line.to,   1.0f);

        glm::vec3 a = glm::vec3(va);
        glm::vec3 b = glm::vec3(vb);
        if (!clip_line_to_near_plane(a, b, near_plane))
            continue;

        const glm::vec4 clip_a = projection * glm::vec4(a, 1.0f);
        const glm::vec4 clip_b = projection * glm::vec4(b, 1.0f);

        const glm::vec3 ndc_a = glm::vec3(clip_a) / clip_a.w;
        const glm::vec3 ndc_b = glm::vec3(clip_b) / clip_b.w;

        ImVec2 screen_a(
            (ndc_a.x * 0.5f + 0.5f) * width,
            (1.0f - (ndc_a.y * 0.5f + 0.5f)) * height
        );

        ImVec2 screen_b(
            (ndc_b.x * 0.5f + 0.5f) * width,
            (1.0f - (ndc_b.y * 0.5f + 0.5f)) * height
        );

        const u32 hash = get_hash(i++);
        const u32 cr = (hash & 0xFF);
        const u32 cg = ((hash >> 8) & 0xFF);
        const u32 cb = ((hash >> 16) & 0xFF);
        draw->AddLine(screen_a, screen_b, IM_COL32(cr, cg, cb, 0xff));
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