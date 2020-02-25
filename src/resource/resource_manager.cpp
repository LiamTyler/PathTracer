#include "resource/resource_manager.hpp"
#include <unordered_map>

namespace PT
{

static std::unordered_map< std::string, std::shared_ptr< Material > > s_materials;
static std::unordered_map< std::string, std::shared_ptr< Model > > s_models;
static std::unordered_map< std::string, std::shared_ptr< Skybox > > s_skyboxes;

namespace ResourceManager
{

    void Init()
    {
        s_materials.clear();
        s_models.clear();
        s_skyboxes.clear();
    }

    void Shutdown()
    {
        Init();
    }

    void AddMaterial( std::shared_ptr< Material > res )
    {
        assert( res->name != "" );
        s_materials[res->name] = res;
    }

    std::shared_ptr< Material > GetMaterial( const std::string& name )
    {
        assert( s_materials.find( name ) != s_materials.end() );
        return s_materials[name];
    }

    void AddModel( std::shared_ptr< Model > res )
    {
        assert( res->name != "" );
        s_models[res->name] = res;
    }

    std::shared_ptr< Model > GetModel( const std::string& name )
    {
        assert( s_models.find( name ) != s_models.end() );
        return s_models[name];
    }

    void AddSkybox( std::shared_ptr< Skybox > res )
    {
        assert( res->name != "" );
        s_skyboxes[res->name] = res;
    }

    std::shared_ptr< Skybox > GetSkybox( const std::string& name )
    {
        assert( s_skyboxes.find( name ) != s_skyboxes.end() );
        return s_skyboxes[name];
    }

} // namespace ResourceManager
} // namespace PT