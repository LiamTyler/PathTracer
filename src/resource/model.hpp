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

    struct BVHTriangleInfo
    {
        
        BVHTriangleInfo() :
            data( 0 )
        {}

        BVHTriangleInfo( int index, int material ) :
            data( index | material << 24 )
        {}

        int Index() const
        {
            return data & ( (1 << 24) - 1 );
        }

        int Material() const
        {
            return data >> 24;
        }

        // lower 24 bits are the first index of the triangle in the model.indices array
        // upper 8 bits are the material index
        int data;
    };

    struct BVHNode
    {
        std::unique_ptr< BVHNode > left;
        std::unique_ptr< BVHNode > right;
        std::vector< BVHTriangleInfo > triangles;
        AABB aabb;
    };

    struct Mesh
    {
    public:
        std::string name;
        int materialIndex = -1;
        int startIndex;
        int numIndices;
        int numVertices;
    };

    class Model : public Resource
    {
    public:
        Model() = default;
        
        bool Load( const ModelCreateInfo& createInfo );
        void RecalculateNormals();
        bool IntersectRay( const Ray& ray, IntersectionData& hitData, int& materialIndex ) const;
        
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
        AABB aabb;
        std::unique_ptr< BVHNode > bvh;
        std::vector< Mesh > meshes;
        std::vector< std::shared_ptr< Material > > materials;
    };

} // namespace PT