#pragma once

#include "loader_template.h"
#include "../Utils/triangulation.h"

#pragma region Helper functions
float _stringToFloat(const std::string& source) {
    std::stringstream ss(source.c_str());
    float result;
    ss >> result;
    return result;
}

unsigned int _stringToUint(const std::string& source) {
    std::stringstream ss(source.c_str());
    unsigned int result;
    ss >> result;
    return result;
}

int _stringToInt(const std::string& source) {
    std::stringstream ss(source.c_str());
    int result;
    ss >> result;
    return result;
}

void _stringTokenize(const std::string& source, std::vector<std::string>& tokens) {
    tokens.clear();
    std::string aux = source;
    for (unsigned int i = 0; i < aux.size(); i++) if (aux[i] == '\t' || aux[i] == '\n') aux[i] = ' ';
    std::stringstream ss(aux, std::ios::in);
    while (ss.good()) {
        std::string s;
        ss >> s;
        if (s.size() > 0) tokens.push_back(s);
    }
}

void _faceTokenize(const std::string& source, std::vector<std::string>& tokens) {
    std::string aux = source;
    for (unsigned int i = 0; i < aux.size(); i++) if (aux[i] == '\\' || aux[i] == '/') aux[i] = ' ';
    _stringTokenize(aux, tokens);
}
#pragma endregion

class Naive : public  LoaderTemplate
{
    public:
        const char* Name() const override
        { 
            return "naive"; 
        }

        Mesh loadObjImplementation(const std::string& filename) override
        {
            std::vector<Vertex> vertices;
            std::vector<int> indices;

            std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
            if (!file.good())
            {
                std::cout << "Obj model not found " << filename << std::endl;
                std::terminate();
            }

            std::string line;
            std::vector<std::string> tokens, facetokens;
            std::vector<int> faceIndices;
            faceIndices.reserve(4);

            std::vector<vec3> positions;
            positions.reserve(1000);

            std::vector<vec3> normals;
            normals.reserve(1000);

            std::vector<vec2> texcoords;
            texcoords.reserve(1000);

            while (std::getline(file, line))
            {
                _stringTokenize(line, tokens);

                if (tokens.size() == 0)
                    continue;

                if (tokens[0].at(0) == '#')
                    continue;

                if (tokens.size() > 3 && tokens[0] == "v")
                    positions.push_back(vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

                if (tokens.size() > 3 && tokens[0] == "vn")
                    normals.push_back(vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

                if (tokens.size() > 2 && tokens[0] == "vt")
                    texcoords.push_back(vec2(_stringToFloat(tokens[1]), _stringToFloat(tokens[2])));

                if (tokens.size() >= 4 && tokens[0] == "f")
                {
                    unsigned int face_format = 0;
                    if (tokens[1].find("//") != std::string::npos) face_format = 3;
                    _faceTokenize(tokens[1], facetokens);

                    if (facetokens.size() == 3)
                        face_format = 4;
                    else
                    {
                        if (facetokens.size() == 2)
                        {
                            if (face_format != 3) face_format = 2;
                        }
                        else
                        {
                            face_format = 1;
                        }
                    }

                    faceIndices.clear();

                    for (unsigned int num_token = 1; num_token < tokens.size(); num_token++)
                    {
                        if (tokens[num_token].at(0) == '#') break;
                        _faceTokenize(tokens[num_token], facetokens);

                        if (face_format == 1)
                        {
                            int p_index = _stringToInt(facetokens[0]);
                            if (p_index > 0) p_index -= 1;
                            else p_index = positions.size() + p_index;

                            vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z));
                        }
                        else if (face_format == 2)
                        {
                            int p_index = _stringToInt(facetokens[0]);
                            if (p_index > 0) p_index -= 1;
                            else p_index = positions.size() + p_index;

                            int t_index = _stringToInt(facetokens[1]);
                            if (t_index > 0) t_index -= 1;
                            else t_index = texcoords.size() + t_index;

                            vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, texcoords[t_index].x, texcoords[t_index].y));
                        }
                        else if (face_format == 3)
                        {
                            int p_index = _stringToInt(facetokens[0]);
                            if (p_index > 0) p_index -= 1;
                            else p_index = positions.size() + p_index;

                            int n_index = _stringToInt(facetokens[1]);
                            if (n_index > 0) n_index -= 1;
                            else n_index = normals.size() + n_index;

                            vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z));
                        }
                        else
                        {
                            int p_index = _stringToInt(facetokens[0]);
                            if (p_index > 0) p_index -= 1;
                            else p_index = positions.size() + p_index;

                            int t_index = _stringToInt(facetokens[1]);
                            if (t_index > 0) t_index -= 1;
                            else t_index = normals.size() + t_index;

                            int n_index = _stringToInt(facetokens[2]);
                            if (n_index > 0) n_index -= 1;
                            else n_index = normals.size() + n_index;

                            vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z, texcoords[t_index].x, texcoords[t_index].y));
                        }

                        faceIndices.push_back(checkedMeshIndex(vertices.size() - 1));
                    }

                    appendFaceTriangles(faceIndices, vertices, indices);
                }
            }

            Mesh mesh(vertices, indices);
            return mesh;
        }
};
