cbuffer constants : register(b0) {
    row_major float4x4 cameraprojection;
    row_major float4x4 lightprojection;
    float3 lightrotation;
    float3 modelrotation;
    float4 modeltranslation;

    float2 shadowmapsize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

struct input {
    float3 position;
    float2 texcoord;
};

struct output {
    float4 position : SV_POSITION;
    float4 lightpos : LPS;
    float2 texcoord : TEX;
    nointerpolation float4 color : COL;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

StructuredBuffer<input> vertexbuffer : register(t0);
Texture2D<float> shadowmap : register(t1);

///////////////////////////////////////////////////////////////////////////////////////////////////

float3 get_rotation(uint i) {
    return float3(max(0, float(i / 4) * 2 - 7), i / 4 % 5, i % 4) *
           1.5708f;    // generate XYZ rotation from instance id
}

float4x4 get_rotation_matrix(float3 r) {
    float4x4 x = {1, 0,        0,        0, 0, cos(r.x), -sin(r.x), 0,
                  0, sin(r.x), cos(r.x), 0, 0, 0,        0,         1};
    float4x4 y = {cos(r.y),  0, sin(r.y), 0, 0, 1, 0, 0,
                  -sin(r.y), 0, cos(r.y), 0, 0, 0, 0, 1};
    float4x4 z = {cos(r.z), -sin(r.z), 0, 0, sin(r.z), cos(r.z), 0, 0,
                  0,        0,         1, 0, 0,        0,        0, 1};

    return mul(mul(z, y), x);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

output framebuffer_vs(uint vertexid
                      : SV_VERTEXID, uint instanceid
                      : SV_INSTANCEID) {
    input myinput = vertexbuffer[vertexid];    // manual vertex fetch

    static float4 normal[2] = {{0, 0, -1, 0}, {0, -1, 0, 0}};
    static float3 color[6] = {
        {0.973f, 0.480f, 0.002f}, {0.897f, 0.163f, 0.011f},
        {0.612f, 0.000f, 0.069f}, {0.127f, 0.116f, 0.408f},
        {0.000f, 0.254f, 0.637f}, {0.001f, 0.447f, 0.067f}};

    float4x4 modeltransform = mul(get_rotation_matrix(get_rotation(instanceid)),
                                  get_rotation_matrix(modelrotation));
    float4x4 lighttransform =
        mul(modeltransform, get_rotation_matrix(lightrotation));

    float light = clamp(
        dot(mul(normal[vertexid / 4], lighttransform), normal[0]), 0.0f, 1.0f);

    output myoutput;

    myoutput.position = mul(
        mul(float4(myinput.position, 1.0f), modeltransform) + modeltranslation,
        cameraprojection);
    myoutput.lightpos = mul(
        mul(float4(myinput.position, 1.0f), lighttransform) + modeltranslation,
        lightprojection);
    myoutput.texcoord = myinput.texcoord;
    myoutput.color = float4(color[instanceid / 4], light);

    myoutput.lightpos.xy =
        (myoutput.lightpos.xy * float2(0.5f, -0.5f) + 0.5f) * shadowmapsize;

    return myoutput;
}

float4 framebuffer_ps(output myinput) : SV_TARGET {
    float3 color =
        myinput.color.rgb *
        ((uint(myinput.texcoord.x * 2) ^ uint(myinput.texcoord.y * 2)) & 1
             ? 0.25f
             : 1.0f);    // procedural checkerboard pattern
    float light = myinput.color.a;

    if (light > 0.0f && shadowmap[myinput.lightpos.xy] < myinput.lightpos.z)
        light *= 0.2;

    return float4(color * (light * 0.8f + 0.2f), 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

float4 shadowmap_vs(uint vertexid
                    : SV_VERTEXID, uint instanceid
                    : SV_INSTANCEID)
    : SV_POSITION {
    float4x4 modeltransform = mul(get_rotation_matrix(get_rotation(instanceid)),
                                  get_rotation_matrix(modelrotation));
    float4x4 lighttransform =
        mul(modeltransform, get_rotation_matrix(lightrotation));

    return mul(
        mul(float4(vertexbuffer[vertexid].position, 1.0f), lighttransform) +
            modeltranslation,
        lightprojection);
}