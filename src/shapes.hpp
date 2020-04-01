#pragma once

#include "aabb.hpp"
#include "intersection_tests.hpp"
#include "resource/material.hpp"
#include "transform.hpp"
#include <memory>

namespace PT
{

class ModelInstance;

struct Shape
{
    Shape() = default;

    virtual Material* GetMaterial() const = 0;
    virtual bool Intersect( const Ray& ray, IntersectionData* hitData ) const = 0;
    virtual AABB WorldSpaceAABB() const = 0;
};

struct Sphere : public Shape
{
    std::shared_ptr< Material > material;
    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    float radius       = 1;

    Transform worldToLocal;

    Material* GetMaterial() const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    AABB WorldSpaceAABB() const override;
};

struct Triangle : public Shape
{
    std::shared_ptr< ModelInstance > model;
    
    uint32_t i0, i1, i2;
    uint32_t materialIndex;

    Material* GetMaterial() const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    AABB WorldSpaceAABB() const override;
};

} // namespace PT
