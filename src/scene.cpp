#include "scene.hpp"
#include "intersection_tests.hpp"
#include "resource/resource_manager.hpp"
#include "utils/json_parsing.hpp"

namespace PT
{

static void ParseBackgroundColor( rapidjson::Value& v, Scene* scene )
{
    auto& member           = v["color"];
    scene->backgroundColor = ParseVec3( member );
}

static void ParseCamera( rapidjson::Value& v, Scene* scene )
{
    Camera& camera = scene->camera;
    static FunctionMapper< void, Camera& > mapping(
    {
        { "position",     []( rapidjson::Value& v, Camera& camera ) { camera.position    = ParseVec3( v ); } },
        { "rotation",     []( rapidjson::Value& v, Camera& camera ) { camera.rotation    = glm::radians( ParseVec3( v ) ); } },
        { "vfov",         []( rapidjson::Value& v, Camera& camera ) { camera.vfov        = glm::radians( ParseNumber< float >( v ) ); } },
        { "aspectRatio",  []( rapidjson::Value& v, Camera& camera ) { camera.aspectRatio = ParseNumber< float >( v ); } },
        { "exposure",     []( rapidjson::Value& v, Camera& camera ) { camera.exposure    = ParseNumber< float >( v ); } },
        { "gamma",        []( rapidjson::Value& v, Camera& camera ) { camera.gamma       = ParseNumber< float >( v ); } },
        { "antialiasing", []( rapidjson::Value& v, Camera& camera ) { camera.aaAlgorithm = AntiAlias::AlgorithmFromString( v.GetString() ); } },
    });

    mapping.ForEachMember( v, camera );
}

static void ParseMaterial( rapidjson::Value& v, Scene* scene )
{
    auto mat = std::make_shared< Material >();
    static FunctionMapper< void, Material& > mapping(
    {
        { "name",   []( rapidjson::Value& v, Material& mat ) { mat.name   = v.GetString(); } },
        { "albedo", []( rapidjson::Value& v, Material& mat ) { mat.albedo = ParseVec3( v ); } },
    });

    mapping.ForEachMember( v, *mat );
    ResourceManager::AddMaterial( mat );
}

static void ParsePointLight( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, PointLight& > mapping(
    {
        { "color",    []( rapidjson::Value& v, PointLight& l ) { l.color    = ParseVec3( v ); } },
        { "position", []( rapidjson::Value& v, PointLight& l ) { l.position = ParseVec3( v ); } },
    });

    scene->pointLights.push_back( {} );
    mapping.ForEachMember( value, scene->pointLights[scene->pointLights.size() - 1] );
}

static void ParseSphere( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, Sphere& > mapping(
    {
        { "position", []( rapidjson::Value& v, Sphere& s ) { s.position = ParseVec3( v ); } },
        { "radius",   []( rapidjson::Value& v, Sphere& s ) { s.radius   = ParseNumber< float >( v ); } },
        { "material", []( rapidjson::Value& v, Sphere& s ) { s.material = ResourceManager::GetMaterial( v.GetString() ); } },
    });

    scene->spheres.push_back( {} );
    mapping.ForEachMember( value, scene->spheres[scene->spheres.size() - 1] );
}

static void ParseOutputImageData( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, Scene& > mapping(
    {
        { "filename",   []( rapidjson::Value& v, Scene& s ) { s.outputImageFilename = v.GetString(); } },
        { "resolution", []( rapidjson::Value& v, Scene& s )
            {
                s.imageResolution.x = ParseNumber< int >( v[0] );
                s.imageResolution.y = ParseNumber< int >( v[1] );
            }
        },
    });

    mapping.ForEachMember( value, *scene );
}

bool Scene::Load( const std::string& filename )
{
    auto document = ParseJSONFile( filename );
    if ( document.IsNull() )
    {
        return false;
    }

    static FunctionMapper< void, Scene* > mapping(
    {
        { "BackgroundColor", ParseBackgroundColor },
        { "Camera",          ParseCamera },
        { "Material",        ParseMaterial },
        { "PointLight",      ParsePointLight },
        { "Sphere",          ParseSphere },
        { "OutputImageData", ParseOutputImageData },
    });

    mapping.ForEachMember( document, this );

    return true;
}

bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    float closestTime      = FLT_MAX;
    int closestSphereIndex = -1;
    float t;
    for ( int i = 0; i < (int)spheres.size(); ++i )
    {
        const Sphere& s = spheres[i];
        if ( intersect::RaySphere( ray.position, ray.direction, s.position, s.radius, t ) )
        {
            if ( t < closestTime )
            {
                closestTime        = t;
                closestSphereIndex = i;
            }
        }
    }

    if ( closestSphereIndex != -1 )
    {
        Sphere& s        = spheres[closestSphereIndex];
        hitData.t        = closestTime;
        hitData.sphere   = &s;
        hitData.material = s.material.get();
        hitData.position = ray.Evaluate( hitData.t );
        hitData.normal   = s.GetNormal( hitData.position );
        assert( s.material );

        return true;
    }

    return false;
}

} // namespace PT
