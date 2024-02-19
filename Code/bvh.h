#ifndef BVH_H
#define BVH_H
#include <glm/glm.hpp>
using namespace glm;
#include <boost/container/vector.hpp>
using boost::container::vector;

/// @link https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
struct Node
{
    vec3 aabbMin;
    int leftFirst;
    vec3 aabbMax;
    int primCount;
};

struct Tri
{
    vec3 vertex0, vertex1, vertex2;
};

class AabbTree
{
    float FindBestSplitPlane( Node& node, int& axis, float& splitPos );
    void UpdateNodeBounds( uint );
    void Subdivide( uint );

    vector<uint> triIdx;
    vector<vec3> centroid;
    uint rootNodeIdx = 0, nodesUsed = 2;
public:
    vector<Tri> tri;
    vector<Node> bvhNode;
    void BuildBvh( vector<vec3> const& );
};

void AabbTree::BuildBvh(vector<vec3> const& V)
{
    tri.resize(V.size()/3);
    triIdx.resize(tri.size());
    centroid.resize(tri.size());
    bvhNode.resize(tri.size() * 2);

    for (int i=0; i<tri.size(); i++)
    {
        tri[i].vertex0 = V[i*3+0];
        tri[i].vertex1 = V[i*3+1];
        tri[i].vertex2 = V[i*3+2];
        triIdx[i] = i;
        centroid[i] = (tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
    }

    uint rootNodeIdx = 0;
    // assign all triangles to root node
    Node& root = bvhNode[rootNodeIdx];
    root.leftFirst = 0, root.primCount = tri.size();
    UpdateNodeBounds( rootNodeIdx );
    // subdivide recursively
    Subdivide( rootNodeIdx );

    vector<Tri> tmp(tri);
    for (int i=0; i<tmp.size(); i++)
    {
        tri[i] = tmp[triIdx[i]];
    }
}

void AabbTree::UpdateNodeBounds(uint nodeIdx)
{
    Node& node = bvhNode[nodeIdx];
    node.aabbMin = vec3( 1e30f );
    node.aabbMax = vec3( -1e30f );
    for (uint first = node.leftFirst, i = 0; i < node.primCount; i++)
    {
        uint leafTriIdx = triIdx[first + i];
        Tri& leafTri = tri[leafTriIdx];
        node.aabbMin = min( node.aabbMin, leafTri.vertex0 );
        node.aabbMin = min( node.aabbMin, leafTri.vertex1 );
        node.aabbMin = min( node.aabbMin, leafTri.vertex2 );
        node.aabbMax = max( node.aabbMax, leafTri.vertex0 );
        node.aabbMax = max( node.aabbMax, leafTri.vertex1 );
        node.aabbMax = max( node.aabbMax, leafTri.vertex2 );
    }
}

struct Aabb
{
    vec3 bmin = vec3(1e30f), bmax = vec3(-1e30f);
    void grow( vec3 p ) { bmin = min( bmin, p ), bmax = max( bmax, p ); }
    void grow( Aabb& b ) { if (b.bmin.x != 1e30f) { grow( b.bmin ); grow( b.bmax ); } }
    float area()
    {
        vec3 e = bmax - bmin; // box extent
        return e.x * e.y + e.y * e.z + e.z * e.x;
    }
};

float AabbTree::FindBestSplitPlane( Node& node, int& axis, float& splitPos )
{
    const int BINS = 8;
    typedef struct { Aabb bounds; int triCount = 0; }Bin;

    float bestCost = 1e30f;
    for (int a = 0; a < 3; a++)
    {
        float boundsMin = 1e30f, boundsMax = -1e30f;
        for (uint i = 0; i < node.primCount; i++)
        {
            vec3 ce = centroid[triIdx[node.leftFirst + i]];
            boundsMin = min( boundsMin, ce[a] );
            boundsMax = max( boundsMax, ce[a] );
        }
        if (boundsMin == boundsMax) continue;
        // populate the bins
        Bin bin[BINS];
        float scale = BINS / (boundsMax - boundsMin);
        for (uint i = 0; i < node.primCount; i++)
        {
            Tri& triangle = tri[triIdx[node.leftFirst + i]];
            int binIdx = min( BINS - 1, (int)((centroid[triIdx[node.leftFirst + i]][a] - boundsMin) * scale) );
            bin[binIdx].triCount++;
            bin[binIdx].bounds.grow( triangle.vertex0 );
            bin[binIdx].bounds.grow( triangle.vertex1 );
            bin[binIdx].bounds.grow( triangle.vertex2 );
        }
        // gather data for the 7 planes between the 8 bins
        float leftArea[BINS - 1], rightArea[BINS - 1];
        int leftCount[BINS - 1], rightCount[BINS - 1];
        Aabb leftBox, rightBox;
        int leftSum = 0, rightSum = 0;
        for (int i = 0; i < BINS - 1; i++)
        {
            leftSum += bin[i].triCount;
            leftCount[i] = leftSum;
            leftBox.grow( bin[i].bounds );
            leftArea[i] = leftBox.area();
            rightSum += bin[BINS - 1 - i].triCount;
            rightCount[BINS - 2 - i] = rightSum;
            rightBox.grow( bin[BINS - 1 - i].bounds );
            rightArea[BINS - 2 - i] = rightBox.area();
        }
        // calculate SAH cost for the 7 planes
        scale = (boundsMax - boundsMin) / BINS;
        for (int i = 0; i < BINS - 1; i++)
        {
            float planeCost = leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
            if (planeCost < bestCost)
                axis = a, splitPos = boundsMin + scale * (i + 1), bestCost = planeCost;
        }
    }
    return bestCost;
}

float CalculateNodeCost( Node& node )
{
    vec3 e = node.aabbMax - node.aabbMin; // extent of the node
    float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
    return node.primCount * surfaceArea;
}

void AabbTree::Subdivide( uint nodeIdx )
{
    // terminate recursion
    Node& node = bvhNode[nodeIdx];

#if 1
    int axis;
    float splitPos;
    float splitCost = FindBestSplitPlane( node, axis, splitPos );
    float nosplitCost = CalculateNodeCost( node );
    if (splitCost >= nosplitCost) return;
#else
    if (node.primCount <= 2) return;
    // determine split axis and position
    vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
#endif

    // in-place partition
    int i = node.leftFirst;
    int j = i + node.primCount - 1;
    while (i <= j)
    {
        if (centroid[triIdx[i]][axis] < splitPos)
            i++;
        else
            std::swap( triIdx[i], triIdx[j--] );
    }
    // abort split if one of the sides is empty
    int leftCount = i - node.leftFirst;
    if (leftCount == 0 || leftCount == node.primCount) return;
    // create child nodes
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;
    bvhNode[leftChildIdx].leftFirst = node.leftFirst;
    bvhNode[leftChildIdx].primCount = leftCount;
    bvhNode[rightChildIdx].leftFirst = i;
    bvhNode[rightChildIdx].primCount = node.primCount - leftCount;
    node.leftFirst = leftChildIdx;
    node.primCount = 0;
    UpdateNodeBounds( leftChildIdx );
    UpdateNodeBounds( rightChildIdx );
    // recurse
    Subdivide( leftChildIdx );
    Subdivide( rightChildIdx );
}

#endif // BVH_H
