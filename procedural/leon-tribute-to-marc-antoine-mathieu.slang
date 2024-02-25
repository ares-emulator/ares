#version 450
// Tribute to Marc-Antoine Mathieu -  leon - 2017-11-21
// https://www.shadertoy.com/view/XlfBR7

// Raymarching skectch inspired by the work of Marc-Antoine Mathieu

// Raymarching sketch inspired by the work of Marc-Antoine Mathieu
// Leon 2017-11-21
// using code from IQ, Mercury, LJ, Duke, Koltes

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

// tweak it
#define donut 30.
#define cell 4.
#define height 2.
#define thin .04
#define radius 15.
#define speed 1.

#define STEPS 100.
#define VOLUME 0.001
#define PI 3.14159
#define TAU (2.*PI)
#define time iGlobalTime

// raymarching toolbox
float rng (vec2 seed) { return fract(sin(dot(seed*.1684,vec2(54.649,321.547)))*450315.); }
mat2 rot (float a) { float c=cos(a),s=sin(a); return mat2(c,-s,s,c); }
float sdSphere (vec3 p, float r) { return length(p)-r; }
float sdCylinder (vec2 p, float r) { return length(p)-r; }
float sdDisk (vec3 p, vec3 s) { return max(max(length(p.xz)-s.x, s.y), abs(p.y)-s.z); }
float sdIso(vec3 p, float r) { return max(0.,dot(p,normalize(sign(p))))-r; }
float sdBox( vec3 p, vec3 b ) { vec3 d = abs(p) - b; return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0)); }
float sdTorus( vec3 p, vec2 t ) { vec2 q = vec2(length(p.xz)-t.x,p.y); return length(q)-t.y; }
float amod (inout vec2 p, float count) { float an = TAU/count; float a = atan(p.y,p.x)+an/2.; float c = floor(a/an); c = mix(c,abs(c),step(count*.5,abs(c))); a = mod(a,an)-an/2.; p.xy = vec2(cos(a),sin(a))*length(p); return c; }
float amodIndex (vec2 p, float count) { float an = TAU/count; float a = atan(p.y,p.x)+an/2.; float c = floor(a/an); c = mix(c,abs(c),step(count*.5,abs(c))); return c; }
float repeat (float v, float c) { return mod(v,c)-c/2.; }
vec2 repeat (vec2 v, vec2 c) { return mod(v,c)-c/2.; }
vec3 repeat (vec3 v, float c) { return mod(v,c)-c/2.; }
float smoo (float a, float b, float r) { return clamp(.5+.5*(b-a)/r, 0., 1.); }
float smin (float a, float b, float r) { float h = smoo(a,b,r); return mix(b,a,h)-r*h*(1.-h); }
float smax (float a, float b, float r) { float h = smoo(a,b,r); return mix(a,b,h)+r*h*(1.-h); }
vec2 displaceLoop (vec2 p, float r) { return vec2(length(p.xy)-r, atan(p.y,p.x)); }
float map (vec3);
float getShadow (vec3 pos, vec3 at, float k) {
    vec3 dir = normalize(at - pos);
    float maxt = length(at - pos);
    float f = 01.;
    float t = VOLUME*50.;
    for (float i = 0.; i <= 1.; i += 1./15.) {
        float dist = map(pos + dir * t);
        if (dist < VOLUME) return 0.;
        f = min(f, k * dist / t);
        t += dist;
        if (t >= maxt) break;
    }
    return f;
}
vec3 getNormal (vec3 p) { vec2 e = vec2(.01,0); return normalize(vec3(map(p+e.xyy)-map(p-e.xyy),map(p+e.yxy)-map(p-e.yxy),map(p+e.yyx)-map(p-e.yyx))); }

void camera (inout vec3 p) {
    p.xz *= rot(PI/8.);
    p.yz *= rot(PI/6.);
}

float windowCross (vec3 pos, vec4 size, float salt) {
    vec3 p = pos;
    float sx = size.x * (.6+salt*.4);
    float sy = size.y * (.3+salt*.7);
    vec2 sxy = vec2(sx,sy);
    p.xy = repeat(p.xy+sxy/2., sxy);
    float scene = sdBox(p, size.zyw*2.);
    scene = min(scene, sdBox(p, size.xzw*2.));
    scene = max(scene, sdBox(pos, size.xyw));
    return scene;
}

float window (vec3 pos, vec2 dimension, float salt) {
    float thinn = .008;
    float depth = .04;
    float depthCadre = .06;
    float padding = .08;
    float scene = windowCross(pos, vec4(dimension,thinn,depth), salt);
    float cadre = sdBox(pos, vec3(dimension, depthCadre));
    cadre = max(cadre, -sdBox(pos, vec3(dimension - padding, depthCadre*2.)));
    scene = min(scene, cadre);
    return scene;
}

float boxes (vec3 pos, float salt) {
    vec3 p = pos;
    float ry = cell * .43*(.3+salt);
    float rz = cell * .2*(.5+salt);
    float salty = rng(vec2(floor(pos.y/ry), floor(pos.z/rz)));
    pos.y = repeat(pos.y, ry);
    pos.z = repeat(pos.z, rz);
    float scene = sdBox(pos, vec3(.1+.8*salt+salty,.1+.2*salt,.1+.2*salty));
    scene = max(scene, sdBox(p, vec3(cell*.2)));
    return scene;
}

float map (vec3 pos) {
    vec3 camOffset = vec3(-4,0,0.);

    float scene = 1000.;
    vec3 p = pos + camOffset;
    float segments = PI*radius;
    float indexX, indexY, salt;
    vec2 seed;

    // donut distortion
    vec3 pDonut = p;
    pDonut.x += donut;
    pDonut.y += radius;
    pDonut.xz = displaceLoop(pDonut.xz, donut);
    pDonut.z *= donut;
    pDonut.xzy = pDonut.xyz;
    pDonut.xz *= rot(time*.05*speed);

    // ground
    p = pDonut;
    scene = min(scene, sdCylinder(p.xz, radius-height));

    // walls
    p = pDonut;
    float py = p.y + time * speed;
    indexY = floor(py / (cell+thin));
    p.y = repeat(py, cell+thin);
    scene = min(scene, max(abs(p.y)-thin, sdCylinder(p.xz, radius)));
    amod(p.xz, segments);
    p.x -= radius;
    scene = min(scene, max(abs(p.z)-thin, p.x));

    // horizontal windot
    p = pDonut;
    p.xz *= rot(PI/segments);
    py = p.y + time * speed;
    indexY = floor(py / (cell+thin));
    p.y = repeat(py, cell+thin);
    indexX = amodIndex(p.xz, segments);
    amod(p.xz, segments);
    seed = vec2(indexX, indexY);
    salt = rng(seed);
    p.x -= radius;
    vec2 dimension = vec2(.75,.5);
    p.x +=  dimension.x * 1.5;
    scene = max(scene, -sdBox(p, vec3(dimension.x, .1, dimension.y)));
    scene = min(scene, window(p.xzy, dimension, salt));

    // vertical window
    p = pDonut;
    py = p.y + cell/2. + time * speed;
    indexY = floor(py / (cell+thin));
    p.y = repeat(py, cell+thin);
    indexX = amodIndex(p.xz, segments);
    amod(p.xz, segments);
    seed = vec2(indexX, indexY);
    salt = rng(seed);
    p.x -= radius;
    dimension.y = 1.5;
    p.x +=  dimension.x * 1.25;
    scene = max(scene, -sdBox(p, vec3(dimension, .1)));
    scene = min(scene, window(p, dimension, salt));

    // elements
    p = pDonut;
    p.xz *= rot(PI/segments);
    py = p.y + cell/2. + time * speed;
    indexY = floor(py / (cell+thin));
    p.y = repeat(py, cell+thin);
    indexX = amodIndex(p.xz, segments);
    amod(p.xz, segments);
    seed = vec2(indexX, indexY);
    salt = rng(seed);
    p.x -= radius - height;
    scene = min(scene, boxes(p, salt));

    return scene;
}

void mainImage( out vec4 color, in vec2 coord ) {
    vec2 uv = (coord.xy-.5*iResolution.xy)/iResolution.y;
    vec3 eye = vec3(0,0,-20);
    vec3 ray = normalize(vec3(uv, 1.3));
    camera(eye);
    camera(ray);
    float dither = rng(uv+fract(time));
    vec3 pos = eye;
    float shade = 0.;
    for (float i = 0.; i <= 1.; i += 1./STEPS) {
        float dist = map(pos);
        if (dist < VOLUME) {
            shade = 1.-i;
            break;
        }
        dist *= .5 + .1 * dither;
        pos += ray * dist;
    }
    vec3 light = vec3(40.,100.,-10.);
    float shadow = getShadow(pos, light, 4.);
    color = vec4(1);
    color *= shade;
    color *= shadow;
    color = smoothstep(.0, .5, color);
    color.rgb = sqrt(color.rgb);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
