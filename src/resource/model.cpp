#include "resource/model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "configuration.hpp"
#include "intersection_tests.hpp"
#include "resource/resource_manager.hpp"
#include "utils/time.hpp"
#include <algorithm>
#include <filesystem>
#include <stack>

namespace PT
{
    static std::string TrimWhiteSpace( const std::string& s )
    {
        size_t start = s.find_first_not_of( " \t" );
        size_t end   = s.find_last_not_of( " \t" );
        return s.substr( start, end - start + 1 );
    }

    static std::shared_ptr< Texture > LoadAssimpTexture( const aiMaterial* pMaterial, aiTextureType texType )
    {
        namespace fs = std::filesystem;
        aiString path;
        if ( pMaterial->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
        {
            std::string name = TrimWhiteSpace( path.data );
            if ( ResourceManager::GetTexture( name ) )
            {
                return ResourceManager::GetTexture( name );
            }

            std::string fullPath;
            // search for texture starting with
            if ( fs::exists( RESOURCE_DIR + name ) )
            {
                fullPath = RESOURCE_DIR + name;
            }
            else
            {
                std::string basename = fs::path( name ).filename().string();
                for( auto itEntry = fs::recursive_directory_iterator( RESOURCE_DIR ); itEntry != fs::recursive_directory_iterator(); ++itEntry )
                {
                    std::string itFile = itEntry->path().filename().string();
                    if ( basename == itEntry->path().filename().string() )
                    {
                        fullPath = fs::absolute( itEntry->path() ).string();
                        break;
                    }
                }
            }
                    
            if ( fullPath != "" )
            {
                TextureCreateInfo info;
                info.name     = std::filesystem::path( fullPath ).stem().string();;
                info.filename = fullPath;
                auto ret = std::make_shared< Texture >();
                if ( !ret->Load( info ) )
                {
                    std::cout << "Failed to load texture '" << name << "' with default settings" << std::endl;
                    return nullptr;
                }
                ret->name = name;
                return ret;
            }
            else
            {
                std::cout << "Could not find image file '" << name << "'" << std::endl;
            }
        }
        else
        {
            std::cout << "Could not get texture of type: " << texType << std::endl;
        }

        return nullptr;
    }

    static bool ParseMaterials( const std::string& filename, Model* model, const aiScene* scene )
    {
        model->materials.resize( scene->mNumMaterials );
        for ( uint32_t mtlIdx = 0; mtlIdx < scene->mNumMaterials; ++mtlIdx )
        {
            const aiMaterial* pMaterial = scene->mMaterials[mtlIdx];
            model->materials[mtlIdx] = std::make_shared< Material >();
            aiString name;
            aiColor3D color;
            pMaterial->Get( AI_MATKEY_NAME, name );
            model->materials[mtlIdx]->name = name.C_Str();

            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_DIFFUSE, color );
            model->materials[mtlIdx]->albedo = { color.r, color.g, color.b };

            if ( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
            {
                assert( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) == 1 );
                model->materials[mtlIdx]->albedoTexture = LoadAssimpTexture( pMaterial, aiTextureType_DIFFUSE );
                if ( !model->materials[mtlIdx]->albedoTexture )
                {
                    return false;
                }
            }
        }

        return true;
    }

    Model::~Model()
    {
    }

    bool Model::Load( const ModelCreateInfo& createInfo )
    {
        auto loadTime = Time::GetTimePoint();
        name = createInfo.name;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile( createInfo.filename.c_str(),
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
        if ( !scene )
        {
            std::cout << "Error parsing model file '" << createInfo.filename.c_str() << "': " << importer.GetErrorString() << std::endl;
            return false;
        }

        meshes.resize( scene->mNumMeshes );       
        uint32_t numVertices = 0;
        uint32_t numIndices = 0;    
        for ( size_t i = 0 ; i < meshes.size(); i++ )
        {
            meshes[i].name          = scene->mMeshes[i]->mName.C_Str();
            meshes[i].materialIndex = scene->mMeshes[i]->mMaterialIndex;
            meshes[i].numIndices    = scene->mMeshes[i]->mNumFaces * 3;
            meshes[i].numVertices   = scene->mMeshes[i]->mNumVertices;
            meshes[i].startIndex    = numIndices;
        
            numVertices += scene->mMeshes[i]->mNumVertices;
            numIndices  += meshes[i].numIndices;
        }
        vertices.reserve( numVertices );
        normals.reserve( numVertices );
        uvs.reserve( numVertices );
        tangents.reserve( numVertices );
        indices.reserve( numIndices );
        triangleMaterialIndices.reserve( numIndices / 3 );

        uint32_t indexOffset = 0; 
        for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
        {
            const aiMesh* paiMesh = scene->mMeshes[meshIdx];
            const aiVector3D Zero3D( 0.0f, 0.0f, 0.0f );

            for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
            {
                const aiVector3D* pPos    = &paiMesh->mVertices[vIdx];
                const aiVector3D* pNormal = &paiMesh->mNormals[vIdx];
                glm::vec3 pos( pPos->x, pPos->y, pPos->z );
                vertices.emplace_back( pos );
                normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );

                if ( paiMesh->HasTextureCoords( 0 ) )
                {
                    const aiVector3D* pTexCoord = &paiMesh->mTextureCoords[0][vIdx];
                    uvs.emplace_back( pTexCoord->x, pTexCoord->y );
                }
                else
                {
                    uvs.emplace_back( 0, 0 );
                }
                if ( paiMesh->HasTangentsAndBitangents() )
                {
                    const aiVector3D* pTangent = &paiMesh->mTangents[vIdx];
                    glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                    const glm::vec3& n = normals[vIdx];
                    t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                    tangents.emplace_back( t );
                }
            }

            for ( size_t iIdx = 0; iIdx < paiMesh->mNumFaces; ++iIdx )
            {
                const aiFace& face = paiMesh->mFaces[iIdx];
                indices.push_back( face.mIndices[0] + indexOffset );
                indices.push_back( face.mIndices[1] + indexOffset );
                indices.push_back( face.mIndices[2] + indexOffset );
            }
            indexOffset = static_cast< uint32_t >( vertices.size() );
        }

        if ( !ParseMaterials( createInfo.filename, this, scene ) )
        {
            std::cout << "Could not load the model's materials" << std::endl;
            return false;
        }
        RecalculateNormals();
        std::cout << "Model '" << name << "' loaded in: " << Time::GetDuration( loadTime ) / 1000.0f << " seconds" << std::endl;

        return true;
    }

    void Model::RecalculateNormals()
    {
        normals.resize( vertices.size(), glm::vec3( 0 ) );

        for ( size_t i = 0; i < indices.size(); i += 3 )
        {
            glm::vec3 v1 = vertices[indices[i + 0]];
            glm::vec3 v2 = vertices[indices[i + 1]];
            glm::vec3 v3 = vertices[indices[i + 2]];
            glm::vec3 n = glm::cross( v2 - v1, v3 - v1 );
            normals[indices[i + 0]] += n;
            normals[indices[i + 1]] += n;
            normals[indices[i + 2]] += n;
        }

        for ( auto& normal : normals )
        {
            normal = glm::normalize( normal );
        }
    }

} // namespace PT
