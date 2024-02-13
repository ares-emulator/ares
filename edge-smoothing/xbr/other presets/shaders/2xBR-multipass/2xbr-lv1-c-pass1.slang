#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 xBR_lv1_mp_refpassSize;
   vec4 SourceSize;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;

void main()
{
   gl_Position = global.MVP * Position;
   //     A1 B1 C1
   //  A0  A  B  C C4
   //  D0  D  E  F F4
   //  G0  G  H  I I4
   //     G5 H5 I5

   float dx = global.SourceSize.z;
   float dy = global.SourceSize.w;
   
   vTexCoord = TexCoord;
   t1 = vec4(dx, 0, 0, dy);  // F H
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D xBR_lv1_mp_refpass;

mat4x2 sym_vectors  = mat4x2(1,  1,   1, -1,   -1, -1,   -1,  1);

float remapFrom01(float v, float high)
{
   return (high*v + 0.5);
}

vec2 unpack_info(float i)
{
   vec2 info;
   info.x = round(modf(i/2.0f, i));
   info.y = i;

   return info;
}

void main()
{
//get original texture coordinates relative to the current pass
   vec2 OriginalCoord = floor(global.xBR_lv1_mp_refpassSize.xy * vTexCoord);
   OriginalCoord = (OriginalCoord + 0.5) * global.xBR_lv1_mp_refpassSize.zw;
   
   float px, edr; // px = pixel to blend, edr = edge detection rule

   vec2 pos = fract(vTexCoord*global.SourceSize.xy)-vec2(0.5, 0.5); // pos = pixel position
   vec2 dir = sign(pos); // dir = pixel direction

   vec2 g1  = dir*t1.xy;
   vec2 g2  = dir*t1.zw;

   vec3 F   = texture(xBR_lv1_mp_refpass, OriginalCoord +g1).rgb;
   vec3 H   = texture(xBR_lv1_mp_refpass, OriginalCoord +g2).rgb;
   vec3 E   = texture(xBR_lv1_mp_refpass, OriginalCoord    ).rgb;

   vec4 icomp = round(clamp(dir*sym_vectors, vec4(0.0), vec4(1.0))); // choose info component
   float  info  = remapFrom01(dot(texture(Source, vTexCoord), icomp), 255.0f); // retrieve 1st pass info
   vec2 flags = unpack_info(info); // retrieve 1st pass flags

   edr = flags.x;
   px  = flags.y;

   vec3 color = mix(E, mix(H, F, px), edr*0.5); // interpolate if there's edge
   
   FragColor = vec4(color, 1.0);
}
