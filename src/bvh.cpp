#include "bvh.hpp"
#include <algorithm>

namespace PT
{

BVH::~BVH()
{
    if ( nodes )
    {
        delete[] nodes;
    }
}

struct BVHBuildNode
{
    std::unique_ptr< BVHBuildNode > firstChild;
    std::unique_ptr< BVHBuildNode > secondChild;
    uint32_t firstIndex;
    uint32_t numShapes;
    AABB aabb = AABB();
    uint8_t axis;
};

// struct to cache AABB info needed during bvh build
struct BVHBuildShapeInfo
{
    AABB aabb;
    glm::vec3 centroid;
    std::shared_ptr< Shape > shape;
};

static std::unique_ptr< BVHBuildNode > BuildBVHInteral( std::vector< BVHBuildShapeInfo >& buildShapeInfos, int start, int end,
    std::vector< std::shared_ptr< Shape > >& orderedShapes, uint32_t& totalNodes, BVH::SplitMethod splitMethod )
{
    auto node = std::make_unique< BVHBuildNode >();
    ++totalNodes;

    // calculate bounding box of all triangles
    for ( int i = start; i < end; ++i )
    {
        node->aabb.Union( buildShapeInfos[i].aabb );
    }

    int numShapes = end - start;
    assert( numShapes > 0 );
    if ( numShapes == 1 )
    {
        node->firstIndex = static_cast< uint32_t >( orderedShapes.size() );
        node->numShapes  = end - start;
        for ( int i = start; i < end; ++i )
        {
            orderedShapes.push_back( buildShapeInfos[i].shape );
        }
        return node;
    }

    // split using the longest dimension of the aabb containing the centroids
    AABB centroidAABB;
    for ( int i = start; i < end; ++i )
    {
        centroidAABB.Union( buildShapeInfos[i].centroid );
    }
    int dim    = centroidAABB.LongestDimension();
    node->axis = dim;

    // sort all the triangles
    BVHBuildShapeInfo* beginShape = &buildShapeInfos[start];
    BVHBuildShapeInfo* endShape   = &buildShapeInfos[0] + end;
    BVHBuildShapeInfo* midShape;   
    
    switch ( splitMethod )
    {
        case BVH::SplitMethod::Middle:
        {
            float mid = (centroidAABB.min[dim] + centroidAABB.max[dim]) / 2;
            midShape  = std::partition( beginShape, endShape, [dim, mid]( const BVHBuildShapeInfo& shape ) { return shape.centroid[dim] < mid; } );

            // if the split did nothing, fall back to just splitting the nodes equally into two categories
            if ( midShape != endShape && midShape != beginShape )
            {
                break;
            }
        }
        case BVH::SplitMethod::EqualCounts:
        {
            midShape = &buildShapeInfos[(start + end) / 2];
            std::nth_element( beginShape, midShape, endShape, [dim]( const BVHBuildShapeInfo &a, const BVHBuildShapeInfo &b ) { return a.centroid[dim] < b.centroid[dim]; } );
            break;
        }
        case BVH::SplitMethod::SAH:
        default:
        {
            // if there are only a few primitives, it doesnt really matter to bother with SAH
            if ( numShapes <= 4 )
            {
                midShape = &buildShapeInfos[(start + end) / 2];
                std::nth_element( beginShape, midShape, endShape, [dim]( const BVHBuildShapeInfo &a, const BVHBuildShapeInfo &b ) { return a.centroid[dim] < b.centroid[dim]; } );
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
                    int b = std::min( nBuckets - 1, static_cast< int >( nBuckets * centroidAABB.Offset( buildShapeInfos[i].centroid )[dim] ) );
                    buckets[b].count++;
                    buckets[b].aabb.Union( buildShapeInfos[i].aabb );
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
                float leafCost       = (float)numShapes;
                int maxShapesPerLeaf = 4;
                if ( numShapes > maxShapesPerLeaf || minCost < leafCost )
                {
                    midShape = std::partition( beginShape, endShape, [&]( const BVHBuildShapeInfo& tri )
                        {
                            int b = std::min( nBuckets - 1, static_cast< int >( nBuckets * centroidAABB.Offset( tri.centroid )[dim] ) );
                            return b <= minCostSplitBucket;
                        });
                }
                else
                {
                    node->firstIndex = static_cast< uint32_t >( orderedShapes.size() );
                    node->numShapes  = end - start;
                    for ( int i = start; i < end; ++i )
                    {
                        orderedShapes.push_back( buildShapeInfos[i].shape );
                    }
                    return node;
                }
            }
        }
    }

    int cutoff        = start + static_cast< int >( midShape - &buildShapeInfos[start] );
    node->firstChild  = BuildBVHInteral( buildShapeInfos, start, cutoff, orderedShapes, totalNodes, splitMethod );
    node->secondChild = BuildBVHInteral( buildShapeInfos, cutoff, end,   orderedShapes, totalNodes, splitMethod );

    return node;
}

static int FlattenBVHBuild( LinearBVHNode* linearRoot, BVHBuildNode* buildNode, uint32_t& slot )
{
    if ( !buildNode )
    {
        return -1;
    }

    int currentSlot = slot++;
    linearRoot[currentSlot].aabb      = buildNode->aabb;
    linearRoot[currentSlot].axis      = buildNode->axis;
    linearRoot[currentSlot].numShapes = buildNode->numShapes;
    if ( buildNode->numShapes > 0 )
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

void BVH::Build( std::vector< std::shared_ptr< Shape > >& listOfShapes )
{
    shapes = std::move( listOfShapes );

    assert( shapes.size() > 0 );
    std::vector< BVHBuildShapeInfo > buildShapes( shapes.size() );
    for ( size_t i = 0; i < shapes.size(); ++i )
    {
        buildShapes[i].aabb     = shapes[i]->WorldSpaceAABB();
        buildShapes[i].centroid = buildShapes[i].aabb.Centroid();
        buildShapes[i].shape    = shapes[i];
    }

    std::vector< std::shared_ptr< Shape > > orderedShapes;
    orderedShapes.reserve( shapes.size() );
    uint32_t totalNodes = 0;
    auto buildRootNode = BuildBVHInteral( buildShapes, 0, static_cast< int >( shapes.size() ), orderedShapes, totalNodes, splitMethod );
    shapes = std::move( orderedShapes );

    // flatten the bvh
    nodes = new LinearBVHNode[totalNodes];
    uint32_t slot  = 0;
    FlattenBVHBuild( &nodes[0], buildRootNode.get(), slot );
    assert( slot == totalNodes );
}

bool BVH::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    int nodesToVisit[64];
    int currentNodeIndex = 0;
    int toVisitOffset    = 0;
    glm::vec3 invRayDir  = glm::vec3( 1.0 ) / ray.direction;
    int isDirNeg[3]      = { invRayDir.x < 0, invRayDir.y < 0, invRayDir.z < 0 };
    float oldMaxT = hitData->t;

    while ( true )
    {
        const LinearBVHNode& node = nodes[currentNodeIndex];
        if ( intersect::RayAABBFastest( ray.position, invRayDir, isDirNeg, node.aabb.min, node.aabb.max, hitData->t ) )
        {
            // if this  is a leaf node, check each triangle
            if ( node.numShapes > 0 )
            {
                for ( int shapeIndex = node.firstIndexOffset; shapeIndex < node.firstIndexOffset + node.numShapes; ++shapeIndex )
                {
                    shapes[shapeIndex]->Intersect( ray, hitData );
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

    return hitData->t < oldMaxT;
}

bool BVH::Occluded( const Ray& ray, float tMax ) const
{
    int nodesToVisit[64];
    int currentNodeIndex = 0;
    int toVisitOffset    = 0;
    glm::vec3 invRayDir  = glm::vec3( 1.0 ) / ray.direction;
    int isDirNeg[3]      = { invRayDir.x < 0, invRayDir.y < 0, invRayDir.z < 0 };

    while ( true )
    {
        const LinearBVHNode& node = nodes[currentNodeIndex];
        if ( intersect::RayAABBFastest( ray.position, invRayDir, isDirNeg, node.aabb.min, node.aabb.max, tMax ) )
        {
            // if this  is a leaf node, check each triangle
            if ( node.numShapes > 0 )
            {
                for ( int shapeIndex = node.firstIndexOffset; shapeIndex < node.firstIndexOffset + node.numShapes; ++shapeIndex )
                {
                    if ( shapes[shapeIndex]->TestIfHit( ray, tMax ) )
                    {
                        return true;
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

    return false;
}

AABB BVH::GetAABB() const
{
    assert( nodes );
    return nodes[0].aabb;
}

} // namespace PT