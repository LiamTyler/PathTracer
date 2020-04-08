#pragma once

#include "aabb.hpp"
#include "intersection_tests.hpp"
#include "lights.hpp"
#include "resource/material.hpp"
#include "transform.hpp"
#include <memory>

namespace PT
{

class MeshInstance;

struct SurfaceInfo
{
    glm::vec3 position;
    glm::vec3 normal;
    float pdf;
};

struct Shape
{
    Shape() = default;

    virtual Material* GetMaterial() const = 0;
    virtual float Area() const = 0;
    // sample random position on shape surface
    virtual SurfaceInfo Sample() const = 0;
    virtual bool Intersect( const Ray& ray, IntersectionData* hitData ) const = 0;
    virtual AABB WorldSpaceAABB() const = 0;
};

struct Sphere : public Shape
{
    std::shared_ptr< Material > material;
    glm::vec3 position   = glm::vec3( 0 );
    glm::vec3 rotation   = glm::vec3( 0 );
    float radius         = 1;

    Transform worldToLocal;

    Material* GetMaterial() const override;
    float Area() const override;
    SurfaceInfo Sample() const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    AABB WorldSpaceAABB() const override;
};

struct Triangle : public Shape
{
    std::shared_ptr< MeshInstance > mesh;
    uint32_t i0, i1, i2;

    Material* GetMaterial() const override;
    float Area() const override;
    SurfaceInfo Sample() const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    AABB WorldSpaceAABB() const override;
};

} // namespace PT
