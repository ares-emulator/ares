#version 450

layout(push_constant) uniform Push
{
   vec4 SourceSize;
   vec4 OriginalSize;
   vec4 OutputSize;
   uint FrameCount;

   float frameShape;
   float frameLimit;
   float frameSharpness;
   float TVNoise;
   float PALSignal;
   float phosphors;
   float border;
} params;

#pragma parameter frameShape "Border mask shape" 0.35 0.0 1.0 0.01
#pragma parameter frameLimit "Border mask limit" 0.30 0.0 1.0 0.01
#pragma parameter frameSharpness "Border mask sharpness" 1.10 0.0 4.0 0.01
#pragma parameter TVNoise "PAL signal modulation + noise" 1.0 0.0 1.0 1.0
#pragma parameter PALSignal "PAL signal simulation" 1.0 0.0 1.0 1.0
#pragma parameter phosphors "Phosphor mask" 1.0 0.0 1.0 1.0
#pragma parameter border "Border mask" 1.0 0.0 1.0 1.0

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


float rand(float x)
{
    vec2 co = vec2(x,x);
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

vec2 crt(vec2 s,float e)
{
   vec2 tempvar = (s - 0.5) * 2.;
   tempvar *= 0.5;
   tempvar.x *= 1.+pow(abs(tempvar.y)/e,2.);
   tempvar.y *= 1.+pow(abs(tempvar.x)/e,2.);
   tempvar = tempvar + .5, tempvar;
   return tempvar;
}

void main()
{
    vec4 col = vec4(0.0);
    vec2 q = vTexCoord.xy;
    vec2 uv = q;
    vec2 uv_q = uv;

    vec2 uv_n = uv_q;
    mat3 rgbtoyuv = mat3(0.299, -0.147,  0.615, 0.587, -0.289, -0.515, 0.114, 0.436, -0.100);
	mat3 yuvtorgb = mat3(1.000, 1.000, 1.000, 0.000, -0.395, 2.032, 1.140, -0.581, 0.000);
    float shade = 1.0;

if(params.TVNoise == 1.0)
{
shade -= rand((uv_q.x*params.FrameCount) * 0.1 + (uv_q.y*params.FrameCount) * 50.0 + params.FrameCount) * 0.5;
}

if(params.PALSignal == 1.0)
{
     vec3 yuv = vec3(0.0);
	float fix = 0.3;
	float lumadelay = -0.002;

  for (int x = 10; x >= 0; x -= 1)
  {
       float xx = float(x) / 10.0;
       if(xx < 0.0) xx = 0.0 ;
       float x1 = (xx * -0.05)* fix + lumadelay;
       float x2 = (xx * 0.1)* fix + lumadelay;
       vec3 mult = (vec3(1.0) - pow(vec3(xx), vec3(0.2, 1.0, 1.0))) * 0.2;
        vec2 uv1 = uv_n + vec2(x1,0.0);
       vec2 uv2 = uv_n + vec2(x2,0.0);
       yuv += (rgbtoyuv * texture(Source,uv1).rgb) * mult;
       yuv += (rgbtoyuv * texture(Source,uv2).rgb) * mult;

  }
  yuv.r = yuv.r * 0.2 + (rgbtoyuv *  texture(Source,uv_n).rgb).r * 0.8;
    col.rgb = yuvtorgb * yuv * shade;
}
else
{
 col.rgb = texture(Source,uv_n).rgb;
}
     
if(params.phosphors==1.0)
{
    float mod_factor = uv.y * params.OutputSize.y * params.OutputSize.y / params.OutputSize.y;
	vec3 dotMaskWeights = mix(vec3(1.0, 0.7, 1.0),vec3(0.7, 1.0, 0.7),floor(mod(mod_factor, 2.0)));
    col.rgb*= dotMaskWeights;
}
 
if(params.border ==1.0)
{
    vec2 p=-1.0+2.0*crt(uv, 2.0);
	float f = (1.0- p.x *p.x) * (1.0-p.y *p.y);
	float frame = clamp(params.frameSharpness * (pow(f, params.frameShape) - params.frameLimit), 0.0, 1.0);
	col.rgb*=frame;
}
    FragColor = vec4(col.rgb, 1.0);
}
