#include "resource/model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "intersection_tests.hpp"

namespace PT
{

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
        }

        return true;
    }

    bool Model::Load( const ModelCreateInfo& createInfo )
    {
        name = createInfo.name;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile( createInfo.filename.c_str(),
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
        if ( !scene )
        {
            std::cout << "Error parsing model file '" << createInfo.filename.c_str() << "': " << importer.GetErrorString() << std::endl;
            return false;
        }

        aabb.min = glm::vec3( FLT_MAX );
        aabb.max = glm::vec3( -FLT_MAX );
        meshes.resize( scene->mNumMeshes );         
        for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
        {
            const aiMesh* assimpMesh = scene->mMeshes[meshIdx];
            Mesh& ptMesh             = meshes[meshIdx];
            ptMesh.name              = assimpMesh->mName.C_Str();
            ptMesh.materialIndex     = assimpMesh->mMaterialIndex;
            ptMesh.vertices.reserve( assimpMesh->mNumVertices );
            ptMesh.normals.reserve( assimpMesh->mNumVertices );
            ptMesh.indices.reserve( assimpMesh->mNumFaces * 3 );
            if ( assimpMesh->HasTextureCoords( 0 ) )
            {
                ptMesh.uvs.reserve( assimpMesh->mNumVertices );
            }
            if ( assimpMesh->HasTangentsAndBitangents() )
            {
                ptMesh.tangents.reserve( assimpMesh->mNumVertices );
            }
            
            for ( uint32_t vIdx = 0; vIdx < assimpMesh->mNumVertices ; ++vIdx )
            {
                const aiVector3D* pPos    = &assimpMesh->mVertices[vIdx];
                const aiVector3D* pNormal = &assimpMesh->mNormals[vIdx];
                glm::vec3 pos             = glm::vec3( pPos->x, pPos->y, pPos->z );
                ptMesh.vertices.emplace_back( pos );
                ptMesh.normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );
                aabb.max = glm::max( aabb.max, pos );
                aabb.min = glm::min( aabb.min, pos );

                if ( assimpMesh->HasTextureCoords( 0 ) )
                {
                    const aiVector3D* pTexCoord = &assimpMesh->mTextureCoords[0][vIdx];
                    ptMesh.uvs.emplace_back( pTexCoord->x, pTexCoord->y );
                }
                if ( assimpMesh->HasTangentsAndBitangents() )
                {
                    const aiVector3D* pTangent = &assimpMesh->mTangents[vIdx];
                    glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                    const glm::vec3& n = ptMesh.normals[vIdx];
                    t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                    ptMesh.tangents.emplace_back( t );
                }
            }

            for ( size_t iIdx = 0; iIdx < assimpMesh->mNumFaces; ++iIdx )
            {
                const aiFace& face = assimpMesh->mFaces[iIdx];
                ptMesh.indices.push_back( face.mIndices[0] );
                ptMesh.indices.push_back( face.mIndices[1] );
                ptMesh.indices.push_back( face.mIndices[2] );
            }
        }

        if ( !ParseMaterials( createInfo.filename, this, scene ) )
        {
            std::cout << "Could not load the model's materials" << std::endl;
            return false;
        }

        return true;
    }

    void Model::RecalculateNormals()
    {
        for ( auto& mesh : meshes )
        {
            mesh.normals.resize( mesh.vertices.size(), glm::vec3( 0 ) );

            for ( size_t i = 0; i < mesh.indices.size(); i += 3 )
            {
                glm::vec3 v1 = mesh.vertices[mesh.indices[i + 0]];
                glm::vec3 v2 = mesh.vertices[mesh.indices[i + 1]];
                glm::vec3 v3 = mesh.vertices[mesh.indices[i + 2]];
                glm::vec3 n = glm::cross( v2 - v1, v3 - v1 );
                mesh.normals[mesh.indices[i + 0]] += n;
                mesh.normals[mesh.indices[i + 1]] += n;
                mesh.normals[mesh.indices[i + 2]] += n;
            }

            for ( auto& normal : mesh.normals )
            {
                normal = glm::normalize( normal );
            }
        }
    }

    bool Model::IntersectRay( const Ray& ray, IntersectionData& hitData, int& materialIndex ) const
    {
        if ( !intersect::RayAABB( ray.position, glm::vec3( 1.0 ) / ray.direction, aabb.min, aabb.max ) )
        {
            return false;
        }

        int closestMeshIndex   = -1;
        int closestIndex       = -1;
        float closestU, closestV;

        float t, u, v;
        for ( int meshIdx = 0; meshIdx < static_cast< int >( meshes.size() ); ++meshIdx )
        {
            const Mesh& mesh = meshes[meshIdx];
            for ( int i = 0; i < (int)mesh.indices.size(); i += 3 )
            {
                const auto& v0 = mesh.vertices[mesh.indices[i + 0]];
                const auto& v1 = mesh.vertices[mesh.indices[i + 1]];
                const auto& v2 = mesh.vertices[mesh.indices[i + 2]];
                if ( intersect::RayTriangle( ray.position, ray.direction, v0, v1, v2, t, u, v ) )
                {
                    if ( t < hitData.t )
                    {
                        hitData.t        = t;
                        closestMeshIndex = meshIdx;
                        closestIndex     = i;
                        closestU         = u;
                        closestV         = v;
                    }
                }
            }
        }

        if ( closestIndex != -1 )
        {
            const Mesh& m    = meshes[closestMeshIndex];
            hitData.position = ray.Evaluate( hitData.t );
            const auto& n0   = m.normals[m.indices[closestIndex + 0]];
            const auto& n1   = m.normals[m.indices[closestIndex + 1]];
            const auto& n2   = m.normals[m.indices[closestIndex + 2]];
            u                = closestU;
            v                = closestV;
            hitData.normal   = glm::normalize( u * n0 + v * n1 + (1 - u - v) * n2 );
            materialIndex    = m.materialIndex;
            return true;
        }

        return false;
    }

} // namespace PT
