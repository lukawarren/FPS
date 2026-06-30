#include "map.h"
#include "enemy.h"

constexpr static inline csg::volume_t VOLUME_AIR = 0;
constexpr static inline csg::volume_t VOLUME_SOLID = 1;

Map::Map(const std::string& filename, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    // Setup CSG "world"
    world = new csg::world_t();
    world->set_void_volume(VOLUME_AIR);

    // Load text file
    std::ifstream file(MAP_ROOT + filename);
    if (!file) throw std::runtime_error("Unable to load map " + filename);

    // Parse
    std::string line;
    while (std::getline(file, line))
    {
        if (line == "{")
            parse_entity(file, device, copy_pass);
    }

    build_meshes(device, copy_pass);
    textures.clear();
    texture_infos.clear();

    // World no longer needed
    delete world;
}

Map::~Map()
{
    for (auto& draw_call : draw_calls)
    {
        delete draw_call.mesh;
        delete draw_call.texture;
    }

    if (nav_query) dtFreeNavMeshQuery(nav_query);
    if (nav_mesh) dtFreeNavMesh(nav_mesh);
}

std::vector<glm::vec3> Map::find_path(const glm::vec3& start, const glm::vec3& end) const
{
    std::vector<glm::vec3> path;
    if (!nav_query) return path;

    const float extents[3] = { 2.0f, 4.0f, 2.0f };
    dtQueryFilter filter;

    dtPolyRef start_ref = 0, end_ref = 0;
    float start_pt[3], end_pt[3];
    const float start_pos[3] = { start.x, start.y, start.z };
    const float end_pos[3] = { end.x, end.y, end.z };

    nav_query->findNearestPoly(start_pos, extents, &filter, &start_ref, start_pt);
    nav_query->findNearestPoly(end_pos, extents, &filter, &end_ref, end_pt);

    if (!start_ref || !end_ref) return path;

    dtPolyRef poly_path[256];
    int poly_count = 0;
    nav_query->findPath(start_ref, end_ref, start_pt, end_pt, &filter, poly_path, &poly_count, 256);
    if (poly_count == 0) return path;

    float straight_path[256 * 3];
    unsigned char straight_path_flags[256];
    dtPolyRef straight_path_polys[256];
    int straight_path_count = 0;

    nav_query->findStraightPath(
        start_pt, end_pt, poly_path, poly_count,
        straight_path, straight_path_flags, straight_path_polys,
        &straight_path_count, 256
    );

    path.reserve(straight_path_count);
    for (int i = 0; i < straight_path_count; ++i)
    {
        path.emplace_back(
            straight_path[i * 3 + 0],
            straight_path[i * 3 + 1],
            straight_path[i * 3 + 2]
        );
    }

    return path;
}

void Map::parse_entity(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    // Get properties
    const auto get_property = [](auto& line)
    {
        std::istringstream iss(line);
        std::string name, value;
        iss >> name;
        iss.ignore(1);
        std::getline(iss, value);

        // Remove surrounding quotes
        if (name[0] == '"') name.erase(0, 1);
        if (name[name.size() - 1] == '"') name.erase(name.size() - 1, 1);
        if (value[0] == '"') value.erase(0, 1);
        if (value[value.size() - 1] == '"') value.erase(value.size() - 1, 1);

        return std::pair {
            name,
            value
        };
    };

    Entity entity = {};

    std::string line;
    while (std::getline(stream, line))
    {
        // Comment
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/')
            continue;

        // Brush data
        if (line == "{")
        {
            parse_brush(stream, device, copy_pass);
            continue;
        }

        // End of entity
        if (line == "}")
            break;

        // Property
        const auto [name, value] = get_property(line);
        entity.properties[name] = value;
    }

    entities.push_back(entity);
}

void Map::parse_brush(std::ifstream& stream, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    std::string line;
    csg::brush_t* brush = world->add();
    std::vector<csg::plane_t> planes;
    std::vector<size_t> texture_info_indices;

    // Brush setup
    brush->set_volume_operation(csg::make_fill_operation(VOLUME_SOLID));

    const auto parse_texture_name = [](std::istream& is)
    {
        is >> std::ws;

        if (is.peek() == '"')
        {
            is.get();
            std::string name;
            std::getline(is, name, '"');
            return name;
        }

        std::string name;
        is >> name;
        return name;
    };

    while (std::getline(stream, line))
    {
        // End of brush
        if (line == "}")
        {
            brush->userdata = texture_info_indices;
            brush->set_planes(planes);
            return;
        }

        else
        {
            // Parse face
            std::istringstream iss(line);

            const auto parse_vertex = [](auto& iss)
            {
                iss.ignore(1);
                float x, y, z;
                iss >> x >> y >> z;
                iss.ignore(3);
                return glm::vec3(x, y, z);
            };

            // Convert three vertices to a plane, consisting of a normal and a direction
            const glm::vec3 a = parse_vertex(iss);
            const glm::vec3 b = parse_vertex(iss);
            const glm::vec3 c = parse_vertex(iss);
            const std::pair<glm::vec3, float> plane = plane_from_points(c, b, a);

            const auto parse_plane = [](auto& iss)
            {
                iss.ignore(2);
                float a, b, c, d;
                iss >> a >> b >> c >> d;
                iss.ignore(3);

                return csg::plane_t {
                    .normal = { a, b, c },
                    .offset = d
                };
            };

            // Get texture info
            const std::string texture_name = parse_texture_name(iss);
            const csg::plane_t u_plane = parse_plane(iss);
            const csg::plane_t v_plane = parse_plane(iss);
            float rotation, scale_x, scale_y;
            iss >> rotation >> scale_x >> scale_y;

            // Rotation not needed
            (void)rotation;

            planes.push_back(csg::plane_t {
                .normal = plane.first,
                .offset = plane.second
            });

            texture_infos.emplace_back(TextureInfo {
                .name = texture_name,
                .axes = { u_plane, v_plane },
                .scale = { scale_x, scale_y },
            });

            textures.insert(texture_name);

            texture_info_indices.push_back(texture_infos.size() - 1);
        }
    }
}

void Map::build_meshes(SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass)
{
    world->rebuild();
    csg::brush_t* brush = world->first();

    // Build separates mesh per texture
    struct TexturedMesh
    {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<float> texture_coordinates;
        std::vector<unsigned int> indices;
    };
    std::unordered_map<std::string, TexturedMesh> meshes;

    while (brush != nullptr)
    {
        // Retrieve texture info (per face)
        std::vector<size_t> texture_info_indices = std::any_cast<std::vector<size_t>>(brush->userdata);

        auto faces = brush->get_faces();
        for (size_t local_index = 0; local_index < faces.size(); ++local_index)
        {
            const csg::face_t& face = faces[local_index];

            // "No-draw" faces
            const TextureInfo& info = texture_infos[texture_info_indices[local_index]];
            if (info.name == "__TB_empty")
                continue;

            // Identify (or create) correct mesh
            if (meshes.count(info.name) == 0)
                meshes[info.name] = TexturedMesh {};
            TexturedMesh& mesh = meshes[info.name];

            for (const csg::fragment_t& fragment : face.fragments)
            {
                csg::volume_t front = fragment.front_volume;
                csg::volume_t back = fragment.back_volume;

                // Discard polygons who are "air-air" or "solid-solid"
                if (front == back)
                    continue;

                // Discard non-frontface polygons
                if (front != VOLUME_AIR)
                    continue;

                // Record vertices
                const size_t offset = mesh.vertices.size() / 3;
                for (const auto& vertex : fragment.vertices)
                {
                    mesh.vertices.push_back(vertex.position.x * METRES_PER_UNIT);
                    mesh.vertices.push_back(vertex.position.z * METRES_PER_UNIT);
                    mesh.vertices.push_back(vertex.position.y * METRES_PER_UNIT * -1.0f);
                }

                // Record normals
                for (size_t i = 0; i < fragment.vertices.size(); ++i)
                {
                    mesh.normals.push_back(face.plane->normal.x);
                    mesh.normals.push_back(face.plane->normal.z);
                    mesh.normals.push_back(face.plane->normal.y * -1.0f);
                }

                // Record indices
                std::vector<csg::triangle_t> triangles = csg::triangulate(fragment);
                for (const auto& triangle : triangles)
                {
                    mesh.indices.push_back(triangle.i + offset);
                    mesh.indices.push_back(triangle.j + offset);
                    mesh.indices.push_back(triangle.k + offset);
                }

                calculate_uvs(mesh.texture_coordinates, fragment.vertices, info);
            }
        }

        brush = world->next(brush);
    }

    // Add for rendering
    for (const auto &[key, value] : meshes)
    {
        if (value.vertices.size() == 0) continue;

        std::vector<Mesh::Vertex> vertices;
        vertices.reserve(value.vertices.size() / 3);

        for (size_t i = 0; i < value.vertices.size() /  3; i++)
        {
            vertices.emplace_back(Mesh::Vertex {
                .position = { value.vertices[i * 3 + 0], value.vertices[i * 3 + 1], value.vertices[i * 3 + 2] },
                .normal = { value.normals[i * 3 + 0], value.normals[i * 3 + 1], value.normals[i * 3 + 2] },
                .uv = { value.texture_coordinates[i * 2 + 0], value.texture_coordinates[i * 2 + 1] },
            });
        }

        draw_calls.emplace_back(DrawCall {
            .mesh = new Mesh(
                vertices,
                value.indices,
                device,
                copy_pass
            ),
            .texture = new Texture(key + ".png", device, copy_pass)
        });
    }

    // Add for physics
    JPH::VertexList p_vertices;
    JPH::IndexedTriangleList p_indices;
    uint32_t vertex_offset = 0;

    // Add for navigation (same geometry, recast-friendly layout)
    std::vector<float> nav_vertices;
    std::vector<int> nav_indices;

    for (const auto &[key, value] : meshes)
    {
        if (value.vertices.size() == 0) continue;

        for (size_t i = 0; i < value.vertices.size() /  3; i++)
        {
            p_vertices.emplace_back(JPH::Float3 {
                value.vertices[i * 3 + 0],
                value.vertices[i * 3 + 1],
                value.vertices[i * 3 + 2]
            });

            nav_vertices.push_back(value.vertices[i * 3 + 0]);
            nav_vertices.push_back(value.vertices[i * 3 + 1]);
            nav_vertices.push_back(value.vertices[i * 3 + 2]);
        }

        for (size_t i = 0; i < value.indices.size() /  3; i++)
        {
            p_indices.emplace_back(JPH::IndexedTriangle {
                value.indices[i * 3 + 0] + vertex_offset,
                value.indices[i * 3 + 1] + vertex_offset,
                value.indices[i * 3 + 2] + vertex_offset
            });

            nav_indices.push_back((int)(value.indices[i * 3 + 0] + vertex_offset));
            nav_indices.push_back((int)(value.indices[i * 3 + 1] + vertex_offset));
            nav_indices.push_back((int)(value.indices[i * 3 + 2] + vertex_offset));
        }

        vertex_offset += value.vertices.size() / 3;
    }

    JPH::MeshShapeSettings settings(p_vertices, p_indices);
    JPH::Shape::ShapeResult result = settings.Create();

    if (result.IsValid())
        physics_shape = result.Get();
    else
        throw std::runtime_error("Failed to create physics shape for map: " + std::string(result.GetError()));

    build_navmesh(nav_vertices, nav_indices);
}

void Map::calculate_uvs(
    std::vector<float>& texture_coordinates,
    const std::vector<csg::vertex_t>& vertices,
    const TextureInfo& info
)
{
    const glm::vec3 u_axis = info.axes[0].normal / info.scale.x;
    const glm::vec3 v_axis = info.axes[1].normal / info.scale.y;

    for (const auto& vertex : vertices)
    {
        float u = glm::dot(vertex.position, u_axis);
        float v = glm::dot(vertex.position, v_axis);
        u += info.axes[0].offset;
        v += info.axes[1].offset;
        u /= TRENCHBROOM_TEXTURE_SIZE;
        v /= TRENCHBROOM_TEXTURE_SIZE;

        texture_coordinates.emplace_back(u);
        texture_coordinates.emplace_back(v);
    }
}

std::pair<glm::vec3, float> Map::plane_from_points(
    const glm::vec3& p1,
    const glm::vec3& p2,
    const glm::vec3& p3) const
{
    const glm::vec3 normal = glm::normalize(glm::cross(
        p3 - p2,
        p1 - p2
    ));
    const float distance = -glm::dot(normal, p1);
    return { normal, distance };
}

glm::vec3 Map::Entity::parse_vec3(
    const std::string& key,
    const glm::vec3 default_value,
    const bool scale
) const
{
    if (properties.count(key) == 0)
        return default_value;

    std::istringstream iss(properties.at(key));
    float x, y, z;
    iss >> x >> y >> z;

    if (scale)
        return glm::vec3 { x, z, -y } * METRES_PER_UNIT;

    return { x, y, z };
}

float Map::Entity::parse_float(
    const std::string& key,
    const float default_value
) const
{
    if (properties.count(key) == 0)
        return default_value;

    std::istringstream iss(properties.at(key));
    float x;
    iss >> x;
    return x;
}

bool Map::Entity::parse_bool(
    const std::string& key,
    const bool default_value
) const
{
    if (properties.count(key) == 0)
        return default_value;

    std::istringstream iss(properties.at(key));
    bool x;
    iss >> x;
    return x;
}

void Map::build_navmesh(const std::vector<float>& vertices, const std::vector<int>& indices)
{
    const int nverts = (int)vertices.size() / 3;
    const int ntris = (int)indices.size() / 3;
    if (nverts == 0 || ntris == 0) return;

    float bmin[3], bmax[3];
    rcCalcBounds(vertices.data(), nverts, bmin, bmax);

    constexpr float AGENT_HEIGHT = Enemy::ENEMY_HEIGHT;
    constexpr float AGENT_RADIUS = Enemy::ENEMY_RADIUS;
    constexpr float AGENT_MAX_CLIMB = 0.4f;
    constexpr float AGENT_MAX_SLOPE = 45.0f;

    rcConfig cfg = {};
    cfg.cs = 0.15f;
    cfg.ch = 0.1f;
    cfg.walkableSlopeAngle = AGENT_MAX_SLOPE;
    cfg.walkableHeight = (int)std::ceil(AGENT_HEIGHT / cfg.ch);
    cfg.walkableClimb = (int)std::floor(AGENT_MAX_CLIMB / cfg.ch);
    cfg.walkableRadius = (int)std::ceil(AGENT_RADIUS / cfg.cs);
    cfg.maxEdgeLen = (int)(12.0f / cfg.cs);
    cfg.maxSimplificationError = 1.3f;
    cfg.minRegionArea = (int)rcSqr(8);
    cfg.mergeRegionArea = (int)rcSqr(20);
    cfg.maxVertsPerPoly = 6;
    cfg.detailSampleDist = 6.0f * cfg.cs;
    cfg.detailSampleMaxError = cfg.ch;
    rcVcopy(cfg.bmin, bmin);
    rcVcopy(cfg.bmax, bmax);
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    rcContext ctx;

    rcHeightfield* hf = rcAllocHeightfield();
    rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch);

    std::vector<unsigned char> tri_areas(ntris, 0);
    rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, vertices.data(), nverts, indices.data(), ntris, tri_areas.data());
    rcRasterizeTriangles(&ctx, vertices.data(), nverts, indices.data(), tri_areas.data(), ntris, *hf, cfg.walkableClimb);

    rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
    rcFilterLedgeSpans(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
    rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);

    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf);
    rcFreeHeightField(hf);

    rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf);
    rcBuildDistanceField(&ctx, *chf);
    rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea);

    rcContourSet* cset = rcAllocContourSet();
    rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset);

    rcPolyMesh* pmesh = rcAllocPolyMesh();
    rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh);

    rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
    rcBuildPolyMeshDetail(&ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh);

    rcFreeCompactHeightfield(chf);
    rcFreeContourSet(cset);

    for (int i = 0; i < pmesh->npolys; ++i)
        if (pmesh->areas[i] == RC_WALKABLE_AREA)
            pmesh->flags[i] = 1;

    dtNavMeshCreateParams params = {};
    params.verts = pmesh->verts;
    params.vertCount = pmesh->nverts;
    params.polys = pmesh->polys;
    params.polyAreas = pmesh->areas;
    params.polyFlags = pmesh->flags;
    params.polyCount = pmesh->npolys;
    params.nvp = pmesh->nvp;
    params.detailMeshes = dmesh->meshes;
    params.detailVerts = dmesh->verts;
    params.detailVertsCount = dmesh->nverts;
    params.detailTris = dmesh->tris;
    params.detailTriCount = dmesh->ntris;
    params.walkableHeight = AGENT_HEIGHT;
    params.walkableRadius = AGENT_RADIUS;
    params.walkableClimb = AGENT_MAX_CLIMB;
    rcVcopy(params.bmin, pmesh->bmin);
    rcVcopy(params.bmax, pmesh->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = true;

    unsigned char* nav_data = nullptr;
    int nav_data_size = 0;
    if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size))
    {
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        throw std::runtime_error("Failed to build navmesh data");
    }

    nav_mesh = dtAllocNavMesh();
    if (dtStatusFailed(nav_mesh->init(nav_data, nav_data_size, DT_TILE_FREE_DATA)))
        throw std::runtime_error("Failed to init navmesh");

    nav_query = dtAllocNavMeshQuery();
    nav_query->init(nav_mesh, 2048);

    rcFreePolyMesh(pmesh);
    rcFreePolyMeshDetail(dmesh);
}