#pragma once

#include "aabb.hpp"
#include "shapes.hpp"

namespace PT
{

struct LinearBVHNode
{
    AABB aabb;
    union
    {
        int firstIndexOffset;  // if node is a leaf
        int secondChildOffset; // if node is not a leaf (only saving the offset of the 2nd child, since the first child offset is always the parentIndex + 1)
    };
    uint16_t numShapes;
    uint8_t axis;
    uint8_t padding;
};

class BVH
{
public:
    BVH() = default;
    ~BVH();

    void Build( std::vector< std::shared_ptr< Shape > >& shapes );
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const;
    AABB GetAABB() const;

    std::vector< std::shared_ptr< Shape > > shapes;
    LinearBVHNode* nodes = nullptr;
};

} // namespace PT