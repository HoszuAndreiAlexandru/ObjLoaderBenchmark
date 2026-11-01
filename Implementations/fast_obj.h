#pragma once

#include "loader_template.h"
#include "../Utils/triangulation.h"

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
        std::vector<int> indices;
        std::vector<int> faceIndices;

        vertices.reserve(mesh->index_count); // each index becomes a vertex
        indices.reserve(mesh->index_count);
        faceIndices.reserve(4);

        size_t indexOffset = 0;
        for (size_t face = 0; face < mesh->face_count; ++face)
        {
            faceIndices.clear();
            const unsigned int faceVertexCount = mesh->face_vertices[face];

            for (unsigned int corner = 0; corner < faceVertexCount; ++corner)
            {
                fastObjIndex idx = mesh->indices[indexOffset++];

                vec3 pos(mesh->positions[3 * idx.p + 0],
                    mesh->positions[3 * idx.p + 1],
                    mesh->positions[3 * idx.p + 2]);

                vec3 norm(0, 0, 0);
                vec2 uv(0, 0);

                if (idx.n > 0)
                    norm = vec3(mesh->normals[3 * idx.n + 0],
                        mesh->normals[3 * idx.n + 1],
                        mesh->normals[3 * idx.n + 2]);

                if (idx.t > 0)
                    uv = vec2(mesh->texcoords[2 * idx.t + 0],
                        mesh->texcoords[2 * idx.t + 1]);

                if (idx.n > 0 && idx.t > 0)
                    vertices.emplace_back(pos, norm, uv);
                else if (idx.n > 0)
                    vertices.emplace_back(pos, norm);
                else if (idx.t > 0)
                    vertices.emplace_back(pos, uv);
                else
                    vertices.emplace_back(pos);

                faceIndices.push_back(checkedMeshIndex(vertices.size() - 1));
            }

            appendFaceTriangles(faceIndices, vertices, indices);
        }

        fast_obj_destroy(mesh);
        return Mesh(vertices, indices);
    }
};
