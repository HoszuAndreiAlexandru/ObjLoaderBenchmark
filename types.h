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

#define NOMINMAX
#include <windows.h>

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