#include "math.hpp"

namespace PT
{

Ray::Ray( const glm::vec3& pos, const glm::vec3& dir ) :
    position( pos ),
    direction( dir )
{
}

glm::vec3 Ray::Evaluate( float t ) const
{
    return position + t * direction;
}

} // namespace PT