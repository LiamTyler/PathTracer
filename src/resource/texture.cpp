#include "math.hpp"
#include "resource/texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include <algorithm>

namespace PT
{

Texture::~Texture()
{
    if ( m_pixels )
    {
        free( m_pixels );
    }
}

bool Texture::Load( const TextureCreateInfo& info )
{
    name = info.name;

    stbi_set_flip_vertically_on_load( info.flipVertically );
    int nc;
    m_pixels = stbi_load( info.filename.c_str(), &m_width, &m_height, &nc, 4 );

    if ( !m_pixels )
    {
        std::cout << "Failed to load image '" << info.filename << "'" << std::endl;
        return false;
    }
    return true;
}
    
int Texture::GetWidth() const
{
    return m_width;
}
int Texture::GetHeight() const
{
    return m_height;
}

unsigned char* Texture::GetPixels() const
{
    return m_pixels;
}

glm::vec4 Texture::GetPixel( float u, float v ) const
{
    int w           = std::min( m_width - 1, static_cast< int >( u * m_width ) );
    int h           = std::min( m_height - 1, static_cast< int >( v * m_height ) );
    unsigned char r = m_pixels[4 * (h * m_width + w) + 0];
    unsigned char g = m_pixels[4 * (h * m_width + w) + 1];
    unsigned char b = m_pixels[4 * (h * m_width + w) + 2];
    unsigned char a = m_pixels[4 * (h * m_width + w) + 3];

    glm::vec4 color = 1.0f / 255.0f * glm::vec4( r, g, b, a );
    return color;
}

} // namespace PT