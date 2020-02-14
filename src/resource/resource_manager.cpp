#include "resource/resource_manager.hpp"
#include <unordered_map>

namespace PT
{

static std::unordered_map< std::string, std::shared_ptr< Material > > s_materials;

namespace ResourceManager
{

    void Init()
    {
        s_materials.clear();
    }

    void Shutdown()
    {
        s_materials.clear();
    }

    void AddMaterial( std::shared_ptr< Material > mat )
    {
        assert( mat->name != "" );
        s_materials[mat->name] = mat;
    }

    std::shared_ptr< Material > GetMaterial( const std::string& name )
    {
        assert( s_materials.find( name ) != s_materials.end() );
        return s_materials[name];
    }

} // namespace ResourceManager
} // namespace PT