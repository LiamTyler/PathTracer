#include "scene.hpp"
#include "configuration.hpp"
#include "intersection_tests.hpp"
#include "resource/resource_manager.hpp"
#include "utils/json_parsing.hpp"

namespace PT
{

Scene::~Scene()
{
    for ( auto light : lights )
    {
        delete light;
    }
}

static Transform ParseTransform( rapidjson::Value& value )
{
    static FunctionMapper< void, Transform& > mapping(
    {
        { "position", []( rapidjson::Value& v, Transform& t ) { t.position = ParseVec3( v ); } },
        { "rotation", []( rapidjson::Value& v, Transform& t ) { t.rotation = glm::radians( ParseVec3( v ) ); } },
        { "scale",    []( rapidjson::Value& v, Transform& t ) { t.scale    = ParseVec3( v ); } },
    });
    Transform t = { glm::vec3( 0 ), glm::vec3( 0 ), glm::vec3( 1 ) };
    mapping.ForEachMember( value, t );

    return t;
}

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

static void ParseModel( rapidjson::Value& v, Scene* scene )
{
    static FunctionMapper< void, ModelCreateInfo& > mapping(
    {
        { "name",     []( rapidjson::Value& v, ModelCreateInfo& m ) { m.name     = v.GetString(); } },
        { "filename", []( rapidjson::Value& v, ModelCreateInfo& m ) { m.filename = RESOURCE_DIR + std::string( v.GetString() ); } },
    });

    ModelCreateInfo info;
    mapping.ForEachMember( v, info );

    auto model = std::make_shared< Model >();
    if ( model->Load( info ) )
    {
        ResourceManager::AddModel( model );
    }
}

static void ParsePointLight( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, PointLight* > mapping(
    {
        { "color",    []( rapidjson::Value& v, PointLight* l ) { l->color    = ParseVec3( v ); } },
        { "position", []( rapidjson::Value& v, PointLight* l ) { l->position = ParseVec3( v ); } },
    });

    scene->lights.push_back( new PointLight );
    mapping.ForEachMember( value, (PointLight*)scene->lights[scene->lights.size() - 1] );
}

static void ParseDirectionalLight( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, DirectionalLight* > mapping(
    {
        { "color",     []( rapidjson::Value& v, DirectionalLight* l ) { l->color     = ParseVec3( v ); } },
        { "direction", []( rapidjson::Value& v, DirectionalLight* l ) { l->direction = glm::normalize( ParseVec3( v ) ); } },
    });

    scene->lights.push_back( new DirectionalLight );
    mapping.ForEachMember( value, (DirectionalLight*)scene->lights[scene->lights.size() - 1] );
}

static void ParseSphere( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, Sphere& > mapping(
    {
        { "transform", []( rapidjson::Value& v, Sphere& o )
            {
                o.transform = ParseTransform( v );
            }
        },
        { "material", []( rapidjson::Value& v, Sphere& s ) { s.material = ResourceManager::GetMaterial( v.GetString() ); } },
    });

    auto o = std::make_shared< Sphere >();
    scene->shapes.push_back( o );
    mapping.ForEachMember( value, *o );
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

static void ParseModelInstance( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, ModelInstance& > mapping(
    {
        { "transform", []( rapidjson::Value& v, ModelInstance& o )
            {
                o.transform = ParseTransform( v );
            }
        },
        { "model",     []( rapidjson::Value& v, ModelInstance& o )
            {
                o.model = ResourceManager::GetModel( v.GetString() );
                assert( o.model );
            }
        },
        { "material",  []( rapidjson::Value& v, ModelInstance& o )
            {
                auto mat = ResourceManager::GetMaterial( v.GetString() );
                assert( mat );
                assert( o.model ); // need to specify model before material
                o.materials.resize( o.model->meshes.size() );
                for ( auto& m : o.materials )
                {
                    m = mat;
                }
            }
        },
    });

    auto o = std::make_shared< ModelInstance >();
    scene->shapes.push_back( o );
    mapping.ForEachMember( value, *o );
    assert( o->model );

    // if no custom materials were specified, use the ones the model was loaded with
    if ( o->materials.size() == 0 )
    {
        o->materials = o->model->materials;
    }
    assert( o->materials.size() );
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
        { "BackgroundColor",  ParseBackgroundColor },
        { "Camera",           ParseCamera },
        { "Material",         ParseMaterial },
        { "Model",            ParseModel },
        { "PointLight",       ParsePointLight },
        { "DirectionalLight", ParseDirectionalLight },
        { "Sphere",           ParseSphere },
        { "OutputImageData",  ParseOutputImageData },
        { "ModelInstance",    ParseModelInstance },
    });

    mapping.ForEachMember( document, this );

    return true;
}

bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    hitData.t = FLT_MAX;

    for ( const auto& shape : shapes )
    {
        shape->Intersect( ray, &hitData );
    }

    return hitData.t != FLT_MAX;
}

} // namespace PT
