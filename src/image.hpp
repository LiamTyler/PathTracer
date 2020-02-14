#pragma once

#include <string>
#include "glm/glm.hpp"

namespace PT
{

class Image
{
public:
    Image() = default;
    Image( int width, int height );
    ~Image();
    Image( const Image& ) = delete;
    Image& operator=( const Image& ) = delete;
    Image( Image&& img );
    Image& operator=( Image&& img );

    bool Save( const std::string& filename ) const;

    int GetWidth() const;
    int GetHeight() const;
    glm::vec3* GetPixels() const;
    glm::vec3 GetPixel( int r, int c ) const;
    void SetPixel( int r, int c, const glm::vec3& pixel );

private:
    int m_width    = 0;
    int m_height   = 0;
    glm::vec3* m_pixels     = nullptr;
};

} // namespace PT