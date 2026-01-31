#pragma once

#include "loader_template.h"
#include "../Externals/fast_float.h"

#pragma region Helper functions
static inline int parseInt(const char* s, size_t n)
{
    int sign = 1;
    if (*s == '-')
    {
        sign = -1; ++s; --n;
    }

    int value = 0;
    while (n-- && *s >= '0' && *s <= '9')
    {
        value = value * 10 + (*s - '0');
        ++s;
    }
    return value * sign;
}

static inline float parseFloat(const char* s, size_t n)
{
    int sign = 1;
    if (*s == '-')
    {
        sign = -1; ++s; --n;
    }

    float value = 0.0f;
    while (n && *s >= '0' && *s <= '9')
    {
        value = value * 10.0f + (*s - '0');
        ++s; --n;
    }

    if (n && *s == '.')
    {
        ++s; --n;
        float frac = 0.0f;
        float base = 0.1f;
        while (n && *s >= '0' && *s <= '9')
        {
            frac += (*s - '0') * base;
            base *= 0.1f;
            ++s; --n;
        }
        value += frac;
    }

    return value * sign;
}

static inline int resolveIndex(int idx, int size)
{
    if (idx == 0) return -1; // invalid OBJ index

    int resolved = (idx > 0) ? (idx - 1) : (size + idx);

    if (resolved < 0 || resolved >= size)
        return -1; // invalid after resolving

    return resolved;
}
#pragma endregion

#pragma region Vertex cache
struct VertexKey {
    int p, t, n;
    bool operator==(VertexKey const& o) const {
        return p == o.p && t == o.t && n == o.n;
    }
};

struct FastVertexCache {
    struct Entry {
        VertexKey key;
        int index;
        bool used = false;
    };

    std::vector<Entry> table;
    size_t capacity;
    size_t count;

    FastVertexCache(size_t cap = 1 << 20) {
        capacity = 1;
        while (capacity < cap) capacity <<= 1;
        table.resize(capacity);
        count = 0;
    }

    inline size_t hash(const VertexKey& k) const {
        return ((size_t)k.p * 73856093u) ^
            ((size_t)k.t * 19349663u) ^
            ((size_t)k.n * 83492791u);
    }

    void rehash() {
        size_t newCapacity = capacity * 2;
        std::vector<Entry> newTable(newCapacity);

        for (auto& e : table) {
            if (!e.used) continue;

            size_t idx = hash(e.key) & (newCapacity - 1);

            while (newTable[idx].used) {
                idx = (idx + 1) & (newCapacity - 1);
            }

            newTable[idx] = e;
        }

        table.swap(newTable);
        capacity = newCapacity;
    }

    inline int findOrInsert(const VertexKey& key, int newIndex, bool& inserted) {
        if (count * 10 >= capacity * 7) {
            // load factor > 0.7
            rehash();
        }

        size_t idx = hash(key) & (capacity - 1);

        while (true) {
            if (!table[idx].used) {
                table[idx].used = true;
                table[idx].key = key;
                table[idx].index = newIndex;
                count++;
                inserted = true;
                return newIndex;
            }

            if (table[idx].key.p == key.p &&
                table[idx].key.t == key.t &&
                table[idx].key.n == key.n) {
                inserted = false;
                return table[idx].index;
            }

            idx = (idx + 1) & (capacity - 1);
        }
    }
};
#pragma endregion

class NewFast : public  LoaderTemplate
{
private:
    static void addVertex(std::vector<Vertex>& vertices,
        const std::vector<vec3>& positions,
        const std::vector<vec3>& normals,
        const std::vector<vec2>& texcoords,
        int pIdx, int nIdx, int tIdx)
    {
        if (tIdx >= 0 && nIdx >= 0)
        {
            vertices.emplace_back(positions[pIdx], normals[nIdx], texcoords[tIdx]);
        }
        else if (tIdx >= 0)
        {
            vertices.emplace_back(positions[pIdx], texcoords[tIdx]);
        }
        else if (nIdx >= 0)
        {
            vertices.emplace_back(positions[pIdx], normals[nIdx]);
        }
        else
        {
            vertices.emplace_back(positions[pIdx]);
        }
    }
public:
    const char* Name() const override
    {
        static std::string nameDedup = "new fast with vertex dedup";
        static std::string nameRaw = "new fast";

        return deduplicateVertices ? nameDedup.c_str() : nameRaw.c_str();
    }

    Mesh loadObjImplementation(const std::string& filename) override
    {
        MappedFile file;
        if (!file.open(filename)) {
            std::cout << "Failed to open file\n";
            std::terminate();
        }

        const char* data = file.data;
        const char* end = data + file.size;

        size_t size = file.size;

        static std::vector<Vertex> vertices;
        static std::vector<unsigned int> indices;
        static std::vector<vec3> positions;
        static std::vector<vec3> normals;
        static std::vector<vec2> texcoords;

        vertices.clear();
        indices.clear();
        positions.clear();
        normals.clear();
        texcoords.clear();

        positions.reserve(size / 20);
        normals.reserve(size / 40);
        texcoords.reserve(size / 40);
        vertices.reserve(size / 10);
        indices.reserve(size / 5);

        FastVertexCache cache(1 << 20);

        while (data < end)
        {
            const char* lineStart = data;
            while (data < end && *data != '\n') data++;
            const char* lineEnd = data;

            if (data < end) data++; // skip newline
            if (lineEnd == lineStart) continue;
            if (*lineStart == '#') continue;

            // Parse vertices, normals, texcoords
            if (lineStart[0] == 'v' && (lineStart[1] == ' ' || lineStart[1] == '\t'))
            {
                const char* p = lineStart + 1;
                while (*p == ' ' || *p == '\t') ++p;
                const char* a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float x = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, x);
                while (p < lineEnd && (*p == ' ' || *p == '\t')) ++p;
                a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float y = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, y);
                while (p < lineEnd && (*p == ' ' || *p == '\t')) ++p;
                a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float z = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, z);
                positions.emplace_back(x, y, z);
            }
            else if (lineStart[0] == 'v' && lineStart[1] == 'n')
            {
                const char* p = lineStart + 2;
                while (*p == ' ' || *p == '\t') ++p;
                const char* a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float x = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, x);
                while (p < lineEnd && (*p == ' ' || *p == '\t')) ++p;
                a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float y = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, y);
                while (p < lineEnd && (*p == ' ' || *p == '\t')) ++p;
                a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float z;// = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, z);
                normals.emplace_back(x, y, z);
            }
            else if (lineStart[0] == 'v' && lineStart[1] == 't')
            {
                const char* p = lineStart + 2;
                while (*p == ' ' || *p == '\t') ++p;
                const char* a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float u = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, u);
                while (p < lineEnd && (*p == ' ' || *p == '\t')) ++p;
                a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                float v = parseFloat(a, p - a);
                //fast_float::from_chars(a, p, v);
                texcoords.emplace_back(u, v);
            }
            else if (lineStart[0] == 'f')
            {
                const char* p = lineStart + 1;
                while (*p == ' ' || *p == '\t') ++p;

                int firstIndex = -1, prevIndex = -1;

                while (p < lineEnd)
                {
                    int pIdx = 0, tIdx = 0, nIdx = 0;
                    const char* a = p; while (p < lineEnd && *p != '/' && *p != ' ' && *p != '\t') ++p;
                    pIdx = parseInt(a, p - a);

                    if (p < lineEnd && *p == '/')
                    {
                        ++p;
                        if (*p == '/')
                        {
                            ++p; a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                            nIdx = parseInt(a, p - a);
                        }
                        else
                        {
                            a = p; while (p < lineEnd && *p != '/' && *p != ' ' && *p != '\t') ++p;
                            tIdx = parseInt(a, p - a);
                            if (p < lineEnd && *p == '/')
                            {
                                ++p; a = p; while (p < lineEnd && *p != ' ' && *p != '\t') ++p;
                                nIdx = parseInt(a, p - a);
                            }
                        }
                    }

                    pIdx = resolveIndex(pIdx, (int)positions.size());
                    tIdx = (tIdx != 0) ? resolveIndex(tIdx, (int)texcoords.size()) : -1;
                    nIdx = (nIdx != 0) ? resolveIndex(nIdx, (int)normals.size()) : -1;

                    if (pIdx < 0) continue; // skip malformed

                    int finalIndex = 0;

                    if (deduplicateVertices)
                    {
                        VertexKey key{ pIdx, tIdx, nIdx };
                        bool inserted;
                        finalIndex = cache.findOrInsert(key, (int)vertices.size(), inserted);

                        if (inserted)
                        {
                            addVertex(vertices, positions, normals, texcoords, pIdx, nIdx, tIdx);
                        }
                    }
                    else
                    {
                        finalIndex = (int)vertices.size();
                        addVertex(vertices, positions, normals, texcoords, pIdx, nIdx, tIdx);
                    }

                    if (firstIndex < 0) firstIndex = finalIndex;
                    else if (prevIndex >= 0)
                    {
                        indices.push_back(firstIndex);
                        indices.push_back(prevIndex);
                        indices.push_back(finalIndex);
                    }

                    prevIndex = finalIndex;

                    while (p < lineEnd && (*p == ' ' || *p == '\t')) ++p;
                }
            }
        }

        return Mesh(vertices, indices);
    }
};