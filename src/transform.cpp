#include "transform.hpp"

namespace PT
{

Transform::Transform() : matrix( glm::mat4( 1 ) ) {}

Transform::Transform( const glm::mat4& mat ) : matrix( mat ) {}

Transform::Transform( const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale )
{
    matrix = glm::mat4( 1 );
    matrix = glm::translate( matrix, position );
    matrix = glm::rotate( matrix, rotation.y, glm::vec3( 0, 1, 0 ) );
    matrix = glm::rotate( matrix, rotation.x, glm::vec3( 1, 0, 0 ) );
    matrix = glm::rotate( matrix, rotation.z, glm::vec3( 0, 0, 1 ) );
    matrix = glm::scale( matrix, scale );
}

Transform Transform::Inverse() const
{
    return Transform( glm::inverse( matrix ) );
}

Ray Transform::operator*( const Ray& ray ) const
{
    Ray ret;
    ret.direction = glm::vec3( matrix * glm::vec4( ray.direction, 0 ) );
    ret.position  = glm::vec3( matrix * glm::vec4( ray.position, 1 ) );

    return ret;
}

} // namespace PT