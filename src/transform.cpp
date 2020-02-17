#include "transform.hpp"

namespace PT
{

glm::mat4 Transform::ModelMatrix() const
{
    glm::mat4 model( 1 );
    model = glm::translate( model, position );
    model = glm::rotate( model, rotation.y, glm::vec3( 0, 1, 0 ) );
    model = glm::rotate( model, rotation.x, glm::vec3( 1, 0, 0 ) );
    model = glm::rotate( model, rotation.z, glm::vec3( 0, 0, 1 ) );
    model = glm::scale( model, scale );
     
    return model;
}

Ray Transform::WorldToLocal( const Ray& ray ) const
{
    glm::mat4 model = glm::inverse( ModelMatrix() );
    Ray ret;
    ret.direction = glm::vec3( model * glm::vec4( ray.direction, 0 ) );
    ret.position  = glm::vec3( model * glm::vec4( ray.position, 1 ) );

    return ret;
}

} // namespace PT