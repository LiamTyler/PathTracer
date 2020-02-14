#include "camera.hpp"

namespace PT
{

void Camera::UpdateOrientationVectors()
{
    glm::mat4 rot( 1 );
    rot         = glm::rotate( rot, rotation.y, glm::vec3( 0, 1, 0 ) );
    rot         = glm::rotate( rot, rotation.x, glm::vec3( 1, 0, 0 ) );
    m_viewDir   = glm::vec3( rot * glm::vec4( 0, 0, -1, 0 ) );
    m_upDir     = glm::vec3( rot * glm::vec4( 0, 1, 0, 0 ) );
    m_rightDir  = glm::cross( m_viewDir, m_upDir );
}

glm::vec3 Camera::GetViewDir() const
{
    return m_viewDir;
}

glm::vec3 Camera::GetUpDir() const
{
    return m_upDir;
}

glm::vec3 Camera::GetRightDir() const
{
    return m_rightDir;
}

} // namespace PT