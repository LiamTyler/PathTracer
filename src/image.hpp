#pragma once

#include "glm/glm.hpp"
#include <functional>
#include <string>

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

    template< typename Func >
    void ForAllPixels( const Func& F )
    {
        #pragma omp parallel for
        for ( int row = 0; row < m_height; ++row )
        {
            for ( int col = 0; col < m_width; ++col )
            {
                SetPixel( row, col, F( GetPixel( row, col ) ) );
            }
        }
    }

    bool Save( const std::string& filename ) const;

    int GetWidth() const;
    int GetHeight() const;
    glm::vec3* GetPixels() const;
    glm::vec3 GetPixel( int r, int c ) const;
    void SetPixel( int r, int c, const glm::vec3& pixel );

private:
    int m_width         = 0;
    int m_height        = 0;
    glm::vec3* m_pixels = nullptr;
};

} // namespace PT