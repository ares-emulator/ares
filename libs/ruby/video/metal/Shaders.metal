//
//  Shaders.metal
//  ares
//
//  Created by jcm on 2024-03-07.
//

#include <metal_stdlib>
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

using namespace metal;

// Vertex shader outputs and fragment shader inputs
struct RasterizerData
{
    // The [[position]] attribute of this member indicates that this value
    // is the clip space position of the vertex when this structure is
    // returned from the vertex function.
    float4 position [[position]];

    // Since this member does not have a special attribute, the rasterizer
    // interpolates its value with the values of the other triangle vertices
    // and then passes the interpolated value to the fragment shader for each
    // fragment in the triangle.
    float2 textureCoordinate;
};

vertex RasterizerData
vertexShader(uint vertexID [[vertex_id]],
             constant MetalVertex *vertices [[buffer(MetalVertexInputIndexVertices)]],
             constant vector_uint2 *viewportSizePointer [[buffer(MetalVertexInputIndexViewportSize)]])
{
    RasterizerData out;

    // Index into the array of positions to get the current vertex.
    // The positions are specified in pixel dimensions (i.e. a value of 100
    // is 100 pixels from the origin).
    float2 pixelSpacePosition = vertices[vertexID].position.xy;

    // Get the viewport size and cast to float.
    vector_float2 viewportSize = vector_float2(*viewportSizePointer);
    

    // To convert from positions in pixel space to positions in clip-space,
    //  divide the pixel coordinates by half the size of the viewport.
    out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
    out.position.xy = pixelSpacePosition / (viewportSize / 2.0);

    // Pass the input color directly to the rasterizer.
    out.textureCoordinate = vertices[vertexID].textureCoordinate;

    return out;
}

fragment float4
samplingShader(RasterizerData in [[stage_in]],
               texture2d<half> colorTexture [[ texture(MetalTextureIndexBaseColor) ]])
{
    constexpr sampler textureSampler (mag_filter::nearest,
                                      min_filter::nearest);

    // Sample the texture to obtain a color
    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);

    // return the color of the texture
    return float4(colorSample);
}

fragment float4
drawableSamplingShader(RasterizerData in [[stage_in]],
               texture2d<half> colorTexture [[ texture(MetalTextureIndexBaseColor) ]])
{
    // We use this shader to sample the intermediate texture onto the screen texture;
    // both textures are identical in size. Despite that, if we use nearest neighbor
    // filtering, we end up with significant interference patterns at some scales,
    // probably due to float rounding lower down in the system, if I were to guess.
    // Linear filtering solves this problem. We could also blit, probably.
    constexpr sampler textureSampler (mag_filter::linear,
                                      min_filter::linear);

    // Sample the texture to obtain a color
    const half4 colorSample = colorTexture.sample(textureSampler, in.textureCoordinate);

    // return the color of the texture
    return float4(colorSample);
}
