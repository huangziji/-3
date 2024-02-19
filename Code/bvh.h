#ifndef BVH_H
#define BVH_H
#include <glm/glm.hpp>
using namespace glm;

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
    vec3 vertex0, vertex1, vertex2, centroid;
};

#include <boost/container/vector.hpp>
using boost::container::vector;

class AabbTree
{
public:
    vector<Tri> tri;
    vector<uint> triIdx;
    vector<Node> bvhNode;
    uint rootNodeIdx = 0, nodesUsed = 1;
    void BuildBvh( vector<vec3> const& );
    void UpdateNodeBounds( uint );
    void Subdivide( uint );
};

void AabbTree::BuildBvh(vector<vec3> const& V)
{
    for (int i=0; i<V.size(); i+=3)
    {
        Tri t;
        t.vertex0 = V[i+0];
        t.vertex1 = V[i+1];
        t.vertex2 = V[i+2];
        tri.push_back(t);
    }

    triIdx.resize(tri.size());
    bvhNode.resize(tri.size() * 2 - 1);

    uint rootNodeIdx = 0;
    for (int i=0; i<tri.size(); i++)
    {
        triIdx[i] = i;
        tri[i].centroid = (tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
    }

    // assign all triangles to root node
    Node& root = bvhNode[rootNodeIdx];
    root.leftFirst = 0, root.primCount = tri.size();
    UpdateNodeBounds( rootNodeIdx );
    // subdivide recursively
    Subdivide( rootNodeIdx );
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

void AabbTree::Subdivide(uint nodeIdx)
{
    using std::swap;
    // terminate recursion
    Node& node = bvhNode[nodeIdx];
    if (node.primCount <= 2) return;
    // determine split axis and position
    vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
    // in-place partition
    int i = node.leftFirst;
    int j = i + node.primCount - 1;
    while (i <= j)
    {
        if (tri[triIdx[i]].centroid[axis] < splitPos)
            i++;
        else
            swap( triIdx[i], triIdx[j--] );
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
