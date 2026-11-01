#pragma once

#include <fstream>
#include <sstream>
#include <iostream>

#include <chrono>
#include <string>
#include <limits>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

void writeNewLine(const char* text)
{
    std::cout << "\n" << text << "\n";
};

struct vec2
{
    float x, y;

    // Constructors
    vec2() : x(0), y(0) {}
    vec2(float v) : x(v), y(v) {}
    vec2(float x_, float y_) : x(x_), y(y_) {}

    // Operators
    vec2 operator + (const vec2& v) const { return { x + v.x, y + v.y }; }
    vec2 operator - (const vec2& v) const { return { x - v.x, y - v.y }; }
    vec2 operator * (float s) const { return { x * s, y * s }; }
    vec2 operator / (float s) const { return { x / s, y / s }; }

    vec2& operator += (const vec2& v) { x += v.x; y += v.y; return *this; }
    vec2& operator -= (const vec2& v) { x -= v.x; y -= v.y; return *this; }
    vec2& operator *= (float s) { x *= s; y *= s; return *this; }
    vec2& operator /= (float s) { x /= s; y /= s; return *this; }

    // Util functions
    float length() const
    {
        return std::sqrt(x * x + y * y);
    }

    vec2 normalized() const
    {
        float len = length();
        return len > 0 ? *this / len : vec2(0);
    }

    static float dot(const vec2& a, const vec2& b)
    {
        return a.x * b.x + a.y * b.y;
    }
};

struct vec3
{
    float x, y, z;

    // Constructors
    vec3() : x(0), y(0), z(0) {}
    vec3(float v) : x(v), y(v), z(v) {}
    vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    // Operators
    vec3 operator + (const vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    vec3 operator - (const vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    vec3 operator * (float s) const { return { x * s, y * s, z * s }; }
    vec3 operator / (float s) const { return { x / s, y / s, z / s }; }

    vec3& operator += (const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec3& operator -= (const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec3& operator *= (float s) { x *= s; y *= s; z *= s; return *this; }
    vec3& operator /= (float s) { x /= s; y /= s; z /= s; return *this; }

    // Util functions
    float length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    vec3 normalized() const
    {
        float len = length();
        return len > 0 ? *this / len : vec3(0);
    }

    static float dot(const vec3& a, const vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static vec3 cross(const vec3& a, const vec3& b)
    {
        return
        {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

struct Vertex
{
	vec3 pos;
	vec3 normals;
	vec2 textureCoords;

    // Constructors
	Vertex() = default;

	Vertex(float pos_x, float pos_y, float pos_z)
	{
		pos.x = pos_x;
		pos.y = pos_y;
		pos.z = pos_z;
	}

	Vertex(float pos_x, float pos_y, float pos_z, float norm_x, float norm_y, float norm_z)
	{
		pos.x = pos_x;
		pos.y = pos_y;
		pos.z = pos_z;

		normals.x = norm_x;
		normals.y = norm_y;
		normals.z = norm_z;
	}

	Vertex(float pos_x, float pos_y, float pos_z, float text_x, float text_y)
	{
		pos.x = pos_x;
		pos.y = pos_y;
		pos.z = pos_z;

		textureCoords.x = text_x;
		textureCoords.y = text_y;
	}

	Vertex(float pos_x, float pos_y, float pos_z, float norm_x, float norm_y, float norm_z, float text_x, float text_y)
	{
		pos.x = pos_x;
		pos.y = pos_y;
		pos.z = pos_z;

		normals.x = norm_x;
		normals.y = norm_y;
		normals.z = norm_z;

		textureCoords.x = text_x;
		textureCoords.y = text_y;
	}

	Vertex(const vec3& p)
		: pos(p), normals(0.0f), textureCoords(0.0f) 
    {}

	Vertex(const vec3& p, const vec3& n)
		: pos(p), normals(n), textureCoords(0.0f) 
    {}

	Vertex(const vec3& p, const vec2& t)
		: pos(p), normals(0.0f), textureCoords(t) 
    {}

	Vertex(const vec3& p, const vec3& n, const vec2& t)
		: pos(p), normals(n), textureCoords(t) 
    {}
};

class Mesh
{
	public:
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

        // Constructors
		Mesh() {};

		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
		{
			this->vertices = vertices;
			this->indices = indices;
		};

		~Mesh() {};
};

struct Result
{
    Mesh mesh;
    std::chrono::milliseconds elapsed;
};

struct Results
{
    const char* implementationName;
    std::vector<Result> data;
};

struct ImplSummary
{
    const char* name;
    size_t totalVertices;
    size_t totalIndices;
    std::chrono::milliseconds totalTime;
};

struct MappedFile {
#if defined(_WIN32)
    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = nullptr;
#else
    int file = -1;
#endif
    const char* data = nullptr;
    size_t size = 0;
    bool isMapped = false;

    MappedFile() = default;
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    bool open(const std::string& path) {
        close();

#if defined(_WIN32)
        file = CreateFileA(
            path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (file == INVALID_HANDLE_VALUE)
            return false;

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(file, &fileSize))
        {
            close();
            return false;
        }

        if (fileSize.QuadPart < 0 ||
            static_cast<std::uintmax_t>(fileSize.QuadPart) > (std::numeric_limits<size_t>::max)())
        {
            close();
            return false;
        }

        size = static_cast<size_t>(fileSize.QuadPart);
        if (size == 0)
        {
            data = emptyBuffer();
            return true;
        }

        mapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!mapping)
        {
            close();
            return false;
        }

        data = (const char*)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
        if (!data)
        {
            close();
            return false;
        }

        isMapped = true;
        return true;
#else
        file = ::open(path.c_str(), O_RDONLY);
        if (file < 0)
            return false;

        struct stat fileInfo;
        if (fstat(file, &fileInfo) != 0 || fileInfo.st_size < 0 ||
            static_cast<std::uintmax_t>(fileInfo.st_size) > (std::numeric_limits<size_t>::max)())
        {
            close();
            return false;
        }

        size = static_cast<size_t>(fileInfo.st_size);
        if (size == 0)
        {
            data = emptyBuffer();
            return true;
        }

        void* view = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file, 0);
        if (view == MAP_FAILED)
        {
            close();
            return false;
        }

        data = static_cast<const char*>(view);
        isMapped = true;
        return true;
#endif
    }

    void close() {
#if defined(_WIN32)
        if (isMapped && data) UnmapViewOfFile(data);
        if (mapping) CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
#else
        if (isMapped && data) munmap(const_cast<char*>(data), size);
        if (file >= 0) ::close(file);
#endif
        data = nullptr;
        size = 0;
        isMapped = false;
#if defined(_WIN32)
        mapping = nullptr;
        file = INVALID_HANDLE_VALUE;
#else
        file = -1;
#endif
    }

    ~MappedFile() {
        close();
    }

private:
    static const char* emptyBuffer()
    {
        static const char empty = '\0';
        return &empty;
    }
};
