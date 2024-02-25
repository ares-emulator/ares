#version 450
// M.A.M. Stairs -  leon - 2017-11-24
// https://www.shadertoy.com/view/MIIBR7

// Another raymarching sketch inspired by Marc-Antoine Mathieu.

// M.A.M. Stairs by Leon Denise aka ponk
// another raymarching sketch inspired by Marc-Antoine Mathieu.
// using code from IQ, Mercury, LJ, Duke, Koltes
// made with Atom Editor GLSL viewer (that's why there is 2 space tabulations)
// 2017-11-24

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

#define STEPS 50.
#define VOLUME 0.01
#define PI 3.14159
#define TAU (2.*PI)
#define time iGlobalTime
#define repeat(v,c) (mod(v,c)-c/2.)
#define sDist(v,r) (length(v)-r)

mat2 rot (float a) { float c=cos(a),s=sin(a); return mat2(c,-s,s,c); }
float rng (vec2 seed) { return fract(sin(dot(seed*.1684,vec2(32.649,321.547)))*43415.); }
float sdBox( vec3 p, vec3 b ) { vec3 d = abs(p) - b; return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0)); }
float amod (inout vec2 p, float count) { float an = TAU/count; float a = atan(p.y,p.x)+an/2.; float c = floor(a/an); c = mix(c,abs(c),step(count*.5,abs(c))); a = mod(a,an)-an/2.; p.xy = vec2(cos(a),sin(a))*length(p); return c; }
float aindex (vec2 p, float count) { float an = TAU/count; float a = atan(p.y,p.x)+an/2.; float c = floor(a/an); return mix(c,abs(c),step(count*.5,abs(c))); }
float map (vec3);
vec3 getNormal (vec3 p) { vec2 e = vec2(.001,0); return normalize(vec3(map(p+e.xyy)-map(p-e.xyy),map(p+e.yxy)-map(p-e.yxy),map(p+e.yyx)-map(p-e.yyx))); }
float hardShadow (vec3 pos, vec3 light) {
    vec3 dir = normalize(light - pos);
    float maxt = length(light - pos);
    float t = .02;
    for (float i = 0.; i <= 1.; i += 1./30.) {
        float dist = map(pos + dir * t);
        if (dist < VOLUME) return 0.;
        t += dist;
        if (t >= maxt) break;
    }
    return 1.;
}

float map (vec3 pos) {
  float scene = 1000.;
  float wallThin = .2;
  float wallRadius = 8.;
  float wallOffset = .2;
  float wallCount = 10.;
  float floorThin = .1;
  float stairRadius = 5.;
  float stairHeight = .4;
  float stairCount = 40.;
  float stairDepth = .31;
  float bookCount = 100.;
  float bookRadius = 9.5;
  float bookSpace = 1.75;
  vec3 bookSize = vec3(1.,.2,.2);
  vec3 panelSize = vec3(.03,.2,.7);
  vec2 cell = vec2(1.4,3.);
  float paperRadius = 4.;
  vec3 paperSize = vec3(.3,.01,.4);
  vec3 p;

  // move it
  pos.y += time;

  // twist it
  // pos.xz *= rot(pos.y*.05+time*.1);
  // pos.xz += normalize(pos.xz) * sin(pos.y*.5+time);

  // holes
  float holeWall = sDist(pos.xz, wallRadius);
  float holeStair = sDist(pos.xz, stairRadius);

  // walls
  p = pos;
  amod(p.xz, wallCount);
  p.x -= wallRadius;
  scene = min(scene, max(-p.x, abs(p.z)-wallThin));
  scene = max(scene, -sDist(pos.xz, wallRadius-wallOffset));

  // floors
  p = pos;
  p.y = repeat(p.y, cell.y);
  float disk = max(sDist(p.xz, 1000.), abs(p.y)-floorThin);
  disk = max(disk, -sDist(pos.xz, wallRadius));
  scene = min(scene, disk);

  // stairs
  p = pos;
  float stairIndex = amod(p.xz, stairCount);
  p.y -= stairIndex*stairHeight;
  p.y = repeat(p.y, stairCount*stairHeight);
  float stair = sdBox(p, vec3(100,stairHeight,stairDepth));
  scene = min(scene, max(stair, max(holeWall, -holeStair)));
  p = pos;
  p.xz *= rot(PI/stairCount);
  stairIndex = amod(p.xz, stairCount);
  p.y -= stairIndex*stairHeight;
  p.y = repeat(p.y, stairCount*stairHeight);
  stair = sdBox(p, vec3(100,stairHeight,stairDepth));
  scene = min(scene, max(stair, max(holeWall, -holeStair)));
  p = pos;
  p.y += stairHeight*.5;
  p.y -= stairHeight*stairCount*atan(p.z,p.x)/TAU;
  p.y = repeat(p.y, stairCount*stairHeight);
  scene = min(scene, max(max(sDist(p.xz, wallRadius), abs(p.y)-stairHeight), -holeStair));

  // books
  p = pos;
  p.y -= cell.y*.5;
  vec2 seed = vec2(floor(p.y/cell.y), 0);
  p.y = repeat(p.y, cell.y);
  p.xz *= rot(PI/wallCount);
  seed.y += amod(p.xz, wallCount)/10.;
  seed.y += floor(p.z/(bookSize.z*bookSpace));
  p.z = repeat(p.z, bookSize.z*bookSpace);
  float salt = rng(seed);
  bookSize.x *= .5+.5*salt;
  bookSize.y += salt;
  bookSize.z *= .5+.5*salt;
  p.x -= bookRadius + wallOffset;
  p.x += cos(p.z*2.) - bookSize.x - salt * .25;
  p.x += .01*smoothstep(.99,1.,sin(p.y*(1.+10.*salt)));
  scene = min(scene, max(sdBox(p, vec3(bookSize.x,100.,bookSize.z)), p.y-bookSize.y));

  // panel
  p = pos;
  p.y = repeat(p.y, cell.y);
  p.xz *= rot(PI/wallCount);
  amod(p.xz, wallCount);
  p.x -= wallRadius;
  float panel = sdBox(p, panelSize);
  float pz = p.z;
  p.z = repeat(p.z, .2+.3*salt);
  panel = min(panel, max(sdBox(p, vec3(.1,.1,.04)), abs(pz)-panelSize.z*.8));
  scene = min(scene, panel);

  // papers
  p = pos;
  p.y -= stairHeight;
  p.y += time*2.;
  p.xz *= rot(PI/stairCount);
  float ry = 8.;
  float iy = floor(p.y/ry);
  salt = rng(vec2(iy));
  float a = iy;
  p.xz -= vec2(cos(a),sin(a))*paperRadius;
  p.y = repeat(p.y, ry);
  p.xy *= rot(p.z);
  p.xz *= rot(PI/4.+salt+time);
  scene = min(scene, sdBox(p, paperSize));

  return scene;
}

vec3 getCamera (vec3 eye, vec2 uv) {
  vec3 lookAt = vec3(0.);
#ifdef MOUSE
  float click = clamp(iMouse.w,0.,1.);
  lookAt.x += mix(0.,((iMouse.x/iResolution.x)*2.-1.) * 10., click);
  lookAt.y += mix(0.,iMouse.y/iResolution.y * 10., click);
#else
  float click = clamp(0.,0.,1.);
  lookAt.x += mix(0.,((0./iResolution.x)*2.-1.) * 10., click);
  lookAt.y += mix(0.,0./iResolution.y * 10., click);
#endif
  float fov = .65;
  vec3 forward = normalize(lookAt - eye);
  vec3 right = normalize(cross(vec3(0,1,0), forward));
  vec3 up = normalize(cross(forward, right));
  return normalize(fov * forward + uv.x * right + uv.y * up);
}

float getLight (vec3 pos, vec3 eye) {
  vec3 light = vec3(-.5,7.,1.);
  vec3 normal = getNormal(pos);
  vec3 view = normalize(eye-pos);
  float shade = dot(normal, view);
  shade *= hardShadow(pos, light);
  return shade;
}

vec4 raymarch () {
  vec2 uv = (gl_FragCoord.xy-.5*iResolution.xy)/iResolution.y;
  float dither = rng(uv+fract(time));
  vec3 eye = vec3(0,5,-4.5);
  vec3 ray = getCamera(eye, uv);
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

  vec4 color = vec4(shade);
  color *= getLight(pos, eye);
  color = smoothstep(.0, .5, color);
  color = sqrt(color);
  return color;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	fragColor = raymarch();
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
