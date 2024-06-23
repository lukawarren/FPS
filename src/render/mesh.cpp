#include "render/mesh.h"

Mesh::Mesh(
    const std::vector<float>& vertices,
    const std::vector<float>& texture_coordinates,
    const std::vector<float>& normals,
    const std::vector<unsigned int>& indices
)
{
    construct(vertices, texture_coordinates, normals, indices);
}

Mesh::Mesh(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices
)
{
    construct(vertices, {}, {}, indices);
}

void Mesh::construct(
    const std::vector<float>& vertices,
    const std::vector<float>& texture_coordinates,
    const std::vector<float>& normals,
    const std::vector<unsigned int>& indices
)
{
    // Create and bind VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create, bind and upload indices' element buffer object (EBO)
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);

    // Make VAOs
    make_vao(0, GL_FLOAT, 3, vertices);
    if (!texture_coordinates.empty()) make_vao(1, GL_FLOAT, 2, texture_coordinates);
    if (!normals.empty()) make_vao(2, GL_FLOAT, 3, normals);

    this->n_indices = indices.size();
}

Mesh::Mesh(const std::string& filename)
{
    const auto flags =
        aiProcess_Triangulate | aiProcess_OptimizeMeshes |
        aiProcess_GenNormals | aiProcess_OptimizeGraph |
        aiProcess_FlipUVs;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("../assets/models/" + filename, flags);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
        throw std::runtime_error("unable to load model " + filename);

    const auto node = scene->mRootNode;
    assert(node->mNumMeshes == 1);

    // Just use root mesh for now
    const auto mesh_index = node->mMeshes[0];
    const aiMesh* mesh = scene->mMeshes[mesh_index];

    make_mesh_from_assimp(mesh);
}

void Mesh::make_vao(
    const unsigned int attribute,
    const unsigned int format,
    const unsigned int dimensions,
    const std::vector<float>& data
)
{
    // Make and fill VBO
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(data[0]), &data[0], GL_STATIC_DRAW);

    // Inform OpenGL of the nature of the data - GL_FALSE disables normalisation of data
    glVertexAttribPointer(attribute, dimensions, format, GL_FALSE, dimensions * sizeof(data[0]), (void*)0);
    glEnableVertexAttribArray(attribute);

    // Unbind then keep track of VBO for future clean-up
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    vbos.push_back(vbo);
}

void Mesh::bind() const
{
    glBindVertexArray(vao);
}

void Mesh::unbind() const
{
    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, 0);
}

void Mesh::make_mesh_from_assimp(const aiMesh* assimp_mesh)
{
    // Check if mesh has all data needed
    if (!assimp_mesh->HasPositions() || !assimp_mesh->HasTextureCoords(0))
        throw std::runtime_error("mesh does not have all required data");

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texture_coords;
    std::vector<unsigned int> indices;
    vertices.reserve(assimp_mesh->mNumVertices * 3);
    normals.reserve(assimp_mesh->mNumVertices * 3);
    texture_coords.reserve(assimp_mesh->mNumVertices * 2);
    indices.reserve(assimp_mesh->mNumFaces * 3);

    // Vertices
    for (unsigned int i = 0; i < assimp_mesh->mNumVertices; ++i)
    {
        vertices.push_back(assimp_mesh->mVertices[i].x);
        vertices.push_back(assimp_mesh->mVertices[i].y);
        vertices.push_back(assimp_mesh->mVertices[i].z);

        normals.push_back(assimp_mesh->mNormals[i].x);
        normals.push_back(assimp_mesh->mNormals[i].y);
        normals.push_back(assimp_mesh->mNormals[i].z);

        texture_coords.push_back(assimp_mesh->mTextureCoords[0][i].x);
        texture_coords.push_back(assimp_mesh->mTextureCoords[0][i].y);
    }

    // Indices
    for (unsigned int i = 0; i < assimp_mesh->mNumFaces; ++i)
    {
        const aiFace& face = assimp_mesh->mFaces[i];
        if (face.mNumIndices != 3)
            throw std::runtime_error("model has incomplete face");

        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }

    construct(
        vertices,
        texture_coords,
        normals,
        indices
    );
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &vao);

    for (const auto vbo: vbos)
        glDeleteBuffers(1, &vbo);

    glDeleteBuffers(1, &ebo);
}
