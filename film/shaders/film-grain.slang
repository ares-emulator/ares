#version 450

/*
Film Grain post-process shader v1.1    
Martins Upitis (martinsh) devlog-martinsh.blogspot.com
2013

Original shader:
https://devlog-martinsh.blogspot.com/2013/05/image-imperfections-and-film-grain-post.html

--------------------------
This work is licensed under a Creative Commons Attribution 3.0 Unported License.
So you are free to share, modify and adapt it for your needs, and even use it for commercial use.
I would also love to hear about a project you are using it.

Have fun,
Martins
--------------------------
https://www.shadertoy.com/view/Mly3Rw

Perlin noise shader by toneburst:
http://machinesdontcare.wordpress.com/2009/06/25/3d-perlin-noise-sphere-vertex-shader-sourcecode/
*/


//    Ported by Hyllian - 2024


layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float GRAIN_AMOUNT, COLOR_AMOUNT, GRAIN_SIZE;
    float LUM_AMOUNT;
    float COLORED;
} params;


#pragma parameter GRAIN_AMOUNT "Grain amount"        0.02 0.01 0.2 0.01
#pragma parameter COLOR_AMOUNT "Color amount"        0.6 0.0 1.0 0.1
#pragma parameter GRAIN_SIZE   "Grain particle size" 1.6 1.5 2.5 0.1
#pragma parameter LUM_AMOUNT   "Luminance amount"    1.0 0.0 1.0 0.1
#pragma parameter COLORED      "Use colored noise"   0.0 0.0 1.0 1.0

#define GRAIN_AMOUNT params.GRAIN_AMOUNT
#define COLOR_AMOUNT params.COLOR_AMOUNT
#define GRAIN_SIZE   params.GRAIN_SIZE
#define LUM_AMOUNT   params.LUM_AMOUNT
#define COLORED      params.COLORED

#define NOISE(x) (sin(dot(x+vec2(timer,timer),vec2(12.9898,78.233)))*43758.5453)

float timer = mod(params.FrameCount, 4268.);             // Global timer

const float permTexUnit     = 1.0 / 256.0;               // Perm texture texel-size
const float permTexUnitHalf = 0.5 / 256.0;               // Half perm texture texel-size
const vec3  rotOffset       = vec3(1.425, 3.892, 5.835); // Rotation offset values    
const vec3  lumcoeff        = vec3(0.299, 0.587, 0.114);
const vec4  nRGBA           = vec4(1.0, 1.2154, 1.3453, 1.3647);

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//a random texture generator, but you can also use a pre-computed perturbation texture
float rnmA(vec2 tc)   {return fract(NOISE(tc) * nRGBA.a  ) * 2.0 - 1.0;}
vec2  rnmRG(vec2 tc)  {return fract(NOISE(tc) * nRGBA.rg ) * 2.0 - vec2(1.0);}

vec2  fade(vec2 t)  {return t * t * t * (t * (t * 6.0 - vec2(15.0)) + vec2(10.0));}

float noise2D_contr(vec2 pi, vec2 pf, vec2 pxy, vec2 ptu)
{
    // Noise contributions from (pxy)
    float perm = rnmA(pi.xy + ptu);
    vec2  grad = rnmRG(vec2(perm, 0.0)) * 4.0 - vec2(1.0);

    return dot(grad, pf - pxy);
}

float pnoise2D(vec2 p)
{
    // Integer part, scaled so +1 moves permTexUnit texel
    vec2 pi = permTexUnit * floor(p) + permTexUnitHalf;

    // and offset 1/2 texel to sample texel centers. Fractional part for interpolation
    vec2 pf = fract(p);

    // Noise contributions from (x=0, y=0), (x=0, y=1), (x=1, y=0), (x=1, y=1).
    float n00 = noise2D_contr(pi, pf, vec2(0.0, 0.0), vec2(        0.0,         0.0));
    float n01 = noise2D_contr(pi, pf, vec2(0.0, 1.0), vec2(        0.0, permTexUnit));
    float n10 = noise2D_contr(pi, pf, vec2(1.0, 0.0), vec2(permTexUnit,         0.0));
    float n11 = noise2D_contr(pi, pf, vec2(1.0, 1.0), vec2(permTexUnit, permTexUnit));

    vec2 fd = fade(pf);

    // Blend contributions along x
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fd.x);

    // Blend contributions along y
    float n_xy = mix(n_x.x, n_x.y, fd.y);

    // We're done, return the final noise value
    return n_xy;
}

//2d coordinate orientation thing
vec2 coordRot(vec2 tc, float angle, float width, float height)
{
    float aspect = width / height;
    float rotX   = ((tc.x * 2.0 - 1.0) * aspect * cos(angle)) - ((tc.y * 2.0 - 1.0) * sin(angle));
    float rotY   = ((tc.x * 2.0 - 1.0) * aspect * sin(angle)) + ((tc.y * 2.0 - 1.0) * cos(angle));
    rotX         = ((rotX / aspect) * 0.5 + 0.5);
    rotY         = rotY * 0.5 + 0.5;

    return vec2(rotX, rotY);
}

void main()
{
    float width  = params.SourceSize.x;
    float height = params.SourceSize.y;

    float dx = params.SourceSize.z;
    float dy = params.SourceSize.w;

    vec2 texCoord = vTexCoord;
    
    vec2 rotCoordsR = coordRot(texCoord, timer + rotOffset.x, width, height);
    vec3 noise = vec3(pnoise2D(vec2(rotCoordsR * vec2(width / GRAIN_SIZE, height / GRAIN_SIZE))));
  
    if (COLORED == 1.0)
    {
        vec2 rotCoordsG = coordRot(texCoord, timer + rotOffset.y, width, height);
        vec2 rotCoordsB = coordRot(texCoord, timer + rotOffset.z, width, height);
        noise.g = mix(noise.r, pnoise2D(vec2(rotCoordsG * vec2(width / GRAIN_SIZE, height / GRAIN_SIZE))), COLOR_AMOUNT);
        noise.b = mix(noise.r, pnoise2D(vec2(rotCoordsB * vec2(width / GRAIN_SIZE, height / GRAIN_SIZE))), COLOR_AMOUNT);
    }

    vec3 col = texture(Source, texCoord).rgb;

    //noisiness response curve based on scene luminance
    float luminance = mix(0.0, dot(col, lumcoeff), LUM_AMOUNT);
    float lum = smoothstep(0.2, 0.0, luminance);
    lum += luminance;
    
    
    noise = mix(noise, vec3(0.0), pow(lum, 4.0));
    col   = col + noise * GRAIN_AMOUNT;
   
    FragColor =  vec4(col, 1.0);
}
