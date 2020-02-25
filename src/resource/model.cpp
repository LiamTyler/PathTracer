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
    struct BVHBuildNode
    {
        std::unique_ptr< BVHBuildNode > firstChild;
        std::unique_ptr< BVHBuildNode > secondChild;
        uint32_t firstIndex;
        uint32_t numTriangles;
        AABB aabb;
        uint8_t axis;
    };

    struct Triangle
    {
        AABB aabb;
        glm::vec3 centroid;
        uint32_t startIndex;
    };

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

    static void InitLeafNode( std::unique_ptr< BVHBuildNode >& node, Model& model, std::vector< Triangle >& triangles, std::vector< uint32_t >& reorderedIndexBuffer, int start, int end )
    {
        node->firstIndex = static_cast< uint32_t >( reorderedIndexBuffer.size() );
        node->numTriangles = end - start;
        for ( uint32_t i = 0; i < node->numTriangles; ++i )
        {
            uint32_t index0 = model.indices[triangles[start + i].startIndex + 0];
            uint32_t index1 = model.indices[triangles[start + i].startIndex + 1];
            uint32_t index2 = model.indices[triangles[start + i].startIndex + 2];
            reorderedIndexBuffer.push_back( index0 );
            reorderedIndexBuffer.push_back( index1 );
            reorderedIndexBuffer.push_back( index2 );
            
            // find which material this triangle corresponds to
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
            model.triangleMaterialIndices.push_back( material );
        }
    }

    static std::unique_ptr< BVHBuildNode > BuildBVHInteral( Model& model, std::vector< Triangle >& triangles, int start, int end,
                                                            std::vector< uint32_t >& reorderedIndexBuffer, int& totalNodes )
    {
        auto node = std::make_unique< BVHBuildNode >();
        ++totalNodes;

        // calculate bounding box of all triangles
        for ( int i = start; i < end; ++i )
        {
            node->aabb.Union( triangles[i].aabb );
        }

        int numTriangles = end - start;
        assert( numTriangles > 0 );
        if ( numTriangles == 1 )
        {
            InitLeafNode( node, model, triangles,reorderedIndexBuffer, start, end );
            return node;
        }

        // split using the longest dimension of the aabb containing the centroids
        AABB centroidAABB;
        for ( int i = start; i < end; ++i )
        {
            centroidAABB.Union( triangles[i].centroid);
        }
        int dim    = centroidAABB.LongestDimension();
        node->axis = dim;

        // sort all the triangles
        Triangle* beginTri    = &triangles[start];
        Triangle* endTri      = &triangles[0] + end;
        Triangle* midTriangle;
        /*
        Triangle* midTriangle = std::partition( beginTri, endTri, [dim, mid]( const Triangle& tri ) { return tri.centroid[dim] < mid; } );
        
        // if the split did nothing, fall back to just splitting the nodes equally into two categories
        if ( midTriangle == endTri || midTriangle == beginTri )
        {
            midTriangle = &triangles[(start + end) / 2];
            std::nth_element( beginTri, midTriangle, endTri, [dim]( const Triangle &a, const Triangle &b ) { return a.centroid[dim] < b.centroid[dim]; } );
        }
        */

        // if there are only a few primitives, it doesnt really matter to bother with SAH
        if ( numTriangles <= 4 )
        {
            midTriangle = &triangles[(start + end) / 2];
            std::nth_element( beginTri, midTriangle, endTri, [dim]( const Triangle &a, const Triangle &b ) { return a.centroid[dim] < b.centroid[dim]; } );
        }
        else
        {
            const int nBuckets = 12;
            struct BucketInfo
            {
                AABB aabb;
                int count = 0;
            };
            BucketInfo buckets[nBuckets];

            // bin all of the primitives into their corresponding buckets and update the bucket AABB
            for ( int i = start; i < end; ++i )
            {
                int b = std::min( nBuckets - 1, static_cast< int >( nBuckets * centroidAABB.Offset( triangles[i].centroid )[dim] ) );
                buckets[b].count++;
                buckets[b].aabb.Union( triangles[i].aabb );
            }

            float cost[nBuckets - 1];
            // Compute costs for splitting after each bucket
            for ( int i = 0; i < nBuckets - 1; ++i )
            {
                AABB b0, b1;
                int count0 = 0, count1 = 0;
                for ( int j = 0; j <= i; ++j )
                {
                    b0.Union( buckets[j].aabb );
                    count0 += buckets[j].count;
                }
                for ( int j = i + 1; j < nBuckets; ++j )
                {
                    b1.Union( buckets[j].aabb );
                    count1 += buckets[j].count;
                }
                cost[i] = 0.5f + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / node->aabb.SurfaceArea();
            }

            float minCost = cost[0];
            int minCostSplitBucket = 0;
            for ( int i = 1; i < nBuckets - 1; ++i )
            {
                if ( cost[i] < minCost )
                {
                    minCost = cost[i];
                    minCostSplitBucket = i;
                }
            }

            // see if the split is actually worth it
            int maxTrisInALeaf = 4;
            float leafCost     = (float)numTriangles;
            if ( numTriangles > maxTrisInALeaf || minCost < leafCost )
            {
                midTriangle = std::partition( beginTri, endTri, [&]( const Triangle& tri )
                    {
                        int b = std::min( nBuckets - 1, static_cast< int >( nBuckets * centroidAABB.Offset( tri.centroid )[dim] ) );
                        return b <= minCostSplitBucket;
                    });
            }
            else
            {
                InitLeafNode( node, model, triangles, reorderedIndexBuffer, start, end );
                return node;
            }
        }

        int cutoff        = start + static_cast< int >( midTriangle - &triangles[start] );
        node->firstChild  = BuildBVHInteral( model, triangles, start, cutoff, reorderedIndexBuffer, totalNodes );
        node->secondChild = BuildBVHInteral( model, triangles, cutoff, end, reorderedIndexBuffer, totalNodes );

        return node;
    }

    static int FlattenBVHBuild( BVHNode* linearRoot, BVHBuildNode* buildNode, int& slot )
    {
        if ( !buildNode )
        {
            return -1;
        }

        int currentSlot = slot++;
        linearRoot[currentSlot].aabb         = buildNode->aabb;
        linearRoot[currentSlot].axis         = buildNode->axis;
        linearRoot[currentSlot].numTriangles = buildNode->numTriangles;
        if ( buildNode->numTriangles > 0 )
        {
            linearRoot[currentSlot].firstIndexOffset = buildNode->firstIndex;
        }
        else
        {
            FlattenBVHBuild( linearRoot, buildNode->firstChild.get(), slot );
            linearRoot[currentSlot].secondChildOffset = FlattenBVHBuild( linearRoot, buildNode->secondChild.get(), slot );
        }        

        return currentSlot;
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

        std::vector< uint32_t > newIndexBuffer;
        newIndexBuffer.reserve( model.indices.size() );
        int totalNodes = 0;
        auto bvhHelper = BuildBVHInteral( model, triangles, 0, static_cast< int >( triangles.size() ), newIndexBuffer, totalNodes );
        model.indices  = std::move( newIndexBuffer );

        // flatten the bvh
        model.bvh = new BVHNode[totalNodes];
        int slot  = 0;
        FlattenBVHBuild( &model.bvh[0], bvhHelper.get(), slot );
        assert( slot == totalNodes );
    }

    Model::~Model()
    {
        if ( bvh )
        {
            delete[] bvh;
        }
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

        std::cout << "Model '" << name << "' loaded in: " << Time::GetDuration( loadTime ) / 1000.0f << " seconds" << std::endl;

        auto bvhTime = Time::GetTimePoint();
        BuildBVH( *this );
        std::cout << "Model '" << name << "' BVH built in: " << Time::GetDuration( bvhTime ) / 1000.0f << " seconds" << std::endl;

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

    // stack-based traversal from pbrt
    bool Model::IntersectRay( const Ray& ray, IntersectionData& hitData, int& materialIndex ) const
    {
        glm::vec3 invRayDir = glm::vec3( 1.0 ) / ray.direction;
        int nodesToVisit[64];
        int currentNodeIndex = 0;
        int toVisitOffset    = 0;
        int isDirNeg[3] = { invRayDir.x < 0, invRayDir.y < 0, invRayDir.z < 0 };

        uint32_t closestTriIndex0 = 0;
        float closestU = -1, closestV = -1;
        float t, u, v;
        while ( true )
        {
            const BVHNode& node = bvh[currentNodeIndex];
            if ( intersect::RayAABBFastest( ray.position, invRayDir, isDirNeg, node.aabb.min, node.aabb.max, hitData.t ) )
            {
                // if this  is a leaf node, check each triangle
                if ( node.numTriangles > 0 )
                {
                    for ( int tri = 0; tri < node.numTriangles; ++tri )
                    {
                        int index0     = node.firstIndexOffset + 3 * tri;
                        const auto& v0 = vertices[indices[index0 + 0]];
                        const auto& v1 = vertices[indices[index0 + 1]];
                        const auto& v2 = vertices[indices[index0 + 2]];
                        if ( intersect::RayTriangle( ray.position, ray.direction, v0, v1, v2, t, u, v ) )
                        {
                            if ( t < hitData.t )
                            {
                                hitData.t        = t;
                                closestTriIndex0 = index0;
                                closestU         = u;
                                closestV         = v;
                            }
                        }
                    }
                    if ( toVisitOffset == 0 ) break;
                    currentNodeIndex = nodesToVisit[--toVisitOffset];
                }
                else
                {
                    nodesToVisit[toVisitOffset++] = node.secondChildOffset;
                    currentNodeIndex = currentNodeIndex + 1;
                }
            }
            else
            {
                if ( toVisitOffset == 0 ) break;
                currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
        }

        if ( closestU != -1 )
        {
            hitData.position = ray.Evaluate( hitData.t );
            const auto& n0   = normals[indices[closestTriIndex0 + 0]];
            const auto& n1   = normals[indices[closestTriIndex0 + 1]];
            const auto& n2   = normals[indices[closestTriIndex0 + 2]];
            u                = closestU;
            v                = closestV;
            hitData.normal   = glm::normalize( ( 1 - u - v ) * n0 + u * n1 + u * n2 );
            // hitData.normal   = glm::normalize( u * n0 + v * n1 + ( 1 - u - v ) * n2 );
            materialIndex    = triangleMaterialIndices[closestTriIndex0 / 3];
            return true;
        }

        return false;
    }

} // namespace PT
