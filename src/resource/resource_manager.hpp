#pragma once

#include "resource/material.hpp"
#include <memory>
#include <string>

namespace PT
{
namespace ResourceManager
{

    void Init();
    void Shutdown();

    void AddMaterial( std::shared_ptr< Material > mat );
    std::shared_ptr< Material > GetMaterial( const std::string& name );

} // namespace ResourceManager
} // namespace PT