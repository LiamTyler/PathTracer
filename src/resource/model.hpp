#pragma once

#include "aabb.hpp"
#include "intersection_tests.hpp"
#include "math.hpp"
#include "resource/material.hpp"
#include "resource/resource.hpp"
#include <memory>
#include <vector>

namespace PT
{

    struct ModelCreateInfo
    {
        std::string name;
        std::string filename;
    };

    struct BVHNode
    {
        AABB aabb;
        union
        {
            int firstIndexOffset; // if node is a leaf
            int secondChildOffset; // if node is not a leaf (dont actually have to save the offset of the first child, since its always parent + 1)
        };
        uint16_t numTriangles;
        uint8_t axis;
        uint8_t padding;
    };

    struct Mesh
    {
    public:
        std::string name;
        int materialIndex = -1;
        int startIndex; // wont be accurate after BVH construction due to reordering
        int numIndices;
        int numVertices;
    };

    class Model : public Resource
    {
    public:
        Model() = default;
        ~Model();
        
        bool Load( const ModelCreateInfo& createInfo );
        void RecalculateNormals();
        bool IntersectRay( const Ray& ray, IntersectionData& hitData, int& materialIndex ) const;
        
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
        std::vector< uint8_t > triangleMaterialIndices;
        BVHNode* bvh = nullptr;
        std::vector< Mesh > meshes;
        std::vector< std::shared_ptr< Material > > materials;
    };

} // namespace PT