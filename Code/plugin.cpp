#include <stdio.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using namespace glm;
using boost::container::vector;
using std::swap;

#if 0
/// @link https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
typedef struct{
    vec3 aabbMin, aabbMax;
    uint leftChild, rightChild;
    uint leftNode, firstTriIdx, triCount;
    bool isLeaf() { return triCount > 0; }
}BvhNode;

typedef struct {
    vec3 centroid;
    vec3 vertex0, vertex1, vertex2;
}Tri;

const int N = 128;
Tri tri[N];
uint triIdx[N];
BvhNode bvhNode[N*2-1];
uint rootNodeIdx = 0, nodesUsed = 1;

void UpdateNodeBounds(uint nodeIdx)
{
    BvhNode& node = bvhNode[nodeIdx];
    node.aabbMin = vec3( 1e30f );
    node.aabbMax = vec3( -1e30f );
    for (uint first = node.firstTriIdx, i = 0; i < node.triCount; i++)
    {
        Tri& leafTri = tri[first + i];
        node.aabbMin = min( node.aabbMin, leafTri.vertex0 );
        node.aabbMin = min( node.aabbMin, leafTri.vertex1 );
        node.aabbMin = min( node.aabbMin, leafTri.vertex2 );
        node.aabbMax = max( node.aabbMax, leafTri.vertex0 );
        node.aabbMax = max( node.aabbMax, leafTri.vertex1 );
        node.aabbMax = max( node.aabbMax, leafTri.vertex2 );
    }
}

void Subdivide(uint nodeIdx)
{    // terminate recursion
    BvhNode& node = bvhNode[nodeIdx];
    if (node.triCount <= 2) return;
    // determine split axis and position
    vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
    // in-place partition
    int i = node.firstTriIdx;
    int j = i + node.triCount - 1;
    while (i <= j)
    {
        if (tri[triIdx[i]].centroid[axis] < splitPos)
            i++;
        else
            swap( triIdx[i], triIdx[j--] );
    }
    // abort split if one of the sides is empty
    int leftCount = i - node.firstTriIdx;
    if (leftCount == 0 || leftCount == node.triCount) return;
    // create child nodes
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;
    bvhNode[leftChildIdx].firstTriIdx = node.firstTriIdx;
    bvhNode[leftChildIdx].triCount = leftCount;
    bvhNode[rightChildIdx].firstTriIdx = i;
    bvhNode[rightChildIdx].triCount = node.triCount - leftCount;
    node.leftNode = leftChildIdx;
    node.triCount = 0;
    UpdateNodeBounds( leftChildIdx );
    UpdateNodeBounds( rightChildIdx );
    // recurse
    Subdivide( leftChildIdx );
    Subdivide( rightChildIdx );
}

void BuildBvh()
{
    for (int i = 0; i < N; i++) tri[i].centroid =
        (tri[i].vertex0 + tri[i].vertex1 + tri[i].vertex2) * 0.3333f;
    // assign all triangles to root node
    BvhNode& root = bvhNode[rootNodeIdx];
    root.leftChild = root.rightChild = 0;
    root.firstTriIdx = 0, root.triCount = N;
    UpdateNodeBounds( rootNodeIdx );
    // subdivide recursively
    Subdivide( rootNodeIdx );
}

typedef struct { vec3 O,D; float t; } Ray;

bool IntersectAABB(Ray &, vec3 aabbMin, vec3 aabbMax)
{
    return false;
}

bool IntersectTri(Ray &, Tri)
{
    return false;
}

void IntersectBVH( Ray& ray, const uint nodeIdx )
{
    BvhNode& node = bvhNode[nodeIdx];
    if (!IntersectAABB( ray, node.aabbMin, node.aabbMax )) return;
    if (node.isLeaf())
    {
        for (uint i = 0; i < node.triCount; i++ )
            IntersectTri( ray, tri[triIdx[node.firstTriIdx + i]] );
    }
    else
    {
        IntersectBVH( ray, node.leftNode );
        IntersectBVH( ray, node.leftNode + 1 );
    }
}
#endif

typedef vector<vec3> Soup;

Soup &operator<<(Soup &a, vec3 b) { a.push_back(b); return a; }

extern "C" void mainAnimation(Soup & V)
{
    static int frame = 0;
    if (!frame++)
    {
        FILE* file = fopen( "../unity.tri", "r" );
        if (file)
        {
            float a, b, c, d, e, f, g, h, i;
            for (;;)
            {
                int eof = fscanf( file, "%f %f %f %f %f %f %f %f %f\n",
                    &a, &b, &c, &d, &e, &f, &g, &h, &i );
                if (eof < 0) break;
                V << vec3( a, b, c );
                V << vec3( d, e, f );
                V << vec3( g, h, i );
            }
            fclose( file );
        }
    }
}
