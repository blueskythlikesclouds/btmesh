#include "bina.h"

static const std::pair<std::string_view, int> TYPES[] =
{
    { "@NONE", 0 },
    { "@STONE", 1 },
    { "@EARTH", 2 },
    { "@WOOD", 3 },
    { "@GRASS", 4 },
    { "@IRON", 5 },
    { "@SAND", 6 },
    { "@LAVA", 7 },
    { "@GLASS", 8 },
    { "@SNOW", 9 },
    { "@NO_ENTRY", 0xA },
    { "@ICE", 0xB },
    { "@WATER", 0xC },
    { "@SEA", 0xD },
    { "@DAMAGE", 0xE },
    { "@DEAD", 0xF },
    { "@FLOWER0", 0x10 },
    { "@FLOWER1", 0x11 },
    { "@FLOWER2", 0x12 },
    { "@AIR", 0x13 },
    { "@DEADLEAVES", 0x14 },
    { "@WIREMESH", 0x15 },
    { "@DEAD_ANYDIR", 0x16 },
    { "@DAMAGE_THROUGH", 0x17 },
    { "@DRY_GRASS", 0x18 },
    { "@RELIC", 0x19 },
    { "@GIANT", 0x1A },
    { "@GRAVEL", 0x1B },
    { "@MUD_WATER", 0x1C },
    { "@SAND2", 0x1D },
    { "@SAND3", 0x1E }
};

static const std::pair<std::string_view, int> LAYERS[] =
{
    { "@NONE", 0 },
    { "@SOLID", 1 },
    { "@LIQUID", 2 },
    { "@THROUGH", 3 },
    { "@CAMERA", 4 },
    { "@SOLID_ONEWAY", 5 },
    { "@SOLID_THROUGH", 6 },
    { "@SOLID_TINY", 7 },
    { "@SOLID_DETAIL", 8 },
    { "@LEAF", 9 },
    { "@LAND", 10 },
    { "@RAYBLOCK", 11 },
    { "@EVENT", 12 },
    { "@RESERVED13", 13 },
    { "@RESERVED14", 14 },
    { "@PLAYER", 15 },
    { "@ENEMY", 16 },
    { "@ENEMY_BODY", 17 },
    { "@GIMMICK", 18 },
    { "@DYNAMICS", 19 },
    { "@RING", 20 },
    { "@CHARACTER_CONTROL", 21 },
    { "@PLAYER_ONLY", 22 },
    { "@DYNAMICS_THROUGH", 23 },
    { "@ENEMY_ONLY", 24 },
    { "@SENSOR_PLAYER", 25 },
    { "@SENSOR_RING", 26 },
    { "@SENSOR_GIMMICK", 27 },
    { "@SENSOR_LAND", 28 },
    { "@SENSOR_ALL", 29 },
    { "@RESERVED30", 30 },
    { "@RESERVED31", 31 },
};

static const std::pair<std::string_view, int> FLAGS[] =
{
    { "@NOT_STAND", 0 },
    { "@BREAKABLE", 1 },
    { "@REST", 2 },
    { "@UNSUPPORTED", 3 },
    { "@REFLECT_LASER", 4 },
    { "@LOOP", 5 },
    { "@WALL", 6 },
    { "@SLIDE", 7 },
    { "@PARKOUR", 8 },
    { "@DECELERATE", 9 },
    { "@MOVABLE", 0xA },
    { "@PARKOUR_KNUCKLES", 0xB },
    { "@PRESS_DEAD", 0xC },
    { "@RAYBLOCK", 0xD },
    { "@WALLJUMP", 0xE },
    { "@PUSH_BOX", 0xF },
    { "@STRIDER_FLOOR", 0x10 },
    { "@GIANT_TOWER", 0x11 },
    { "@PUSHOUT_LANDING", 0x12 },
    { "@TEST_GRASS", 0x14 },
    { "@TEST_WATER", 0x15 }
};

static bool check_tag_exists(const std::string& value, const std::string_view& tag)
{
    const size_t index = value.find(tag);
    return index != std::string::npos && (index + tag.size() == value.size() || value[index + tag.size()] == '@');
}

struct ai_mesh_cache
{
    bool is_convex_shape;
    int layer;
    std::vector<btVector3> vertices;
    std::vector<short> indices;
    std::unique_ptr<uint8_t[]> serialized_bvh;
    int type_and_flags;
};

static void convert_to_ai_mesh_caches(const aiScene* ai_scene, std::vector<ai_mesh_cache>& mesh_caches, const aiNode* ai_node)
{
    if (ai_node->mNumMeshes != 0)
    {
        std::string mesh_name(ai_node->mName.C_Str());
        std::transform(mesh_name.begin(), mesh_name.end(), mesh_name.begin(), std::toupper);

        const bool is_convex_shape = check_tag_exists(mesh_name, "@CONVEX");
        int layer = 1;
        int type = 0;
        int flags = 0;

        for (auto& [name, value] : LAYERS)
        {
            if (check_tag_exists(mesh_name, name))
            {
                layer = value;
                break;
            }
        }

        for (auto& [name, value] : TYPES)
        {
            if (check_tag_exists(mesh_name, name))
            {
                type = value;
                break;
            }
        }

        for (auto& [name, value] : FLAGS)
        {
            if (check_tag_exists(mesh_name, name))
            {
                flags |= 1 << value;
            }
        }

        for (int i = 0; i < ai_node->mNumMeshes; i++)
        {
            const aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];

            ai_mesh_cache mesh_cache;
            mesh_cache.is_convex_shape = is_convex_shape;
            mesh_cache.layer = layer;
            mesh_cache.type_and_flags = (type << 24) | flags;

            for (int j = 0; j < ai_mesh->mNumVertices; j++)
            {
                auto& vertex = ai_mesh->mVertices[j];
                mesh_cache.vertices.emplace_back(vertex.x, vertex.y, vertex.z);
            }

            if (!mesh_cache.is_convex_shape)
            {
                for (int j = 0; j < ai_mesh->mNumFaces; j++)
                {
                    aiFace& ai_face = ai_mesh->mFaces[j];
                    if (ai_face.mNumIndices == 3)
                    {
                        mesh_cache.indices.push_back((short)ai_face.mIndices[0]);
                        mesh_cache.indices.push_back((short)ai_face.mIndices[1]);
                        mesh_cache.indices.push_back((short)ai_face.mIndices[2]);
                    }
                }
            }

            if (!mesh_cache.vertices.empty() && (mesh_cache.is_convex_shape || !mesh_cache.indices.empty()))
            {
                mesh_caches.push_back(std::move(mesh_cache));
            }
        }
    }

    for (int i = 0; i < ai_node->mNumChildren; i++)
    {
        convert_to_ai_mesh_caches(ai_scene, mesh_caches, ai_node->mChildren[i]);
    }
}

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        return -1;
    }

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 32767);
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, 
        aiComponent_NORMALS |
        aiComponent_TANGENTS_AND_BITANGENTS |
        aiComponent_COLORS | 
        aiComponent_TEXCOORDS | 
        aiComponent_BONEWEIGHTS |
        aiComponent_TEXTURES | 
        aiComponent_MATERIALS
    );
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);

    const aiScene* ai_scene = importer.ReadFile(argv[1], 
        aiProcess_Triangulate | 
        aiProcess_SortByPType | 
        aiProcess_JoinIdenticalVertices | 
        aiProcess_SplitLargeMeshes |
        aiProcess_RemoveComponent | 
        aiProcess_PreTransformVertices
    );

    std::vector<ai_mesh_cache> mesh_caches;
    convert_to_ai_mesh_caches(ai_scene, mesh_caches, ai_scene->mRootNode);

    std::string file_path = argv[1];

    int index = file_path.find_last_of("\\/");
    std::string dir_path = file_path.substr(0, index + 1);
    std::string file_name = file_path.substr(index + 1);

    std::string file_name_no_ext = file_name;
    if ((index = file_name_no_ext.find('.')) >= 0)
    {
        file_name_no_ext = file_name_no_ext.substr(0, index);
    }

    if (file_name_no_ext.find("_col") == std::string::npos)
    {
        file_name_no_ext += "_col";
    }

    std::string btmesh_file_path = dir_path + file_name_no_ext + ".btmesh";
    bina btmesh(btmesh_file_path.c_str());

    btmesh.write<int>(0);
    btmesh.write<int>(3);
    btmesh.write_offset(16, [&btmesh, &mesh_caches]
    {
        for (auto& mesh_cache : mesh_caches)
        {
            if (mesh_cache.is_convex_shape)
            {
                btmesh.write<int>(2); // is_convex_shape
                btmesh.write<int>(mesh_cache.layer);
                btmesh.write<int>(mesh_cache.vertices.size());
                btmesh.write<int>(0);
                btmesh.write<int>(0);
                btmesh.write<int>(1);
                btmesh.write<int>(0);
                btmesh.write<int>(0);
                btmesh.write_offset(16, [&btmesh, &mesh_cache]
                {
                    for (auto& vertex : mesh_cache.vertices)
                    {
                        btmesh.write(vertex.getX());
                        btmesh.write(vertex.getY());
                        btmesh.write(vertex.getZ());
                    }
                });
                btmesh.write<size_t>(0);
                btmesh.write<size_t>(0);
                btmesh.write_offset(16, [&btmesh, &mesh_cache]
                {
                    btmesh.write<int>(mesh_cache.type_and_flags);
                });
            }
            else
            {
                btIndexedMesh indexed_mesh;
                indexed_mesh.m_numTriangles = mesh_cache.indices.size() / 3;
                indexed_mesh.m_triangleIndexBase = (const unsigned char*)mesh_cache.indices.data();
                indexed_mesh.m_triangleIndexStride = sizeof(short) * 3;
                indexed_mesh.m_numVertices = mesh_cache.vertices.size();
                indexed_mesh.m_vertexBase = (const unsigned char*)mesh_cache.vertices.data();
                indexed_mesh.m_vertexStride = sizeof(btVector3);
                indexed_mesh.m_indexType = PHY_SHORT;
                indexed_mesh.m_vertexType = PHY_FLOAT;

                btVector3 aabb_min(FLT_MAX, FLT_MAX, FLT_MAX);
                btVector3 aabb_max(FLT_MIN, FLT_MIN, FLT_MIN);

                for (auto& vertex : mesh_cache.vertices)
                {
                    aabb_min.setMin(vertex);
                    aabb_max.setMax(vertex);
                }

                btTriangleIndexVertexArray vertex_array;
                vertex_array.addIndexedMesh(indexed_mesh, PHY_SHORT);

                btOptimizedBvh bvh;
                bvh.build(&vertex_array, true, aabb_min, aabb_max);

                int bvh_size = bvh.calculateSerializeBufferSize();
                mesh_cache.serialized_bvh = std::make_unique<uint8_t[]>(bvh_size);
                bvh.serializeInPlace(mesh_cache.serialized_bvh.get(), bvh_size, false);

                btmesh.write<int>(1); // u16_index_format
                btmesh.write<int>(mesh_cache.layer);
                btmesh.write<int>(mesh_cache.vertices.size());
                btmesh.write<int>(mesh_cache.indices.size() / 3);
                btmesh.write<int>(bvh_size);
                btmesh.write<int>(mesh_cache.indices.size() / 3);
                btmesh.write<int>(0);
                btmesh.write<int>(0);
                btmesh.write_offset(16, [&btmesh, &mesh_cache]
                {
                    for (auto& vertex : mesh_cache.vertices)
                    {
                        btmesh.write(vertex.getX());
                        btmesh.write(vertex.getY());
                        btmesh.write(vertex.getZ());
                    }
                });
                btmesh.write_offset(16, [&btmesh, &mesh_cache]
                {
                    btmesh.write(mesh_cache.indices.data(), mesh_cache.indices.size() * sizeof(short));
                });
                btmesh.write_offset(16, [&btmesh, &mesh_cache, bvh_size]
                {
                    btmesh.write(mesh_cache.serialized_bvh.get(), bvh_size);
                });
                btmesh.write_offset(16, [&btmesh, &mesh_cache]
                {
                    for (int j = 0; j < mesh_cache.indices.size() / 3; j++)
                    {
                        btmesh.write<int>(mesh_cache.type_and_flags);
                    }
                });
            }
        }
    });
    btmesh.write<int>(mesh_caches.size());
    btmesh.close();

    std::string pccol_file_path = dir_path + file_name_no_ext + ".pccol";
    bina pccol(pccol_file_path.c_str());

    pccol.write<int>(0x43495043);
    pccol.write<int>(2);
    pccol.write_offset(8, [&]
    {
        pccol.write_string_offset(file_name_no_ext);
        pccol.write_string_offset(file_name_no_ext);
        pccol.write<float>(0);
        pccol.write<float>(0);
        pccol.write<float>(0);
        pccol.write<float>(0);
        pccol.write<float>(0);
        pccol.write<float>(0);
        pccol.write<int>(1);
        pccol.write<float>(1);
        pccol.write<float>(1);
        pccol.write<float>(1);
        pccol.write<int>(0);
    });
    pccol.write<int>(1);
    pccol.close();

    return 0;
}