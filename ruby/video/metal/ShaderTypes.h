/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Header containing types and enum constants shared between Metal shaders and C/ObjC source
*/
#include <simd/simd.h>

// Buffer index values shared between shader and C code to ensure Metal shader buffer inputs
// match Metal API buffer set calls.
typedef enum MetalVertexInputIndex
{
    MetalVertexInputIndexVertices     = 0,
    MetalVertexInputIndexViewportSize = 1,
} MetalVertexInputIndex;

typedef enum MetalTextureIndex
{
    MetalTextureIndexBaseColor = 0,
} MetalTextureIndex;

//  This structure defines the layout of vertices sent to the vertex
//  shader. This header is shared between the .metal shader and C code, to guarantee that
//  the layout of the vertex array in the C code matches the layout that the .metal
//  vertex shader expects.
typedef struct
{
    vector_float2 position;
    vector_float2 textureCoordinate;
} MetalVertex;
