#include "image.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include <iostream>

namespace PT
{
    Image::Image( unsigned int width, unsigned int height ) :
        m_width( width ),
        m_height( height ),
        m_pixels( static_cast< glm::vec3* >( malloc( width * height * sizeof( glm::vec3 ) ) ) )
    {
    }

    Image::~Image()
    {
        if ( m_pixels )
        {
            free( m_pixels );
        }
    }

    Image::Image( Image&& img )
    {
        *this = std::move( img );
    }

    Image& Image::operator=( Image&& img )
    {
        
        if ( m_pixels )
        {
            free( m_pixels );
        }
        m_width      = img.m_width;
        m_height     = img.m_height;
        m_pixels     = img.m_pixels;
        img.m_pixels = nullptr;

        return *this;
    }

    bool Image::Save( const std::string& filename ) const
    {
        std::string ext;
        size_t i = filename.length();
        while ( filename[--i] != '.' && i >= 0 );
        if ( i < 0 )
        {
            std::cout << "Image filename '" << filename << "' has no extension, don't know how to save it" << std::endl;
            return false;
        }
        ext = filename.substr( i + 1 );

        if ( ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "tga" || ext == "bmp" )
        {
            uint8_t* image24Bit = static_cast< uint8_t* >( malloc( m_height * m_width * 3 ) );
            for ( unsigned row = 0; row < m_height; ++row )
            {
                for ( unsigned col = 0; col < m_width; ++col )
                {
                    glm::vec3 color       = 255.0f * glm::clamp( GetPixel( row, col ), glm::vec3( 0 ), glm::vec3( 1 ) );
                    unsigned index        = 3 * (row * m_width + col);
                    image24Bit[index + 0] = static_cast< uint8_t >( color.r );
                    image24Bit[index + 1] = static_cast< uint8_t >( color.g );
                    image24Bit[index + 2] = static_cast< uint8_t >( color.b );
                }
            }

            int ret = 0;
            switch ( ext[0] )
            {
                case 'p':
                    ret = stbi_write_png( filename.c_str(), m_width, m_height, 3, image24Bit, m_width * 3 );
                    break;
                case 'j':
                    ret = stbi_write_jpg( filename.c_str(), m_width, m_height, 3, image24Bit, 95 );
                    break;
                case 'b':
                    ret = stbi_write_bmp( filename.c_str(), m_width, m_height, 3, image24Bit );
                    break;
                case 't':
                    ret = stbi_write_tga( filename.c_str(), m_width, m_height, 3, image24Bit );
                    break;
            }
            free( image24Bit );
            if ( !ret )
            {
                std::cout << "Failed to write image: '" << filename << "'" << std::endl;
                return false;
            }
        }
        else
        {
            std::cout << "Saving image as filetype '" << ext << "' is not supported" << std::endl;
            return false;
        }

        return true;
    }

    glm::vec3 Image::GetPixel( int r, int c ) const
    {
        assert( m_pixels );
        return m_pixels[r * m_width + c];
    }

    void Image::SetPixel( int r, int c, const glm::vec3& pixel )
    {
        m_pixels[r * m_width + c] = pixel;
    }

    unsigned int Image::GetWidth() const
    {
        return m_width;
    }

    unsigned int Image::GetHeight() const
    {
        return m_height;
    }

    glm::vec3* Image::GetPixels() const
    {
        return m_pixels;
    }

} // namespace PT