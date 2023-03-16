#include "bina.h"

static const std::pair<const char*, int> MATERIAL_TYPES[] =
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

static const std::pair<const char*, int> MATERIAL_FLAGS[] =
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
    { "@PRESS_DEAD", 0xC },
    { "@RAYBLOCK", 0xD },
    { "@WALLJUMP", 0xE },
    { "@PUSH_BOX", 0xF },
    { "@STRIDER_FLOOR", 0x10 },
    { "@GIANT_TOWER", 0x11 },
    { "@TEST_GRASS", 0x14 },
    { "@TEST_WATER", 0x15 }
};

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

    const aiScene* ai_scene = importer.ReadFile(argv[1], 
        aiProcess_Triangulate | 
        aiProcess_SortByPType | 
        aiProcess_JoinIdenticalVertices | 
        aiProcess_SplitLargeMeshes |
        aiProcess_RemoveComponent | 
        aiProcess_PreTransformVertices
    );

    struct ai_mesh_cache
    {
        std::vector<btVector3> vertices;
        std::vector<short> indices;
        int material_type_and_flags = 0;
        std::unique_ptr<uint8_t[]> serialized_bvh;
    };

    std::vector<ai_mesh_cache> mesh_caches;

    for (int i = 0; i < ai_scene->mNumMeshes; i++)
    {
        aiMesh* ai_mesh = ai_scene->mMeshes[i];
        ai_mesh_cache mesh_cache;

        for (int j = 0; j < ai_mesh->mNumVertices; j++)
        {
            auto& vertex = ai_mesh->mVertices[j];
            mesh_cache.vertices.emplace_back(vertex.x, vertex.y, vertex.z);
        }

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

        if (!mesh_cache.vertices.empty() && !mesh_cache.indices.empty())
        {
            printf("%s\n", ai_mesh->mName.C_Str());

            for (auto& [name, value] : MATERIAL_TYPES)
            {
                if (strstr(ai_mesh->mName.C_Str(), name) != nullptr)
                {
                    mesh_cache.material_type_and_flags = value << 24;
                }
            }

            for (auto& [name, value] : MATERIAL_FLAGS)
            {
                if (strstr(ai_mesh->mName.C_Str(), name) != nullptr)
                {
                    mesh_cache.material_type_and_flags |= 1 << value;
                }
            }

            mesh_caches.push_back(std::move(mesh_cache));
        }
    }

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
    btmesh.write_offset(16, [&]
    {
        for (auto& mesh_cache : mesh_caches)
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

            btmesh.write<int>(1);
            btmesh.write<int>(1);
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
                    btmesh.write<int>(mesh_cache.material_type_and_flags);
                }
            });
        }
    });
    btmesh.write<int>(mesh_caches.size());

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

    return 0;
}