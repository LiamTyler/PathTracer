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

    struct Mesh
    {
    public:
        std::string name;
        int materialIndex = -1;
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
    };

    class Model : public Resource
    {
    public:
        Model() = default;
        
        bool Load( const ModelCreateInfo& createInfo );
        void RecalculateNormals();
        bool IntersectRay( const Ray& ray, IntersectionData& hitData ) const;
        
        AABB aabb;
        std::vector< Mesh > meshes;
        std::vector< std::shared_ptr< Material > > materials;
    };

} // namespace PT