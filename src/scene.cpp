#include "scene.hpp"
#include "configuration.hpp"
#include "intersection_tests.hpp"
#include "resource/resource_manager.hpp"
#include "utils/json_parsing.hpp"
#include "utils/time.hpp"

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

static void ParseAmbientLight( rapidjson::Value& v, Scene* scene )
{
    auto& member        = v["color"];
    scene->ambientLight = ParseVec3( member );
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
    camera.UpdateOrientationVectors();
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

static void ParseMaterial( rapidjson::Value& v, Scene* scene )
{
    auto mat = std::make_shared< Material >();
    static FunctionMapper< void, Material& > mapping(
    {
        { "name",          []( rapidjson::Value& v, Material& mat ) { mat.name   = v.GetString(); } },
        { "albedo",        []( rapidjson::Value& v, Material& mat ) { mat.albedo = ParseVec3( v ); } },
        { "Ks",            []( rapidjson::Value& v, Material& mat ) { mat.Ks     = ParseVec3( v ); } },
        { "Ns",            []( rapidjson::Value& v, Material& mat ) { mat.Ns     = ParseNumber< float >( v ); } },
        { "Tr",            []( rapidjson::Value& v, Material& mat ) { mat.Tr     = ParseVec3( v ); } },
        { "ior",           []( rapidjson::Value& v, Material& mat ) { mat.ior    = ParseNumber< float >( v ); } },
        { "albedoTexture", []( rapidjson::Value& v, Material& mat )
            {
                mat.albedoTexture = ResourceManager::GetTexture( v.GetString() );
                if ( !mat.albedoTexture )
                {
                    std::cout << "No texture with name '" << v.GetString() << "' found in resource manager!" << std::endl;
                }
            }
        },
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

static void ParseSkybox( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, SkyboxCreateInfo& > mapping(
    {
        { "name",           []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.name           = v.GetString(); } },
        { "right",          []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.right          = v.GetString(); } },
        { "left",           []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.left           = v.GetString(); } },
        { "top",            []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.top            = v.GetString(); } },
        { "bottom",         []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.bottom         = v.GetString(); } },
        { "back",           []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.back           = v.GetString(); } },
        { "front",          []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.front          = v.GetString(); } },
        { "flipVertically", []( rapidjson::Value& v, SkyboxCreateInfo& i ) { i.flipVertically = v.GetBool(); } },
    });

    SkyboxCreateInfo info;
    mapping.ForEachMember( value, info );

    auto res = std::make_shared< Skybox >();
    if ( res->Load( info ) )
    {
        ResourceManager::AddSkybox( res );
        scene->skybox = res;
    }
}

static void ParseTexture( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, TextureCreateInfo& > mapping(
    {
        { "name",           []( rapidjson::Value& v, TextureCreateInfo& info ) { info.name           = v.GetString(); } },
        { "filename",       []( rapidjson::Value& v, TextureCreateInfo& info ) { info.filename       = RESOURCE_DIR + std::string( v.GetString() ); } },
        { "flipVertically", []( rapidjson::Value& v, TextureCreateInfo& info ) { info.flipVertically = v.GetBool(); } },
    });

    TextureCreateInfo info;
    mapping.ForEachMember( value, info );
    auto res = std::make_shared< Texture >();
    if ( res->Load( info ) )
    {
        ResourceManager::AddTexture( res );
    }
}

bool Scene::Load( const std::string& filename )
{
    auto startTime = Time::GetTimePoint();

    auto document = ParseJSONFile( filename );
    if ( document.IsNull() )
    {
        return false;
    }

    static FunctionMapper< void, Scene* > mapping(
    {
        { "AmbientLight",     ParseAmbientLight },
        { "BackgroundColor",  ParseBackgroundColor },
        { "Camera",           ParseCamera },
        { "DirectionalLight", ParseDirectionalLight },
        { "Material",         ParseMaterial },
        { "Model",            ParseModel },
        { "ModelInstance",    ParseModelInstance },
        { "OutputImageData",  ParseOutputImageData },
        { "PointLight",       ParsePointLight },
        { "Skybox",           ParseSkybox },
        { "Sphere",           ParseSphere },
        { "Texture",          ParseTexture },
    });

    mapping.ForEachMember( document, this );

    // compute some scene statistics
    size_t numSpheres        = 0;
    size_t numModelInstances = 0;
    size_t totalTriangles    = 0;
    for ( const auto& shape : shapes )
    {
        if ( std::dynamic_pointer_cast< Sphere >( shape ) )
        {
            numSpheres += 1;
        }
        if ( std::dynamic_pointer_cast< ModelInstance >( shape ) )
        {
            numModelInstances += 1;
            totalTriangles += std::dynamic_pointer_cast< ModelInstance >( shape )->model->indices.size() / 3;
        }
    }
    std::cout << "\nScene '" << filename << "' stats:" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Load time: " << Time::GetDuration( startTime ) << " ms" << std::endl;
    std::cout << "Number of shapes: " << shapes.size() << std::endl;
    std::cout << "\tSpheres: " << numSpheres << std::endl;
    std::cout << "\tModelInstances: " << numModelInstances << std::endl;
    std::cout << "Total number of triangles: " << totalTriangles << std::endl;

    return true;
}

bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    hitData.t = FLT_MAX;

    for ( const auto& shape : shapes )
    {
        shape->Intersect( ray, &hitData );
    }

    return hitData.t != FLT_MAX && hitData.t > 0;
}

} // namespace PT
