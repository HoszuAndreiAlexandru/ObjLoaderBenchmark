#pragma once

#include "loader_template.h"

//#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
//#endif
#pragma optimize( "", off )
#include "../Externals/fast_obj.h"
#pragma optimize( "", on ) 

#pragma region Helper functions

#pragma endregion

class FastObj : public  LoaderTemplate
{
public:
    const char* Name() const override
    {
        return "fast obj";
    }

    Mesh loadObjImplementation(const std::string& filename) override
    {
        fastObjMesh* mesh = fast_obj_read(filename.c_str());
        if (!mesh) return Mesh({}, {});

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(mesh->index_count); // each index becomes a vertex
        indices.reserve(mesh->index_count);

        for (size_t i = 0; i < mesh->index_count; ++i)
        {
            fastObjIndex idx = mesh->indices[i];

            vec3 pos(mesh->positions[3 * idx.p + 0],
                mesh->positions[3 * idx.p + 1],
                mesh->positions[3 * idx.p + 2]);

            vec3 norm(0, 0, 0);
            vec2 uv(0, 0);

            if (idx.n >= 0)
                norm = vec3(mesh->normals[3 * idx.n + 0],
                    mesh->normals[3 * idx.n + 1],
                    mesh->normals[3 * idx.n + 2]);

            if (idx.t >= 0)
                uv = vec2(mesh->texcoords[2 * idx.t + 0],
                    mesh->texcoords[2 * idx.t + 1]);

            // create a Vertex for this index (no deduplication)
            if (idx.n >= 0 && idx.t >= 0)
                vertices.emplace_back(pos, norm, uv);
            else if (idx.n >= 0)
                vertices.emplace_back(pos, norm);
            else if (idx.t >= 0)
                vertices.emplace_back(pos, uv);
            else
                vertices.emplace_back(pos);

            indices.push_back((unsigned int)i); // direct mapping
        }

        fast_obj_destroy(mesh);
        return Mesh(vertices, indices);
    }
};