#pragma once

#include "../types.h"
#include "consoleColors.h"

#include <array>
#include <cstdint>

namespace MeshComparator
{
    // OBJ parsers can round decimal values slightly differently. Quantizing
    // attributes to this step makes the comparison tolerant to those differences.
    static const double attributeTolerance = 0.00001;

    using TriangleFingerprint = std::array<std::uint64_t, 3>;

    struct MeshSignature
    {
        size_t triangleCount = 0;
        size_t trailingIndexCount = 0;
        size_t invalidIndexCount = 0;
        std::vector<TriangleFingerprint> triangles;
    };

    struct MeshComparison
    {
        size_t mismatchedTriangles = 0;
        size_t mismatchedVertices = 0;
        size_t firstDifference = (std::numeric_limits<size_t>::max)();
    };

    struct AttributeStats
    {
        size_t comparedComponents = 0;
        size_t mismatchedComponents = 0;
        double maxAbsoluteDelta = 0.0;
        long double totalAbsoluteDelta = 0.0;
    };

    struct AttributeComparison
    {
        AttributeStats positions;
        AttributeStats normals;
        AttributeStats textureCoords;
        size_t comparedVertices = 0;
        size_t mismatchedVertices = 0;
        size_t unmatchedReferenceVertices = 0;
        size_t unmatchedCandidateVertices = 0;
    };

    static std::uint64_t mix(std::uint64_t value)
    {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31);
    }

    static std::int64_t quantize(float value)
    {
        if (std::isnan(value))
            return (std::numeric_limits<std::int64_t>::min)();
        if (std::isinf(value))
            return value < 0 ? (std::numeric_limits<std::int64_t>::min)() + 1
                             : (std::numeric_limits<std::int64_t>::max)();

        const double scaled = static_cast<double>(value) / attributeTolerance;
        if (scaled >= static_cast<double>((std::numeric_limits<std::int64_t>::max)()))
            return (std::numeric_limits<std::int64_t>::max)();
        if (scaled <= static_cast<double>((std::numeric_limits<std::int64_t>::min)() + 2))
            return (std::numeric_limits<std::int64_t>::min)() + 2;

        return static_cast<std::int64_t>(std::round(scaled));
    }

    static std::uint64_t fingerprintVertex(const Vertex& vertex)
    {
        const std::array<std::int64_t, 8> attributes = {
            quantize(vertex.pos.x), quantize(vertex.pos.y), quantize(vertex.pos.z),
            quantize(vertex.normals.x), quantize(vertex.normals.y), quantize(vertex.normals.z),
            quantize(vertex.textureCoords.x), quantize(vertex.textureCoords.y)
        };

        std::uint64_t fingerprint = 0x243f6a8885a308d3ULL;
        for (size_t i = 0; i < attributes.size(); ++i)
        {
            const std::uint64_t value = static_cast<std::uint64_t>(attributes[i]);
            fingerprint = mix(fingerprint ^ mix(value + i));
        }
        return fingerprint;
    }

    static bool triangleFingerprintLess(const TriangleFingerprint& left, const TriangleFingerprint& right)
    {
        for (size_t i = 0; i < left.size(); ++i)
        {
            if (left[i] != right[i])
                return left[i] < right[i];
        }
        return false;
    }

    // Choose a stable cyclic rotation while preserving triangle winding.
    static bool rotationLess(const std::uint64_t triangle[3], size_t left, size_t right)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            const std::uint64_t a = triangle[(left + i) % 3];
            const std::uint64_t b = triangle[(right + i) % 3];
            if (a < b) return true;
            if (b < a) return false;
        }
        return false;
    }

    static TriangleFingerprint fingerprintTriangle(const Vertex& a, const Vertex& b, const Vertex& c)
    {
        const std::uint64_t vertices[3] = {
            fingerprintVertex(a), fingerprintVertex(b), fingerprintVertex(c)
        };

        size_t first = 0;
        if (rotationLess(vertices, 1, first)) first = 1;
        if (rotationLess(vertices, 2, first)) first = 2;

        return {{ vertices[first], vertices[(first + 1) % 3], vertices[(first + 2) % 3] }};
    }

    static MeshSignature buildSignature(const Mesh& mesh)
    {
        MeshSignature signature;
        signature.triangleCount = mesh.indices.size() / 3;
        signature.trailingIndexCount = mesh.indices.size() % 3;
        signature.triangles.reserve(signature.triangleCount);

        for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
        {
            const int ia = mesh.indices[i];
            const int ib = mesh.indices[i + 1];
            const int ic = mesh.indices[i + 2];
            if (ia < 0 || static_cast<size_t>(ia) >= mesh.vertices.size()) ++signature.invalidIndexCount;
            if (ib < 0 || static_cast<size_t>(ib) >= mesh.vertices.size()) ++signature.invalidIndexCount;
            if (ic < 0 || static_cast<size_t>(ic) >= mesh.vertices.size()) ++signature.invalidIndexCount;
            if (ia < 0 || ib < 0 || ic < 0 ||
                static_cast<size_t>(ia) >= mesh.vertices.size() ||
                static_cast<size_t>(ib) >= mesh.vertices.size() ||
                static_cast<size_t>(ic) >= mesh.vertices.size())
                continue;

            signature.triangles.push_back(fingerprintTriangle(
                mesh.vertices[static_cast<size_t>(ia)],
                mesh.vertices[static_cast<size_t>(ib)],
                mesh.vertices[static_cast<size_t>(ic)]));
        }

        std::sort(signature.triangles.begin(), signature.triangles.end(), triangleFingerprintLess);
        return signature;
    }

    static MeshComparison compareSignatures(const MeshSignature& a, const MeshSignature& b)
    {
        MeshComparison comparison;
        std::vector<std::uint64_t> unmatchedVerticesA;
        std::vector<std::uint64_t> unmatchedVerticesB;
        size_t unmatchedTrianglesA = 0;
        size_t unmatchedTrianglesB = 0;
        size_t i = 0;
        size_t j = 0;

        while (i < a.triangles.size() && j < b.triangles.size())
        {
            if (a.triangles[i] == b.triangles[j])
            {
                ++i;
                ++j;
                continue;
            }

            if (comparison.firstDifference == (std::numeric_limits<size_t>::max)())
                comparison.firstDifference = (std::min)(i, j);

            const bool takeA = triangleFingerprintLess(a.triangles[i], b.triangles[j]);
            if (takeA)
            {
                const TriangleFingerprint& unmatched = a.triangles[i++];
                unmatchedVerticesA.insert(unmatchedVerticesA.end(), unmatched.begin(), unmatched.end());
                ++unmatchedTrianglesA;
            }
            else
            {
                const TriangleFingerprint& unmatched = b.triangles[j++];
                unmatchedVerticesB.insert(unmatchedVerticesB.end(), unmatched.begin(), unmatched.end());
                ++unmatchedTrianglesB;
            }
        }

        while (i < a.triangles.size())
        {
            if (comparison.firstDifference == (std::numeric_limits<size_t>::max)())
                comparison.firstDifference = i;
            unmatchedVerticesA.insert(unmatchedVerticesA.end(), a.triangles[i].begin(), a.triangles[i].end());
            ++unmatchedTrianglesA;
            ++i;
        }

        while (j < b.triangles.size())
        {
            if (comparison.firstDifference == (std::numeric_limits<size_t>::max)())
                comparison.firstDifference = j;
            unmatchedVerticesB.insert(unmatchedVerticesB.end(), b.triangles[j].begin(), b.triangles[j].end());
            ++unmatchedTrianglesB;
            ++j;
        }

        std::sort(unmatchedVerticesA.begin(), unmatchedVerticesA.end());
        std::sort(unmatchedVerticesB.begin(), unmatchedVerticesB.end());

        size_t vertexA = 0;
        size_t vertexB = 0;
        size_t remainingA = 0;
        size_t remainingB = 0;
        while (vertexA < unmatchedVerticesA.size() && vertexB < unmatchedVerticesB.size())
        {
            if (unmatchedVerticesA[vertexA] == unmatchedVerticesB[vertexB])
            {
                ++vertexA;
                ++vertexB;
            }
            else if (unmatchedVerticesA[vertexA] < unmatchedVerticesB[vertexB])
            {
                ++remainingA;
                ++vertexA;
            }
            else
            {
                ++remainingB;
                ++vertexB;
            }
        }

        remainingA += unmatchedVerticesA.size() - vertexA;
        remainingB += unmatchedVerticesB.size() - vertexB;
        comparison.mismatchedVertices = (std::max)(remainingA, remainingB);
        comparison.mismatchedTriangles = (std::max)(unmatchedTrianglesA, unmatchedTrianglesB);

        return comparison;
    }

    static double vertexPairScore(const Vertex& a, const Vertex& b)
    {
        const double aValues[] = {
            a.pos.x, a.pos.y, a.pos.z,
            a.normals.x, a.normals.y, a.normals.z,
            a.textureCoords.x, a.textureCoords.y
        };
        const double bValues[] = {
            b.pos.x, b.pos.y, b.pos.z,
            b.normals.x, b.normals.y, b.normals.z,
            b.textureCoords.x, b.textureCoords.y
        };

        double score = 0.0;
        for (size_t i = 0; i < 8; ++i)
        {
            const double delta = std::fabs(aValues[i] - bValues[i]);
            if (!std::isfinite(delta))
                return (std::numeric_limits<double>::infinity)();
            score += delta / attributeTolerance;
        }
        return score;
    }

    static void recordDelta(float referenceValue, float candidateValue, AttributeStats& stats, bool& vertexDiffers)
    {
        const double delta = std::fabs(static_cast<double>(referenceValue) - candidateValue);
        ++stats.comparedComponents;
        if (!std::isfinite(delta) || delta > attributeTolerance)
        {
            ++stats.mismatchedComponents;
            vertexDiffers = true;
        }

        if (!std::isfinite(delta))
            stats.maxAbsoluteDelta = (std::numeric_limits<double>::infinity)();
        else
            stats.maxAbsoluteDelta = (std::max)(stats.maxAbsoluteDelta, delta);
        stats.totalAbsoluteDelta += delta;
    }

    static AttributeComparison compareAttributesInTriangleOrder(const Mesh& reference, const Mesh& candidate)
    {
        AttributeComparison comparison;
        const size_t referenceTriangles = reference.indices.size() / 3;
        const size_t candidateTriangles = candidate.indices.size() / 3;
        const size_t pairedTriangles = (std::min)(referenceTriangles, candidateTriangles);

        for (size_t triangle = 0; triangle < pairedTriangles; ++triangle)
        {
            const int referenceIndices[3] = {
                reference.indices[triangle * 3],
                reference.indices[triangle * 3 + 1],
                reference.indices[triangle * 3 + 2]
            };
            const int candidateIndices[3] = {
                candidate.indices[triangle * 3],
                candidate.indices[triangle * 3 + 1],
                candidate.indices[triangle * 3 + 2]
            };

            bool validReference = true;
            bool validCandidate = true;
            for (size_t corner = 0; corner < 3; ++corner)
            {
                if (referenceIndices[corner] < 0 ||
                    static_cast<size_t>(referenceIndices[corner]) >= reference.vertices.size())
                    validReference = false;
                if (candidateIndices[corner] < 0 ||
                    static_cast<size_t>(candidateIndices[corner]) >= candidate.vertices.size())
                    validCandidate = false;
            }

            if (!validReference)
                comparison.unmatchedReferenceVertices += 3;
            if (!validCandidate)
                comparison.unmatchedCandidateVertices += 3;
            if (!validReference || !validCandidate)
                continue;

            const Vertex* referenceVertices[3] = {
                &reference.vertices[static_cast<size_t>(referenceIndices[0])],
                &reference.vertices[static_cast<size_t>(referenceIndices[1])],
                &reference.vertices[static_cast<size_t>(referenceIndices[2])]
            };
            const Vertex* candidateVertices[3] = {
                &candidate.vertices[static_cast<size_t>(candidateIndices[0])],
                &candidate.vertices[static_cast<size_t>(candidateIndices[1])],
                &candidate.vertices[static_cast<size_t>(candidateIndices[2])]
            };

            size_t bestRotation = 0;
            double bestScore = (std::numeric_limits<double>::infinity)();
            for (size_t rotation = 0; rotation < 3; ++rotation)
            {
                double score = 0.0;
                for (size_t corner = 0; corner < 3; ++corner)
                    score += vertexPairScore(*referenceVertices[corner], *candidateVertices[(corner + rotation) % 3]);
                if (score < bestScore)
                {
                    bestScore = score;
                    bestRotation = rotation;
                }
            }

            for (size_t corner = 0; corner < 3; ++corner)
            {
                const Vertex& a = *referenceVertices[corner];
                const Vertex& b = *candidateVertices[(corner + bestRotation) % 3];
                bool vertexDiffers = false;
                recordDelta(a.pos.x, b.pos.x, comparison.positions, vertexDiffers);
                recordDelta(a.pos.y, b.pos.y, comparison.positions, vertexDiffers);
                recordDelta(a.pos.z, b.pos.z, comparison.positions, vertexDiffers);
                recordDelta(a.normals.x, b.normals.x, comparison.normals, vertexDiffers);
                recordDelta(a.normals.y, b.normals.y, comparison.normals, vertexDiffers);
                recordDelta(a.normals.z, b.normals.z, comparison.normals, vertexDiffers);
                recordDelta(a.textureCoords.x, b.textureCoords.x, comparison.textureCoords, vertexDiffers);
                recordDelta(a.textureCoords.y, b.textureCoords.y, comparison.textureCoords, vertexDiffers);
                ++comparison.comparedVertices;
                if (vertexDiffers) ++comparison.mismatchedVertices;
            }
        }

        if (referenceTriangles > pairedTriangles)
            comparison.unmatchedReferenceVertices = (referenceTriangles - pairedTriangles) * 3;
        if (candidateTriangles > pairedTriangles)
            comparison.unmatchedCandidateVertices = (candidateTriangles - pairedTriangles) * 3;

        return comparison;
    }

    static double averageDelta(const AttributeStats& stats)
    {
        return stats.comparedComponents == 0 ? 0.0
            : static_cast<double>(stats.totalAbsoluteDelta / stats.comparedComponents);
    }

    static void printAttributeStats(const char* label, const AttributeStats& stats)
    {
        std::cout << "        ";
        ConsoleColors::write(ConsoleColors::Color::Cyan, label);
        std::cout << ": " << stats.mismatchedComponents << "/"
            << stats.comparedComponents << " components > tolerance; max |delta|="
            << stats.maxAbsoluteDelta << ", average |delta|=" << averageDelta(stats) << "\n";
    }

    static const char* implementationName(const Results& result)
    {
        return result.implementationName ? result.implementationName : "unnamed implementation";
    }

    static void checkMeshes(const std::vector<Results>& results, const std::vector<ObjFileInfo>& paths)
    {
        ConsoleColors::section("MESH CONSISTENCY CHECK", ConsoleColors::Color::Cyan);
        ConsoleColors::write(ConsoleColors::Color::Gray,
            "    Compares positions, normals, and UVs (tolerance ");
        ConsoleColors::write(ConsoleColors::Color::Yellow, attributeTolerance);
        ConsoleColors::write(ConsoleColors::Color::Gray,
            "); ignores vertex/index layout and triangle ordering, and preserves winding.\n\n");

        if (results.empty())
        {
            ConsoleColors::write(ConsoleColors::Color::Yellow, "No implementations produced results.\n");
            ConsoleColors::divider('=', 88, ConsoleColors::Color::Cyan);
            return;
        }

        for (size_t pathIndex = 0; pathIndex < paths.size(); ++pathIndex)
        {
            ConsoleColors::divider('-', 88, ConsoleColors::Color::Gray);
            ConsoleColors::write(ConsoleColors::Color::Cyan, "    MODEL ");
            ConsoleColors::write(ConsoleColors::Color::White,
                std::to_string(pathIndex + 1) + "/" + std::to_string(paths.size()));
            ConsoleColors::write(ConsoleColors::Color::Gray, "  |  ");
            ConsoleColors::write(ConsoleColors::Color::White, paths[pathIndex].path);
            std::cout << '\n';

            size_t referenceIndex = results.size();
            const char* preferredReferences[] = { "tiny obj loader", "fast obj" };
            for (const char* preferredName : preferredReferences)
            {
                for (size_t implementationIndex = 0; implementationIndex < results.size(); ++implementationIndex)
                {
                    if (pathIndex < results[implementationIndex].data.size() &&
                        std::string(implementationName(results[implementationIndex])) == preferredName)
                    {
                        referenceIndex = implementationIndex;
                        break;
                    }
                }
                if (referenceIndex != results.size()) break;
            }

            if (referenceIndex == results.size())
            {
                for (size_t implementationIndex = 0; implementationIndex < results.size(); ++implementationIndex)
                {
                    if (pathIndex < results[implementationIndex].data.size())
                    {
                        referenceIndex = implementationIndex;
                        break;
                    }
                }
            }

            if (referenceIndex == results.size())
            {
                std::cout << "      ";
                ConsoleColors::write(ConsoleColors::Color::Yellow, "[INCOMPLETE]");
                std::cout << " No implementation produced a mesh for this file.\n";
                continue;
            }

            const Mesh& referenceMesh = results[referenceIndex].data[pathIndex].mesh;
            const MeshSignature reference = buildSignature(referenceMesh);
            const bool referenceValid = reference.trailingIndexCount == 0 && reference.invalidIndexCount == 0;
            std::cout << "      ";
            ConsoleColors::write(ConsoleColors::Color::Gray, "Reference");
            std::cout << ": ";
            ConsoleColors::write(ConsoleColors::Color::Magenta, implementationName(results[referenceIndex]));
            std::cout << " (";
            ConsoleColors::write(ConsoleColors::Color::Yellow, reference.triangleCount);
            std::cout << " triangles)\n";

            for (size_t implementationIndex = 0; implementationIndex < results.size(); ++implementationIndex)
            {
                if (implementationIndex == referenceIndex) continue;

                const char* name = implementationName(results[implementationIndex]);
                if (pathIndex >= results[implementationIndex].data.size())
                {
                    std::cout << "      ";
                    ConsoleColors::write(ConsoleColors::Color::Yellow, "[INCOMPLETE]");
                    std::cout << ' ' << name << " has no result for this file.\n";
                    continue;
                }

                const Mesh& candidateMesh = results[implementationIndex].data[pathIndex].mesh;
                const MeshSignature candidate = buildSignature(candidateMesh);
                const bool candidateValid = candidate.trailingIndexCount == 0 && candidate.invalidIndexCount == 0;
                if (!candidateValid)
                {
                    std::cout << "      ";
                    ConsoleColors::write(ConsoleColors::Color::Red, "[INVALID]");
                    std::cout << ' ' << name << ": " << candidate.triangleCount << " triangles, "
                        << candidate.invalidIndexCount << " out-of-range indices, "
                        << candidate.trailingIndexCount << " trailing indices.\n";
                    continue;
                }

                if (!referenceValid)
                {
                    std::cout << "      ";
                    ConsoleColors::write(ConsoleColors::Color::Yellow, "[NOT COMPARED]");
                    std::cout << ' ' << name << ": reference mesh is invalid.\n";
                    continue;
                }

                const MeshComparison comparison = compareSignatures(reference, candidate);
                if (comparison.mismatchedTriangles == 0 && reference.triangleCount == candidate.triangleCount)
                {
                    std::cout << "      ";
                    ConsoleColors::write(ConsoleColors::Color::Green, "[MATCH]");
                    std::cout << ' ' << name << " (" << candidate.triangleCount << " triangles)\n";
                    continue;
                }

                const AttributeComparison deltas = compareAttributesInTriangleOrder(referenceMesh, candidateMesh);
                const size_t comparedCorners = (std::max)(reference.triangleCount, candidate.triangleCount) * 3;
                const size_t mismatchedCorners = deltas.mismatchedVertices +
                    deltas.unmatchedReferenceVertices + deltas.unmatchedCandidateVertices;

                if (mismatchedCorners == 0)
                {
                    std::cout << "      ";
                    ConsoleColors::write(ConsoleColors::Color::Green, "[MATCH]");
                    std::cout << ' ' << name << " (" << candidate.triangleCount
                        << " triangles; all attributes within tolerance)\n";
                    continue;
                }

                const double mismatchPercent = comparedCorners == 0
                    ? 0.0
                    : 100.0 * mismatchedCorners / comparedCorners;

                std::cout << "      ";
                ConsoleColors::write(ConsoleColors::Color::Red, "[MISMATCH]");
                std::cout << ' ' << name << ": " << std::fixed << std::setprecision(6);
                ConsoleColors::write(ConsoleColors::Color::Red, mismatchPercent);
                std::cout << "% vertex corners differ.\n" << std::defaultfloat;

                std::cout << "        ";
                ConsoleColors::write(ConsoleColors::Color::Cyan, "Attribute deltas");
                std::cout << " (paired by triangle-buffer order, cyclic corner rotations allowed):\n";
                printAttributeStats("positions", deltas.positions);
                printAttributeStats("normals", deltas.normals);
                printAttributeStats("UVs", deltas.textureCoords);
                if (deltas.unmatchedReferenceVertices || deltas.unmatchedCandidateVertices)
                {
                    std::cout << "        Unpaired corners: " << deltas.unmatchedReferenceVertices
                        << " reference, " << deltas.unmatchedCandidateVertices << " candidate.\n";
                }
            }

            if (!referenceValid)
            {
                std::cout << "      ";
                ConsoleColors::write(ConsoleColors::Color::Red, "[INVALID]");
                std::cout << " Reference mesh has " << reference.invalidIndexCount
                    << " out-of-range indices and " << reference.trailingIndexCount << " trailing indices.\n";
            }
        }

        ConsoleColors::divider('=', 88, ConsoleColors::Color::Cyan);
    }
}
