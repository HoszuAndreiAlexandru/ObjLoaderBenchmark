#pragma once

#include "../types.h"

inline void appendFaceTriangles(
    const std::vector<int>& faceIndices,
    const std::vector<Vertex>& vertices,
    std::vector<int>& indices)
{
    if (faceIndices.size() < 3)
        return;

    if (faceIndices.size() == 4)
    {
        const vec3 diagonal02 = vertices[faceIndices[2]].pos - vertices[faceIndices[0]].pos;
        const vec3 diagonal13 = vertices[faceIndices[3]].pos - vertices[faceIndices[1]].pos;
        const float length02 = vec3::dot(diagonal02, diagonal02);
        const float length13 = vec3::dot(diagonal13, diagonal13);

        if (length02 < length13)
        {
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[2]);
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[2]);
            indices.push_back(faceIndices[3]);
        }
        else
        {
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[3]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[2]);
            indices.push_back(faceIndices[3]);
        }
        return;
    }

    for (size_t i = 2; i < faceIndices.size(); ++i)
    {
        indices.push_back(faceIndices[0]);
        indices.push_back(faceIndices[i - 1]);
        indices.push_back(faceIndices[i]);
    }
}
