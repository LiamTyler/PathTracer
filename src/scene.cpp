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
    struct Vectors
    {
        glm::vec3 position = glm::vec3( 0 );
        glm::vec3 rotation = glm::vec3( 0 );
        glm::vec3 scale    = glm::vec3( 1 );
    };
    static FunctionMapper< void, Vectors& > mapping(
    {
        { "position", []( rapidjson::Value& v, Vectors& t ) { t.position = ParseVec3( v ); } },
        { "rotation", []( rapidjson::Value& v, Vectors& t ) { t.rotation = glm::radians( ParseVec3( v ) ); } },
        { "scale",    []( rapidjson::Value& v, Vectors& t ) { t.scale    = ParseVec3( v ); } },
    });
    Vectors v;
    mapping.ForEachMember( value, v );

    return Transform( v.position, v.rotation, v.scale );
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

static void ParseBVH( rapidjson::Value& value, Scene* scene )
{
    static std::unordered_map< std::string, BVH::SplitMethod > stringToSplitMethod =
    {
        { "Middle", BVH::SplitMethod::Middle },
        { "EqualCounts", BVH::SplitMethod::EqualCounts },
        { "SAH", BVH::SplitMethod::SAH },
    };
    static FunctionMapper< void, BVH& > mapping(
    {
        { "splitMethod",      []( rapidjson::Value& v, BVH& b )
            {
                auto it = stringToSplitMethod.find( v.GetString() );
                if ( it == stringToSplitMethod.end() )
                {
                    std::cout << "No SplitMethod with name '" << v.GetString() << "' found! Using SAH" << std::endl;
                }
                else
                {
                    b.splitMethod = it->second;
                }
            }
        },
    });
    mapping.ForEachMember( value, scene->bvh );
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

static void ParseMaxDepth( rapidjson::Value& v, Scene* scene )
{
    scene->maxDepth = v.GetInt();
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
    struct ModelInstanceCreateInfo
    {
        Transform transform;
        std::string modelName;
        std::string materialName;
    };

    static FunctionMapper< void, ModelInstanceCreateInfo& > mapping(
    {
        { "transform", []( rapidjson::Value& v, ModelInstanceCreateInfo& o ) { o.transform    = ParseTransform( v ); } },
        { "model",     []( rapidjson::Value& v, ModelInstanceCreateInfo& o ) { o.modelName    = v.GetString(); } },
        { "material",  []( rapidjson::Value& v, ModelInstanceCreateInfo& o ) { o.materialName = v.GetString(); } },
    });

    ModelInstanceCreateInfo info;
    mapping.ForEachMember( value, info );

    std::shared_ptr< Model > model = ResourceManager::GetModel( info.modelName );
    assert( model );
    std::shared_ptr< Material > material = nullptr;
    if ( info.materialName != "" )
    {
        material = ResourceManager::GetMaterial( info.materialName );
        assert( material );
    }
    for ( auto& mesh : model->meshes )
    {
        auto meshInstance = std::make_shared< MeshInstance >( mesh, info.transform, material );
        meshInstance->EmitTrianglesAndLights( scene->shapes, scene->lights, meshInstance );
    }
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

static void ParseSamplesPerAreaLight( rapidjson::Value& value, Scene* scene )
{
    scene->numSamplesPerAreaLight = value.GetInt();
}

static void ParseSamplesPerPixel( rapidjson::Value& value, Scene* scene )
{
    scene->numSamplesPerPixel = value.GetInt();
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

static void ParseSphere( rapidjson::Value& value, Scene* scene )
{
    static FunctionMapper< void, Sphere& > mapping(
    {
        { "position", []( rapidjson::Value& v, Sphere& s ) { s.position = ParseVec3( v ); } },
        { "rotation", []( rapidjson::Value& v, Sphere& s ) { s.rotation = glm::radians( ParseVec3( v ) ); } },
        { "radius",   []( rapidjson::Value& v, Sphere& s ) { s.radius   = ParseNumber< float >( v ); } },
        { "material", []( rapidjson::Value& v, Sphere& s ) { s.material = ResourceManager::GetMaterial( v.GetString() ); } },
    });

    auto o = std::make_shared< Sphere >();
    scene->shapes.push_back( o );
    mapping.ForEachMember( value, *o );
    o->worldToLocal = Transform( o->position, o->rotation, glm::vec3( o->radius ) ).Inverse();
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
    std::cout << "Loading scene '" << filename << "'..." << std::endl;

    auto document = ParseJSONFile( filename );
    if ( document.IsNull() )
    {
        return false;
    }

    static FunctionMapper< void, Scene* > mapping(
    {
        { "AmbientLight",        ParseAmbientLight },
        { "BackgroundColor",     ParseBackgroundColor },
        { "BVH",                 ParseBVH },
        { "Camera",              ParseCamera },
        { "DirectionalLight",    ParseDirectionalLight },
        { "Material",            ParseMaterial },
        { "MaxDepth",            ParseMaxDepth },
        { "Model",               ParseModel },
        { "ModelInstance",       ParseModelInstance },
        { "OutputImageData",     ParseOutputImageData },
        { "PointLight",          ParsePointLight },
        { "SamplesPerAreaLight", ParseSamplesPerAreaLight },
        { "SamplesPerPixel",     ParseSamplesPerPixel },
        { "Skybox",              ParseSkybox },
        { "Sphere",              ParseSphere },
        { "Texture",             ParseTexture },
    });

    mapping.ForEachMember( document, this );

    float sceneLoadTime = Time::GetDuration( startTime ) / 1000.0f;
    std::cout << "Building BVH..." << std::endl;
    auto bvhTime = Time::GetTimePoint();
    bvh.Build( shapes );
    float bvhBuildTime = Time::GetDuration( bvhTime ) / 1000.0f;

    for ( auto light : lights )
    {
        if ( dynamic_cast< AreaLight* >( light ) )
        {
            light->nSamples = numSamplesPerAreaLight;
        }
    }

    // compute some scene statistics
    size_t numSpheres = 0, numTris = 0;
    size_t numPointLights = 0, numDirectionalLights = 0, numAreaLights = 0;
    for ( const auto& shape : bvh.shapes )
    {
        if ( std::dynamic_pointer_cast< Sphere >( shape ) ) numSpheres += 1;
        else if ( std::dynamic_pointer_cast< Triangle >( shape ) ) numTris += 1;
    }
    for ( auto light : lights )
    {
        if ( dynamic_cast< AreaLight* >( light ) ) numAreaLights += 1;
        else if ( dynamic_cast< PointLight* >( light ) ) numPointLights += 1;
        else if ( dynamic_cast< DirectionalLight* >( light ) ) numDirectionalLights += 1;
    }
    std::cout << "Scene stats:" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Load time: " << sceneLoadTime << " seconds" << std::endl;
    std::cout << "BVH build time: " << bvhBuildTime << " seconds" << std::endl;
    std::cout << "Number of shapes: " << shapes.size() << std::endl;
    std::cout << "\tSpheres: " << numSpheres << std::endl;
    std::cout << "\tTriangles: " << numTris << std::endl;
    std::cout << "Number of lights: " << lights.size() << std::endl;
    std::cout << "\tAreaLight: " << numAreaLights << std::endl;
    std::cout << "\tPointLight: " << numPointLights << std::endl;
    std::cout << "\tDirectionalLights: " << numDirectionalLights << std::endl;

    return true;
}

bool Scene::Intersect( const Ray& ray, IntersectionData& hitData )
{
    hitData.t = FLT_MAX;
    return bvh.Intersect( ray, &hitData );
    //for ( const auto& shape : bvh.shapes )
    //{
    //    shape->Intersect( ray, &hitData );
    //}
    //return hitData.t != FLT_MAX;
}

glm::vec3 Scene::LEnvironment( const Ray& ray )
{
    if ( skybox )
    {
        return glm::vec3( skybox->GetPixel( ray ) );
    }
    return backgroundColor;
}

} // namespace PT
