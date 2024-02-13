#version 450
// Spell Demon's Souls -  leon - 2017-10-21
// https://www.shadertoy.com/view/XljcD1

// A sketch inspired by Dark Souls. Sometime you can see the twisted souls emerging from the distorted shapes. I could tweak this for days and nights...


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


#define STEPS 1./50.
#define VOLUME_BIAS 0.01
#define MIN_DIST 0.005
#define STEP_DAMPING .9
#define PI 3.14159
#define TAU PI*2.

// raymarch toolbox
float rng (vec2 seed) { return fract(sin(dot(seed*.1684,vec2(54.649,321.547)))*450315.); }
mat2 rot (float a) { float c=cos(a),s=sin(a); return mat2(c,-s,s,c); }
float sdSphere (vec3 p, float r) { return length(p)-r; }
float sdCylinder (vec2 p, float r) { return length(p)-r; }
float sdIso(vec3 p, float r) { return max(0.,dot(p,normalize(sign(p))))-r; }
float sdBox( vec3 p, vec3 b ) {
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}
float amod (inout vec2 p, float count) {
    float an = TAU/count;
    float a = atan(p.y,p.x)+an/2.;
    float c = floor(a/an);
    a = mod(a,an)-an/2.;
    p.xy = vec2(cos(a),sin(a))*length(p);
    return c;
}

float repeat (float v, float c) { return mod(v,c)-c/2.; }
float smin (float a, float b, float r) {
    float h = clamp(.5+.5*(b-a)/r, 0., 1.);
    return mix(b,a,h)-r*h*(1.-h);
}

// geometry for spell
float tubes (vec3 pos) {
    
    // cylinder made of 8 tube
    float cylinderRadius = .02; // change shape
    vec3 p = pos;
    p.xz *= rot(p.y*.5); // twist amount
    float c = amod(p.xz, 8.); // amount of tubes
    p.x -= 2.; // tube cylinder radius
    float tube = sdCylinder(p.xz, cylinderRadius);
    
    // another cylinder made of tubes 16
    p = pos;
    p.xz *= rot(-p.y*.5); // twist amount
    c = amod(p.xz, 16.); // amount of tubes
    p.x -= 2.; // tube cylinder radius
    tube = smin(tube, sdCylinder(p.xz, cylinderRadius), .15);
    return tube;
}

// geometry for spell
float disks (vec3 pos) {
    float radius = 1.5;
    float radiusInner = .57;
    float thin = .01;
    float repeatY = 2.;
    float cellY = floor(pos.y/repeatY);
    float a = atan(pos.z,pos.x)-iGlobalTime*.3+cellY*.1;
    vec3 p = pos;
    p.y += sin(a*6.)*.1;
    p.y = repeat(p.y, repeatY);
    float disk = max(-sdCylinder(p.xz, radiusInner), sdCylinder(p.xz, radius));
    disk = max(abs(p.y)-thin,disk);
    return disk;
}

vec3 anim1 (vec3 p) {
    float t = iGlobalTime*.5;
    p.xz *= rot(t);
    p.xy *= rot(t*.7);
    p.yz *= rot(t*.5);
    return p;
}

vec3 anim2 (vec3 p) {
    float t = -iGlobalTime*.4;
    p.xz *= rot(t*.9);
    p.xy *= rot(t*.6);
    p.yz *= rot(t*.3);
    return p;
}

float map (vec3 pos) {
    float scene = 1000.;
    
    // ground and ceiling
#ifdef BUMP
    float bump = texture(iChannel0, pos.xz*.1).r;
#else
    float bump = 0.0;
#endif
    float ground = 2. - bump*.1;
    scene = min(scene, pos.y+ground);
    scene = min(scene, -(pos.y-ground));
    
    // spell geometry 1
    vec3 p = pos;
    p.y += sin(atan(p.z,p.x)*10.)*3.; // change numbers to get new distortion
    p.xz *= rot(p.y*.2-iGlobalTime);
    p = anim1(p);
    p.x = length(p.xyz)-3.;
    scene = smin(scene, tubes(p), .5);
    scene = smin(scene, disks(p), .5);
    
    // spell geometry 2
    p = pos;
    p.y += sin(atan(p.z,p.x)*3.)*2.; // change numbers to get new distortion
    p = anim2(p);
    p.xz *= rot(p.y+iGlobalTime);
    p.x = length(p.xyz)-3.;
    scene = smin(scene, tubes(p), .3);
    scene = smin(scene, disks(p), .3);
    
    return scene;
}

void camera (inout vec3 p) {
#ifdef MOUSE
    p.xz *= rot((-PI*(iMouse.x/iResolution.x-.5)));
#else
    p.xz *= rot((-PI*(0.0/iResolution.x-.5)));
#endif
}

void mainImage( out vec4 color, in vec2 uv )
{
	uv = (uv.xy-.5*iResolution.xy)/iResolution.y;
#ifdef MOUSE
    vec2 mouse = iMouse.xy/iResolution.xy;
#else
    vec2 mouse = 0.0/iResolution.xy;
#endif
    vec3 eye = vec3(0.,0.,-7.+mouse.y*3.);
    vec3 ray = normalize(vec3(uv,.7));
    camera(eye);
    camera(ray);
    vec3 pos = eye;
    float shade = 0.;
    for (float i = 0.; i <= 1.; i += STEPS) {
        float dist = map(pos);
        if (dist < VOLUME_BIAS) {
            shade += STEPS;
        }
        if (shade >= 1.) break;
        dist *= STEP_DAMPING + .1 * rng(uv+fract(iGlobalTime));
        dist = max(MIN_DIST, dist);
        pos += dist * ray;
    }
	color = vec4(1);
    color.rgb *= shade;
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
