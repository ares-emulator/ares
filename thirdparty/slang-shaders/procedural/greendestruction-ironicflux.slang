#version 450
// ironicflux - greendestruction - 2018-02-22
// https://www.shadertoy.com/view/MdcyzB
// The words of these songs were composed without any human intervention whatever on an instrument known as a versificator. But the woman sang so tunefully as to turn the dreadful rubbish into an almost pleasant sound.

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
const vec2 madd = vec2(0.5, 0.5);
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = gl_Position.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
float iGlobalTime = float(global.FrameCount)*0.025;
vec2 iResolution = global.OutputSize.xy;

/* it even comes with a little message */

mat2 rot(float x) {
    return mat2(cos(x), sin(x), -sin(x), cos(x));
}

float map(vec3 p) {
    
    p.z = abs(p.z) - 2.0;
    p.z = abs(p.z) - 2.0;
    
    
    float gt = iGlobalTime * 2.0;
    p.x += gt;
    float d = 1000.0;
    const int n = 12;
    for (int i = 1; i < n; ++i) {
        float x = float(i) / float(n);
        vec3 q = p;
        q.x += x * 3.141592 * 1.0;
        q.z = abs(q.z) - 1.0;
        q.yz *= rot(x * q.x);
        q.y = abs(q.y) - 2.0;
        q.y += x * 3.141592 * 0.5;
        float k = length(q.yz) - 0.125;
        d = min(d, k);
        d = max(d, 1.5 * x + gt - p.x);
    }
    return min(d, 5.0 + gt - p.x);
}

float trace(vec3 o, vec3 r) {
    float t = 0.0;
    for (int i = 0; i < 32; ++i) {
        t += map(o + r * t) * 0.5;
    }
    return t;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;
    
    vec3 r = normalize(vec3(uv, 1.0 - dot(uv, uv) * 0.5));
    vec3 o = vec3(0.0, 0.0, 0.0);
    
    r.xz *= rot(3.141592 * 0.5);
    
    float t = trace(o, r);
    
    vec3 w = o + r * t;
    float fd = map(w);

    float f = 1.0 / (1.0 + t * t * 0.1 + fd * 1000.0);

    vec3 c = vec3(0.0, 0.5, 1.0) * f;
    
    c = mix(c, vec3(1.0), 1.0 / (1.0 + t * t * 0.1));
    
    c = mix(vec3(0.125), c, 1.0 / (1.0 + t * t * 0.1));
    
    vec3 fc = vec3(c);
    
    float b = sign((fract(uv.y * 16.0) - 0.5) / 16.0) * 0.5 + 0.5;
    
    float b2 = sign(0.8 - abs(uv.y)) * 0.5 + 0.5;
    
    fc = mix(fc * vec3(0.5, 0.5, 0.5), fc, b);
    
    fc *= b2;

    fragColor = vec4(fc, 1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
