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

    // samples shape uniformly. PDF is with respect to the solid angle from a reference point/normal
    // to the sampled shape position
    SurfaceInfo SampleWithRespectToSolidAngle( const Interaction& it ) const;

    // samples the shape uniformly, with respect to the surface area
    virtual SurfaceInfo SampleWithRespectToArea() const = 0;
    virtual bool Intersect( const Ray& ray, IntersectionData* hitData ) const = 0;
    virtual bool TestIfHit( const Ray& ray, float maxT = FLT_MAX ) const = 0;
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
    SurfaceInfo SampleWithRespectToArea() const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    bool TestIfHit( const Ray& ray, float maxT = FLT_MAX ) const override;
    AABB WorldSpaceAABB() const override;
};

struct Triangle : public Shape
{
    std::shared_ptr< MeshInstance > mesh;
    uint32_t i0, i1, i2;

    Material* GetMaterial() const override;
    float Area() const override;
    SurfaceInfo SampleWithRespectToArea() const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    bool TestIfHit( const Ray& ray, float maxT = FLT_MAX ) const override;
    AABB WorldSpaceAABB() const override;
};

} // namespace PT
