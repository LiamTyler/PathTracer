#include "aabb.hpp"

namespace PT
{

AABB::AABB() :
    min( glm::vec3( FLT_MAX ) ),
    max( glm::vec3( -FLT_MAX ) )
{
}

AABB::AABB( const glm::vec3& _min, const glm::vec3& _max ) :
    min( _min ),
    max( _max )
{
}

void AABB::Union( const AABB& aabb )
{
    min = glm::min( min, aabb.min );
    max = glm::max( max, aabb.max );
}

void AABB::Union( const glm::vec3& point )
{
    min = glm::min( min, point );
    max = glm::max( max, point );
}

glm::vec3 AABB::Centroid() const
{
    return 0.5f * (min + max);
}

int AABB::LongestDimension() const
{
    glm::vec3 d = max - min;
    if ( d[0] > d[1] && d[0] > d[2] )
    {
        return 0;
    }
    else if ( d[1] > d[2] )
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

} // namespace PT