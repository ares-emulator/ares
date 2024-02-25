#version 450

layout(push_constant) uniform Push
{
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;


const vec3 dt = vec3(65536.0,256.0,1.0);


float GET_RESULT(float A, float B, float C, float D)
{
    return (sign(abs(A-C)+abs(A-D)) - sign(abs(B-C)+abs(B-D))); 
}


float reduce(vec3 color)
{ 
    return dot(color,dt);
}

#pragma stage vertex
layout(location = 0) in  vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/* Default Vertex shader */
void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in  vec2 vTexCoord;
layout(location = 1) in  vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


void main()
{
    vec2 OGL2Size    = params.SourceSize.xy;
    vec2 OGL2InvSize = params.SourceSize.zw;

    // Calculating texel coordinates

    vec2 OGL2Pos = vTexCoord*OGL2Size.xy;
    vec2 fp  = fract(OGL2Pos);
    vec2 dx  = vec2(OGL2InvSize.x,0.0);
    vec2 dy  = vec2(0.0,OGL2InvSize.y);
    vec2 g1  = vec2( OGL2InvSize.x,OGL2InvSize.y);
    vec2 g2  = vec2(-OGL2InvSize.x,OGL2InvSize.y);

    vec2 pC4 = floor(OGL2Pos)/OGL2Size.xy + 0.5*OGL2InvSize;
    vec2 pC8 = pC4 + g1;

    // Reading the texels

    vec3 C0 = texture(Source,pC4-g1).xyz; 
    vec3 C1 = texture(Source,pC4-dy).xyz;
    vec3 C2 = texture(Source,pC4-g2).xyz;
    vec3 D3 = texture(Source,pC4-g2+dx).xyz;
    vec3 C3 = texture(Source,pC4-dx).xyz;
    vec3 C4 = texture(Source,pC4).xyz;
    vec3 C5 = texture(Source,pC4+dx).xyz;
    vec3 D4 = texture(Source,pC8-g2).xyz;
    vec3 C6 = texture(Source,pC4+g2).xyz;
    vec3 C7 = texture(Source,pC4+dy).xyz;
    vec3 C8 = texture(Source,pC8).xyz;
    vec3 D5 = texture(Source,pC8+dx).xyz;
    vec3 D0 = texture(Source,pC4+g2+dy).xyz;
    vec3 D1 = texture(Source,pC8+g2).xyz;
    vec3 D2 = texture(Source,pC8+dy).xyz;
    vec3 D6 = texture(Source,pC8+g1).xyz;

    vec3 p00,p10,p01,p11;

    float c0 = reduce(C0);float c1 = reduce(C1);
    float c2 = reduce(C2);float c3 = reduce(C3);
    float c4 = reduce(C4);float c5 = reduce(C5);
    float c6 = reduce(C6);float c7 = reduce(C7);
    float c8 = reduce(C8);float d0 = reduce(D0);
    float d1 = reduce(D1);float d2 = reduce(D2);
    float d3 = reduce(D3);float d4 = reduce(D4);
    float d5 = reduce(D5);float d6 = reduce(D6);

    /*              Super2xSaI code               */
    /*  Copied from the Dosbox source code        */
    /*  Copyright (C) 2002-2007  The DOSBox Team  */
    /*  License: GNU-GPL                          */
    /*  Adapted by guest(r) on 16.4.2007          */

   
   if (c7 == c5 && c4 != c8) {
            p11 = p01 = C7;
    } else if (c4 == c8 && c7 != c5) {
            p11 = p01 = C4;
    } else if (c4 == c8 && c7 == c5) {
            float r;
            r = GET_RESULT(c5,c4,c6,d1);
            r+= GET_RESULT(c5,c4,c3,c1);
            r+= GET_RESULT(c5,c4,d2,d5);
            r+= GET_RESULT(c5,c4,c2,d4);

            if (r > 0.0)
                    p11 = p01 = C5;
            else if (r < 0.0)
                    p11 = p01 = C4;
            else {
                    p11 = p01 = 0.5*(C4+C5);
            }
    } else {
            if (c5 == c8 && c8 == d1 && c7 != d2 && c8 != d0)
                    p11 = 0.25*(3.0*C8+C7);
            else if (c4 == c7 && c7 == d2 && d1 != c8 && c7 != d6)
                    p11 = 0.25*(3.0*C7+C8);
            else
                    p11 = 0.5*(C7+C8);

            if (c5 == c8 && c5 == c1 && c4 != c2 && c5 != c0)
                    p01 = 0.25*(3.0*C5+C4);
            else if (c4 == c7 && c4 == c2 && c1 != c5 && c4 != d3)
                    p01 = 0.25*(3.0*C4+C5);
            else
                    p01 = 0.5*(C4+C5);
    }

    if (c4 == c8 && c7 != c5 && c3 == c4 && c4 != d2)
            p10 = 0.5*(C7+C4);
    else if (c4 == c6 && c5 == c4 && c3 != c7 && c4 != d0)
            p10 = 0.5*(C7+C4);
    else
            p10 = C7;

    if (c7 == c5 && c4 != c8 && c6 == c7 && c7 != c2)
            p00 = 0.5*(C7+C4);
    else if (c3 == c7 && c8 == c7 && c6 != c4 && c7 != c0)
            p00 = 0.5*(C7+C4);
    else
            p00 = C4;

    // Distributing the final products    

    vec3 color = 0.0.xxx;

    if (fp.x < 0.50)
    { if (fp.y < 0.50) color = p00; else color = p10;}
    else
    { if (fp.y < 0.50) color = p01; else color = p11;}

    FragColor = vec4(color, 1.0);
}
