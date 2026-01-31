#pragma once

#include "loader_template.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../Externals/tiny_obj_loader.h"

#pragma region Helper functions

#pragma endregion

class TinyObjLoader : public  LoaderTemplate
{
public:
    const char* Name() const override
    {
        return "tiny obj loader";
    }

    Mesh loadObjImplementation(const std::string& filename) override
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        bool ret = tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            filename.c_str(),
            nullptr,
            true
        );

        if (!ret) {
            if (!err.empty()) std::cerr << "TinyObj error: " << err << std::endl;
            std::terminate();
        }

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // Pre-allocate based on total number of indices for speed
        size_t totalIndices = 0;
        for (const auto& shape : shapes)
            totalIndices += shape.mesh.indices.size();

        vertices.reserve(totalIndices);
        indices.reserve(totalIndices);

        // Directly create vertices for each index
        for (const auto& shape : shapes)
        {
            for (const auto& idx : shape.mesh.indices)
            {
                Vertex v;

                // Position
                int vp = 3 * idx.vertex_index;
                v.pos = vec3(
                    attrib.vertices[vp + 0],
                    attrib.vertices[vp + 1],
                    attrib.vertices[vp + 2]
                );

                // Normal
                if (idx.normal_index >= 0)
                {
                    int np = 3 * idx.normal_index;
                    v.normals = vec3(
                        attrib.normals[np + 0],
                        attrib.normals[np + 1],
                        attrib.normals[np + 2]
                    );
                }

                // Texcoord
                if (idx.texcoord_index >= 0)
                {
                    int tp = 2 * idx.texcoord_index;
                    v.textureCoords = vec2(
                        attrib.texcoords[tp + 0],
                        attrib.texcoords[tp + 1]
                    );
                }

                vertices.push_back(v);
                indices.push_back((unsigned int)(vertices.size() - 1)); // direct mapping
            }
        }

        return Mesh(vertices, indices);
    }
};