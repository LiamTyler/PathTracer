#include "resource/model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "intersection_tests.hpp"
#include "utils/time.hpp"
#include <algorithm>
#include <stack>

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

    struct Triangle
    {
        AABB aabb;
        glm::vec3 centroid;
        uint32_t startIndex;
    };

    static void InitLeafNode( std::unique_ptr< BVHNode >& node, Model& model, std::vector< Triangle >& triangles, int start, int end )
    {
        int numTriangles = end - start;
        node->triangles.resize( numTriangles );
        for ( int i = 0; i < numTriangles; ++i )
        {
            // encode the material index of the triangle into the upper 8 bits of the int so
            // that it doesn't have to be calculated during intersection.
            // Lower 24 bits are the first index of the triangle
            int index = triangles[start + i].startIndex;
            int material = 0;
            for ( const auto& mesh : model.meshes )
            {
                if ( mesh.startIndex <= index && index < mesh.startIndex + mesh.numIndices )
                {
                    material = mesh.materialIndex;
                    break;
                }
            }
            assert( index < 1 << 24 );
            assert( model.materials.size() < 1 << 8 );
            node->triangles[i] = BVHTriangleInfo( index, material );
        }
    }

    static std::unique_ptr< BVHNode > BuildBVHInteral( Model& model, std::vector< Triangle >& triangles, int start, int end )
    {
        auto node = std::make_unique< BVHNode >();

        // calculate bounding box of all triangles
        for ( int i = start; i < end; ++i )
        {
            node->aabb.Union( triangles[i].aabb );
        }

        int numTriangles = end - start;
        assert( numTriangles > 0 );
        if ( numTriangles == 1 )
        {
            InitLeafNode( node, model, triangles, start, end );
            return node;
        }

        // split using the longest dimension of the aabb
        int dim   = node->aabb.LongestDimension();
        float mid = node->aabb.Centroid()[dim];

        // sort all the triangles
        Triangle* beginTri    = &triangles[start];
        Triangle* endTri      = &triangles[0] + end;
        Triangle* midTriangle = std::partition( beginTri, endTri, [dim, mid]( const Triangle& tri ) { return tri.centroid[dim] < mid; } );
        
        // if the split did nothing
        if ( midTriangle == endTri || midTriangle == beginTri )
        {
            if ( numTriangles > 4 )
            {
                //std::cout << "Split not great, leaf has " << numTriangles << " triangles" << std::endl;
            }
            InitLeafNode( node, model, triangles, start, end );
        }
        else
        {
            int cutoff  = start + static_cast< int >( midTriangle - &triangles[start] );
            node->left  = BuildBVHInteral( model, triangles, start, cutoff );
            node->right = BuildBVHInteral( model, triangles, cutoff, end );
        }

        return node;
    }

    static void BuildBVH( Model& model )
    {
        assert( model.indices.size() > 0 );
        std::vector< Triangle > triangles( model.indices.size() / 3 );
        for ( uint32_t i = 0; i < static_cast< uint32_t >( model.indices.size() ); i += 3 )
        {
            const auto& v0 = model.vertices[model.indices[i + 0]];
            const auto& v1 = model.vertices[model.indices[i + 1]];
            const auto& v2 = model.vertices[model.indices[i + 2]];
            AABB aabb;
            aabb.Union( v0 );
            aabb.Union( v1 );
            aabb.Union( v2 );

            Triangle& tri  = triangles[i / 3];
            tri.aabb       = aabb;
            tri.centroid   = aabb.Centroid();
            tri.startIndex = i;
        }

        model.bvh = BuildBVHInteral( model, triangles, 0, static_cast< int >( triangles.size() ) );
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
                aabb.Union( pos );
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
            indexOffset = static_cast< uint32_t >( indices.size() );
        }

        if ( !ParseMaterials( createInfo.filename, this, scene ) )
        {
            std::cout << "Could not load the model's materials" << std::endl;
            return false;
        }

        std::cout << "Model '" << name << "' loaded in: " << Time::GetDuration( loadTime ) / 1000.0f << " seconds" << std::endl;

        auto bvhTime = Time::GetTimePoint();
        BuildBVH( *this );
        std::cout << "Model '" << name << "' BVH builded in: " << Time::GetDuration( bvhTime ) / 1000.0f << " seconds" << std::endl;

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

    bool Model::IntersectRay( const Ray& ray, IntersectionData& hitData, int& materialIndex ) const
    {
        glm::vec3 invRayDir = glm::vec3( 1.0 ) / ray.direction;
        std::stack< BVHNode* > nodes;
        nodes.push( bvh.get() );

        BVHTriangleInfo closestTri;
        float closestU = -1, closestV;
        float t, u, v;
        while ( !nodes.empty() )
        {
            auto node = nodes.top();
            nodes.pop();
            if ( intersect::RayAABB( ray.position, invRayDir, node->aabb.min, node->aabb.max ) )
            {
                // if this is a leaf node, check each triangle
                if ( !node->triangles.empty() )
                {
                    for ( const auto& tri : node->triangles )
                    {
                        int index0 = tri.Index();
                        const auto& v0 = vertices[indices[index0 + 0]];
                        const auto& v1 = vertices[indices[index0 + 1]];
                        const auto& v2 = vertices[indices[index0 + 2]];
                        if ( intersect::RayTriangle( ray.position, ray.direction, v0, v1, v2, t, u, v ) )
                        {
                            if ( t < hitData.t )
                            {
                                hitData.t  = t;
                                closestTri = tri;
                                closestU   = u;
                                closestV   = v;
                            }
                        }
                    }
                }
                else
                {
                    if ( node->left )
                    {
                        nodes.push( node->left.get() );
                    }
                    if ( node->right )
                    {
                        nodes.push( node->right.get() );
                    }
                }
            }
        }

        if ( closestU != -1 )
        {
            int index0       = closestTri.Index();
            hitData.position = ray.Evaluate( hitData.t );
            const auto& n0   = normals[indices[index0 + 0]];
            const auto& n1   = normals[indices[index0 + 1]];
            const auto& n2   = normals[indices[index0 + 2]];
            u                = closestU;
            v                = closestV;
            hitData.normal   = glm::normalize( ( 1 - u - v ) * n0 + u * n1 + u * n2 );
            materialIndex    = closestTri.Material();
            return true;
        }

        return false;
    }

} // namespace PT
