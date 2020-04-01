#pragma once

#include "aabb.hpp"
#include "intersection_tests.hpp"
#include "math.hpp"
#include "resource/material.hpp"
#include "resource/resource.hpp"
#include "shapes.hpp"
#include "transform.hpp"
#include <memory>
#include <vector>

namespace PT
{

    struct ModelCreateInfo
    {
        std::string name;
        std::string filename;
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
        
        bool Load( const ModelCreateInfo& createInfo );
        void RecalculateNormals();
        
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
        std::vector< uint8_t > triangleMaterialIndices;
        std::vector< Mesh > meshes;
        std::vector< std::shared_ptr< Material > > materials;
    };

    class ModelInstance
    {
    public:
        ModelInstance( const Model& model, const Transform& localToWorld, std::shared_ptr< Material > material = nullptr );

        void EmitTriangles( std::vector< std::shared_ptr< Shape > >& shapes, std::shared_ptr< ModelInstance > modelPtr ) const;

        Transform localToWorld, worldToLocal;
        AABB worldSpaceAABB;
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
        std::vector< uint8_t > triangleMaterialIndices;
        std::vector< std::shared_ptr< Material > > materials;
    };

} // namespace PT