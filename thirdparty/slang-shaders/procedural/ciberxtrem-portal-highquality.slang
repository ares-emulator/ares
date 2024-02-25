#version 450
// Portal Gameplay
// ciberxtrem - https://www.shadertoy.com/view/XlsSzM

// Inspired in Portal, Chamber 8.

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

// Thanks iq and the Shadertoy comunity for all the shared knowledge! :)

#define PI 3.14159265359

struct LightStruct {
  vec3 pos;
  vec3 color;
};

struct PortalStruct {
  vec3 pos;
  vec3 front;
  float rotateY;
  float time;
};

struct StateStruct
{
  vec3 posStart;
  vec3 posEnd;
  float lerptime;
  float duration;
  float isEnterTime;
  float enterPortal;
  float exitPortal;
};
    
struct OrientationStruct
{
  vec3 orientation;
  float lerptime;
  float duration;
};
    
PortalStruct portalA;
PortalStruct portalB;

PortalStruct portalsA[8];
StateStruct states[10];
OrientationStruct orientations[15];
LightStruct lights[3];

float t;
vec3 position;
vec3 orientation;
float ambient = 0.05;
vec3 ambientcolor;

vec3 rotateY(vec3 p, float a){
    return vec3(
        p.x*cos(a) + p.z*sin(a),
        p.y,
        p.z*cos(a) - p.x*sin(a)
    );
}

float hash( float n ) {
    return fract(sin(n)*43758.5453123);
}

float noise( in vec2 x ) {
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*157.0;
    return mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
               mix( hash(n+157.0), hash(n+158.0),f.x),f.y);
}

const mat2 m2 = mat2( 0.80, -0.60, 0.60, 0.80 );

float fbm( vec2 p ) {
    float f = 0.0;
    f += 0.5000*noise( p ); p = m2*p*2.02;
    f += 0.2500*noise( p ); p = m2*p*2.03;
    f += 0.1250*noise( p ); p = m2*p*2.01;
    f += 0.0625*noise( p );
    
    return f/0.9375;
}

float dBox(vec3 p, vec3 hsize)
{
    return length(max(abs(p)-hsize,0.0));
}

float udRoundBox( vec3 p, vec3 b, float r )
{
  return length(max(abs(p)-b,0.0))-r;
}

float sdCappedCylinder( vec3 p, vec2 h )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}
//-------------------------------

// Scene
float dElipse(vec3 p, vec3 hsize, float globalSize)
{
    float d = length(max(abs(p)-hsize,0.0));
    vec3 q = p;
    q.y *= 0.6;
    return max(d, length(q)-max(max(hsize.x, hsize.y)*globalSize, hsize.z));
}

vec2 dStructure(vec3 p)
{  
    // Front
    vec2 res = vec2(-1., -1.);
    vec3 q = p - vec3(0., 0.5, 28.25);
    vec2 c = vec2(3., 6.2);
    vec3 q2 = q; q2.xy = mod(q2.xy,c.xy)-0.5*c.xy;
    float d = udRoundBox(q2, vec3(1.35, 3., 1.0), 0.25);
    q = p - vec3(0.0, 9.5, 28.); // Hole for exit
    float d2 = udRoundBox(q, vec3(3.0, 3.25, 2.0), 0.1);
    d = max(d,-d2);
    res = vec2(d, 24.);
    //Back
    q = p - vec3(0., 3.5, -28.0);
    c = vec2(3., 6.2);
    q.x = mod(q.x,c.x)-0.5*c.x;
    d = udRoundBox(q, vec3(1.35, 3., 1.0), 0.25);
    res = mix(vec2(d, 24.), res, step(res.x, d));
    q = p - vec3(0., 0.6, -28.01); // Brown block
    c = vec2(3., 3.0);
    q.xy = mod(q.xy,c.xy)-0.5*c.xy;
    d = udRoundBox(q, vec3(1.35, 1.35, 1.0), 0.25);
    q = p - vec3(0.0, 9.5, -28.); // Hole for exit
    d2 = udRoundBox(q, vec3(3.0, 3.25, 2.0), 0.1);
    d = max(d,-d2);
    res = mix(vec2(d, 20.), res, step(res.x, d));
    //Exit
    vec3 p2 = p; p2.z = abs(p.z);
    q = p2 - vec3(0.0, 9.5, 30.5);
    d = udRoundBox(q, vec3(3.0, 3.25, 0.1), 0.1);
    if(d<res.x){res = vec2(d, 10.);} // magic panel
    q2 = q - vec3(4.5, 0., -1.); q2.x += abs(sin(length(1.5+abs(q2.y)*0.36)));
    d = udRoundBox(q2, vec3(1.5, 4.5, 2.0), 0.1);
    q2 = q - vec3(-4.5, 0., -1.); q2.x -= abs(sin(length(1.5+abs(q2.y)*0.36)));
    d2 = udRoundBox(q2, vec3(1.5, 4.5, 2.0), 0.1);
    d = min(d, d2);
    res = mix(vec2(d, 11.), res, step(res.x, d));
    //Left
    q = p - vec3(-16., 0.5, 0.0);
    c = vec2(3.0, 6.2);
    q2 = q; q2.zy = mod(q2.zy,c)-0.5*c;
    d = udRoundBox(q2, vec3(1., 3., 1.35), 0.25);
    res = mix(vec2(d, 24.), res, step(res.x, d));
    //Right
    q = p - vec3(16.0, 0.5, 0.0);
    c = vec2(3.0, 6.2);
    q.zy = mod(q.zy,c)-0.5*c;
    d = udRoundBox(q, vec3(1., 3., 1.35), 0.25);
    // Hole for window
    q = p - vec3(15.0, 15.5, -9.0);
    d2 = udRoundBox(q, vec3(2.0, 3., 6.0), 0.02);
    d = max(d, -d2);
    res = mix(vec2(d, 24.), res, step(res.x, d));
    // Column
    q = p - vec3(0., 0.6, -23.90);
    c = vec2(3., 3.0);
    q.xy = mod(q.xy,c)-0.5*c;
    d = udRoundBox(q, vec3(1.35, 1.35, 2.70), 0.25);
    d = max(d, -(p.x-6.1));
    res = mix(vec2(d, 24.), res, step(res.x, d));
    // Ceiling
    q = p - vec3(0., 20.4, .0);
    c = vec2(3.0, 3.0);
    q2 = q; q2.xz = mod(q2.xz,c)-0.5*c;
    d = udRoundBox(q2, vec3(1.35, 1., 1.35), 0.25);
    d2 = length(q)-6.;
    d = max(d, -d2);
    res = mix(vec2(d, 23.), res, step(res.x, d));
    //Window
    q = p - vec3(16.0, 15.5, -9.0);
    d = udRoundBox(q, vec3(1.25, 3., 6.0), 0.02); // framework
    d2 = udRoundBox(q, vec3(1.5, 2.6, 5.6), 0.02);
    d = max(d, -d2);
    d = min(d, udRoundBox(q, vec3(1.2, 3., 0.2), 0.02)); // bars
    res = mix(vec2(d, 7.), res, step(res.x, d));
    d = udRoundBox(q, vec3(1.0, 2.6, 5.6), 0.1);
    d -= smoothstep(-1., 1., cos(p.y*15.)) * 0.0025;
    res = mix(vec2(d, 5.), res, step(res.x, d));
    // Dome
    q = p-vec3(0., 21.5, 0.);
    q.y *= 1.0;
    d = length(q) - 6.;
    res = mix(vec2(d, 9.), res, step(res.x, d));
    //Turbina
    q = p - vec3(0.0, 15.0, 0.0);
    q = rotateY(q, t);
    d = dBox(q, vec3(5.5, 0.05, 0.4));
    d2 = dBox(q, vec3(0.4, 0.05, 5.5));
    d = min(d, d2);
    d2 = sdCappedCylinder(q-vec3(0., 0.5, 0.), vec2(0.4, 0.5));
    d = min(d, d2);
    res = mix(vec2(d, 4.), res, step(res.x, d));
    // Front railing
    q = p - vec3(-5.5, 6.5, 23.2);
    d = dBox(q, vec3(9.0, 0.1, 4.0));
    res = mix(vec2(d, 8.), res, step(res.x, d));
    d = udRoundBox(q-vec3(0., 1.6, -4.0), vec3(9.0, 1.7, 0.05), 0.02);
    d2 = udRoundBox(q-vec3(0., 1.6, -4.0), vec3(8.8, 1.5, 0.5), 0.02);
    d = max(d, -d2);
    d = min(d, udRoundBox(q-vec3(8.9, 3.2, 0.0), vec3(0.10, 0.10, 4.0), 0.01));
    res = mix(vec2(d, 7.), res, step(res.x, d));
    
    // Back railing
    q = p - vec3(-0.5, 6.5, -24.2);
    d = dBox(q, vec3(6.0, 0.1, 3.0));
    res = mix(vec2(d, 8.), res, step(res.x, d));
    d = udRoundBox(q-vec3(0.5, 1.6, 3.2), vec3(6.0, 1.7, 0.1), 0.02);
    d2 = udRoundBox(q-vec3(0.5, 1.6, 3.2), vec3(5.8, 1.5, 1.0), 0.02);
    d = max(d, -d2);
    res = mix(vec2(d, 7.), res, step(res.x, d));

    return res;
}

vec2 dWater(vec3 p)
{
    vec3 q = p - vec3(0., -0., .0);
    float d = dBox(q, vec3(20., 0.2, 40.));
    d-=smoothstep(-1., 1., cos(p.x*2.1+t*2.)*cos(p.z*3.5+t*1.6))*0.01;
    float d2 = smoothstep(-1., 1., cos(length(p.xz)*2.-t))*0.05;
    d2 = mix(0., d2, smoothstep(1.0, 10., length(q.xz)));
    d -= d2;
    
    return vec2(d, 3.);
}

vec2 dPlatforms(vec3 p)
{
    // Front Right Platform
    vec2 res = vec2(-1, -1.);
    vec3 q = p - vec3(11.5, 7., 7.0);
    vec2 c = vec2(3.0);
    vec3 q2 = q; q2.xz = mod(q2.xz+vec2(0., 1.0),c)-0.5*c;
    float d = udRoundBox(q2, vec3(1.415, 0.2, 1.415), 0.1);
    d = max(d, q.z-5.); d = max(d, -(q.z+4.)); d = max(d, -(q.x+6.));
    res = vec2(d, 21.);
    //Tube
    d = sdCappedCylinder(q-vec3(0., -4., 0.), vec2(0.65, 3.));
    d = min(d, sdCappedCylinder(q-vec3(0., -7.5, 0.), vec2(1.0, 2.)));
    res = mix(vec2(d, 6.), res, step(res.x, d));
    // Front Left Platform
    vec3 leftPlatPos = vec3(-10.0, 7., -2.0);
    if(t > 29.0){
        float phase = (t-29.)/7.;
    	float seq = mod(phase, 2.);
    	leftPlatPos =mix(vec3(-10, 7., -2.), vec3(-11.6, 7., -23.0), fract(phase));
    }
    q = p - leftPlatPos;
    d = udRoundBox(q, vec3(3.8, 0.02, 3.8), 0.01);
    res = mix(vec2(d, 22.), res, step(res.x, d));
    // Border
    d = udRoundBox(q, vec3(4.0, 0.05, 4.0), 0.1);
    float d2 = udRoundBox(q, vec3(3.5, 0.1, 3.5), 0.1);
    d = max(d, -d2);
    res = mix(vec2(d, 24.), res, step(res.x, d));
    // Crosses
    d = udRoundBox(q-vec3(0., -0.05, 0.), vec3(4.0, 0.08, 0.1), 0.05);
    d = min(d, udRoundBox(q-vec3(0., -0.05, 0.), vec3(0.1, 0.08, 4.0), 0.05));
    res = mix(vec2(d, 24.), res, step(res.x, d));
    //Tube
    d = sdCappedCylinder(q-vec3(0., -1.5, 0.), vec2(0.65, 1.6));
    d = min(d, sdCappedCylinder(q-vec3(0., -5.0, 0.), vec2(1.0, 2.)));
    res = mix(vec2(d, 6.), res, step(res.x, d));
    
    return res;
}

vec4 dBloomObjects(vec3 p)
{
    vec4 res = vec4(-1, -1, 999., 0.);
    //Lamp
    vec3 q = p - vec3(0.0, 14.6, 0.0);
    q.y *= 1.4;
    float d = length(q)-0.8;
    res.zw = vec2(d, 1.);
    res.xy = vec2(d, 7.);
    // FireballBaseStart
    q = p - vec3(10.5, 9.5, 27.5);
    d = length(vec3(q.x, q.y, q.z))-2.5;
    d = max(d, -(q.z+1.5));
    float d2 = (length((vec3(q.x, q.y, (q.z+1.5)*0.8)))-1.5); d2 += cos((p.z+0.5)*4.5)*0.1;
    d = min(d, d2);
    d += mix(0., 1., smoothstep(0.6, 1., cos(q.x*2.)*cos(q.y*2.)*cos(q.z+1.)))*1.0;
    res.xy = mix(vec2(d, 12.), res.xy, step(res.x, d));
    // FireballBaseEnd
    q = p - vec3(-15.9, 9.5, -12.0);
    q = rotateY(q, PI*0.5);
    d = length(vec3(q.x, q.y, q.z))-2.5;
    d = max(d, -(q.z+1.5));
    d = max(d, -(length(q-vec3(0., 0., -5.6))-4.5));
    d += mix(0., 1., smoothstep(0.5, 1., cos(q.x*2.)*cos(q.y*2.)*cos(q.z+1.)))*1.0;
    res.xy = mix(vec2(d, 12.), res.xy, step(res.x, d));
    // Redpoint
    q = p - vec3(-15.4, 9.5, -12.0);
    d = length(q)-0.9;
    res.xy = mix(vec2(d, 22.), res.xy, step(res.x, d));
    res.zw = mix(vec2(d, 0.),res.zw, step(res.z, d));
    // RedPoint
   	q = p - vec3(14.8, 9.5, -12.0);
    d = length(q)-0.01;
    res.xy = mix(vec2(d, 22.), res.xy, step(res.x, d));
    res.zw = mix(vec2(d, 0.),res.zw, step(res.z, d));
    // Fireball
    vec3 firePos = vec3(0.);
    if(t < 20.) {
        float phase = t/4.;
        float phasePart = fract(phase);
        float phaseSeq = mod(phase, 2.);
        firePos = mix( mix(vec3(10., 9.5, 20.5), vec3(10., 9.5, -19.0), phasePart), mix(vec3(10., 9.5, -19.0), vec3(10., 9.5, 20.5), phasePart), step(1.0, phaseSeq));
    } else if(t < 22.3) {
        firePos = mix(vec3(14.5, 10., 7.5), vec3(-14.5, 10., 7.5), smoothstep(20., 22.3, t));
    } else if(t < 24.3) {
        firePos = mix(vec3(-14.5, 10., 7.5), vec3(14.5, 10., 7.5), smoothstep(22.3, 24.3, t));
    } else {
        firePos = mix(vec3(14.5, 9.5, -12.0), vec3(-15.0, 9.5, -12.0), smoothstep(24.3, 27.5, t));
    }
        
    q = p - firePos;
    d = length(vec3(q.x, q.y, q.z))-0.15;
    d += cos(q.x*15.+t*2.)*cos(q.y*12.)*cos(q.z*10.)*0.05;
    res.xy = mix(vec2(d, 13.), res.xy, step(res.x, d));
    res.zw = mix(vec2(d, 0.),res.zw, step(res.z, d));

    return res;
}

vec2 dPortalA(vec3 p)
{
    vec3 q = rotateY(p - portalA.pos, portalA.rotateY);
    float d = dElipse(q, vec3(1.6, 2.6, 0.05), smoothstep(0., 0.15, t-portalA.time)*0.6);
    return vec2(d, 1.);
}

vec2 dPortalB(vec3 p)
{
    vec3 q = rotateY(p - portalB.pos, portalB.rotateY);
    float d = dElipse(q, vec3(1.6, 2.6, 0.05), smoothstep(0., 0.15, t-portalB.time)*0.6);
    return vec2(d, 2.);
}

vec4 map(vec3 p)
{
    vec4 res = dBloomObjects(p);
    vec2 res2 = dStructure(p);
    if(res2.x < res.x) res.xy = res2.xy;
    
    res2 = dWater(p);
    if(res2.x < res.x) res.xy = res2.xy;
    
    res2 = dPortalA(p);
    if(res2.x < res.x) res.xy = res2.xy;
    
    res2 = dPortalB(p);
    if(res2.x < res.x) res.xy = res2.xy;
    
    res2 = dPlatforms(p);
    if(res2.x < res.x) res.xy = res2.xy;
    
    return res;
}

vec4 intersect(vec3 o, vec3 rd, float tmin, float tmax)
{
    float k = tmin;
    vec4 res = vec4(tmax, -1, 999., 0.);
    for(int i=0; i<120; ++i)
    {
        vec4 r = map(o + rd*k); 
        res.zw = mix(r.zw, res.zw, step(res.z, r.z));
        if(r.x < 0.01)
        {
            res.x = k;
            res.y = r.y;
            break;
        }
        
        k+=r.x;
        if(k > tmax)
        {
            break;
        }
    }
    
    return res;
}

vec3 calcnormal(vec3 p)
{
    vec2 e = vec2(0.001, 0.);
    return normalize(
        vec3(map(p+e.xyy).x-map(p-e.xyy).x,
        	 map(p+e.yxy).x-map(p-e.yxy).x,
        	 map(p+e.yyx).x-map(p-e.yyx).x)
    );
}

float calcshadow(vec3 o, vec3 rd, float tmin, float tmax)
{
    float k = tmin;
    float shadow = 1.;
    for(int i = 0; i < 20; ++i)
    {
        vec4 res = map(o + rd*k);
        shadow = min(shadow, res.x*1.5);
        
        k+=res.x;
        
        if(k > tmax)
        {
            break;
        }
    }
    
    return shadow;
}

vec4 mapPortalColor(vec3 p, vec3 portalPos, float rotY, vec4 cristalcolor, vec4 fxcolor)
{
    vec2 q = rotateY(p-portalPos, rotY).xy; q.y *= 0.55;
    float d = length(q) - 1.4 + sin(q.x*10.+t*2.)*cos(q.y*10.+t*2.) * 0.05;
    return mix(cristalcolor, fxcolor, smoothstep(-0.5, 0.2, d));
}

void calculatePosRayDirFromPortals(in PortalStruct portalO, in PortalStruct portalD, in vec3 p, in vec3 rd, out vec3 refpos, out vec3 refdir)
{
    vec3 oRight = cross(vec3(0., 1., 0.), portalO.front);
    vec3 oUp = cross(portalO.front, oRight);
    
    vec3 dRight = cross(vec3(0., 1., 0.), portalD.front);
    vec3 dUp = cross(portalD.front, dRight);
    
    vec3 projRD=vec3(dot(oRight, rd), dot(oUp, rd), dot(portalO.front, rd));
    vec3 localPos = p-portalO.pos;
    vec3 projPos = vec3(dot(localPos, oRight), dot(localPos, oUp), dot(localPos, portalO.front));

    refdir = normalize(-portalD.front*projRD.z + -dRight*projRD.x + dUp*projRD.y);
    refpos = portalD.pos + -dRight*projPos.x + dUp*projPos.y + -portalD.front*projPos.z;
}

vec4 texcube( sampler2D sam, in vec3 p, in vec3 n )
{
	vec4 x = texture( sam, p.yz );
	vec4 y = texture( sam, p.zx );
	vec4 z = texture( sam, p.xy );
	return x*abs(n.x) + y*abs(n.y) + z*abs(n.z);
}

vec2 point2plane( in vec3 p, in vec3 n )
{
	return p.zy*abs(n.x) + p.xz*abs(n.y) + p.xy*abs(n.z);
}

vec4 mapcolor(inout vec3 p, in vec4 res, inout vec3 normal, in vec3 rd, out vec3 refpos, out vec3 refdir, out vec4 lparams)
{
    vec4 color = vec4(0.498, 0.584, 0.619, 1.0); lparams = vec4(1.0, 10., 0., 0.);
    refdir = reflect(rd, normal); refpos = p;

    if(res.y < 1.1) { // PortalA
        color = mapPortalColor(p, portalA.pos, portalA.rotateY, vec4(1., 1., 1., 0.1), vec4(0.0, 0.35, 1.0, 1.));
        calculatePosRayDirFromPortals(portalA, portalB, p, rd, refpos, refdir);
    }
    else if(res.y < 2.1) { // PortalB
        color = mapPortalColor(p, portalB.pos, portalB.rotateY, vec4(0.0, 1., 1.0, 0.1), vec4(0.91, 0.46, 0.07, 1.));
        calculatePosRayDirFromPortals(portalB, portalA, p, rd, refpos, refdir);
    }
    else if(res.y < 3.1) { // Water
        color = vec4(0.254, 0.239, 0.007, 1.0); lparams.xy = vec2(2.0, 50.);
        color.rgb = mix(color.rgb, vec3(0.254, 0.023, 0.007), 1.-smoothstep(0.2, 1., fbm((p.xz+vec2(cos(t+p.x*2.)*0.2, cos(t+p.y*2.)*0.2))*0.5)));
        color.rgb = mix(color.rgb, vec3(0.007, 0.254, 0.058), smoothstep(0.5, 1., fbm((p.xz*0.4+vec2(cos(t+p.x*2.)*0.2, cos(t+p.y*2.)*0.2))*0.5)));
    }
    else if(res.y < 4.1) { // Turbina
        color = vec4(0.447, 0.490, 0.513, 1.0);
    }
    else if(res.y < 5.1) { //Window
        color = vec4(0.662, 0.847, 0.898, 0.6); lparams=vec4(3., 5., 0., 0.9);
    }
    else if(res.y < 6.1) { // Metal tube
        color = vec4(0.431, 0.482, 0.650, 0.6); lparams.xy=vec2(2., 5.);
    }
    else if(res.y < 7.1) {// Plastic
        color = vec4(0.8, 0.8, 0.8, 1.); lparams.xy=vec2(0.5, 1.);
    }
    else if(res.y < 8.1) { //Railing
        color = mix(vec4(1.), vec4(1., 1., 1., 0.), smoothstep(0.2, 0.21, fract(p.x)));
        color = mix(vec4(1.), color, smoothstep(0.2, 0.21, fract(p.z)));
        lparams.xy=vec2(1.0, 1.); refdir = rd;
    }
    else if(res.y < 9.1) { // Reflectance -> can be plastic
        color = vec4(1., 1., 1., 0.1); lparams.xy=vec2(1.0, 10.);
    }
    else if(res.y < 10.1) { // Exit
        vec3 q = p - vec3(1.5, 11.0, -31.);
        color = vec4(0.6, 0.6, 0.6, 0.65);
        color.rgb = mix(vec3(0.749, 0.898, 0.909), color.rgb, smoothstep(2., 10., length(q.xy)));        
        color.rgb += mix(vec3(0.1), vec3(0.), smoothstep(2., 5., length(q.xy)));

        vec3 q2 = q;
        vec2 c = vec2(2., 1.5);
        float velsign = mix(-1., 1., step(0.5, fract(q2.y*0.5)));
        q2.x = mod(velsign*t+q2.x+cos(q2.y*3.)*0.5, 1.8);
        q2.y = mod(q2.y, 1.15);
		float d = max(abs(q2.x)-0.9, abs(q2.y)-0.1);
        color.rgb += mix(vec3(0.286, 0.941, 0.992)*1.6, vec3(0.), smoothstep(-0.1, 0.1, d));
        
        vec3 localp = p - vec3(1.5, 11.0, -31.);
        refpos = vec3(1.5, 11.0, 28.0) + localp;
        lparams=vec4(1.0, 10., 0., 0.1); refdir = rd;
    }
    else if(res.y < 11.1) { // Exit border
        vec3 q = p; q.z = abs(q.z); q = q - vec3(0.0, 9.5, 31.);
        color = vec4(0.8, 0.8, 0.8, 1.);
        float d =length(abs(q.x+cos(q.y*0.5)*0.6 -3.0))-0.06;
        d = min(d, length(abs(q.x+cos(PI+q.y*0.5)*0.6 +3.0))-0.06);        
        color.rgb = mix(vec3(0.286, 0.941, 0.992), color.rgb, smoothstep(0., 0.01, d));
        lparams = mix(vec4(0., 0., 0., 1.), lparams, smoothstep(0., 0.2, d));
    }
    else if(res.y < 12.1) { // Fireball base
        vec3 q = p - vec3(10., 9.5, 26.5);
        color = vec4(1.0, 1.0, 1.0, 1.);
        float d = length(q-vec3(0., 0., -2.5)) - 2.0;
        color = mix(vec4(0.976, 0.423, 0.262, 1.), color, smoothstep(-2., 0.01, d));
    }
    else if(res.y < 13.1) { // Fireball
        color = vec4(1., 0.0, 0.0, 1.0);
        color.rgb = mix(color.rgb, vec3(0.75, 0.94, 0.28), smoothstep(26.5, 27.0, t));
    }
    
    else if(res.y > 19. && res.y < 25.) { // Walls
        
        float rand = fbm(point2plane(p, normal));
        vec3 col = vec3(0.498, 0.584, 0.619);
        color = vec4(vec3(col), 1.0);
        color = mix(color, vec4(col*0.75, 1.0), smoothstep(0.2, 1.0, rand));
        color = mix(color, vec4(col*0.80, 1.0), smoothstep(0.4, 1.0, fbm(point2plane(p*1.5, normal))));
        color = mix(color, vec4(col*0.7, 1.0), smoothstep(0.6, 1.0, fbm(point2plane(p*4.5, normal))));
        
        vec3 dirtcolor = mix(vec3(0., 0., 0.), vec3(0.403, 0.380, 0.274)*0.2, rand);
        float dirtheight = 0.1+rand*1.0;
        dirtcolor = mix(dirtcolor, vec3(0.243, 0.223, 0.137), smoothstep(dirtheight, dirtheight + 0.5, p.y));
        dirtheight = rand*2.;
        color.rgb = mix(dirtcolor, color.rgb, smoothstep(dirtheight, dirtheight+2.0, p.y));
        
#ifdef TEXTURE
        vec4 noise = mix(vec4(0.), texture(iChannel0, point2plane(p*0.037, normal)) * 0.2, smoothstep(0.2, 1., rand));
#else
        vec4 noise = vec4(0.0, 0.0, 0.0, 0.0);
#endif
        normal = normalize(normal + vec3(noise.x, 0., noise.z));
        refdir = normalize(reflect(rd, normal));
        
        if(res.y < 20.1) { // BROWN_WALL_BLOCK
            float d = -(p.x-6.1);
            d = max(d, p.y-12.6); d = min(d, p.y-6.5);
            color *= mix(vec4(1.), vec4(0.227, 0.137, 0.011, 1.0), smoothstep(0.0, 0.1, d));
        }
        else if(res.y < 21.1) { // WHITE_PLATFORM_BLOCK
            color *= vec4(0.529, 0.572, 0.709, 1.0);
            vec3 q = p - vec3(11.5, 6.85, 7.0);
            float d = abs(q.y)-0.05;
            color.rgb = mix(vec3(0.945, 0.631, 0.015), color.rgb, smoothstep(0., 0.01, d));
            lparams.w = mix(1., 0., smoothstep(0., 0.2, d));
        }
        else if(res.y < 22.1) { // TRANSPARENT_PLATFORM_BLOCK
            color *= vec4(0.431, 0.482, 0.650, 0.1);
            refdir = rd; lparams.xy=vec2(2., 5.);
        }
        else if(res.y < 23.1) { // CEILING_BLOCK
            color *= mix(vec4(0.227, 0.137, 0.011, 1.0), vec4(1.), smoothstep(0., 0.01, p.z+6.));
        }
    }
    return color;
}

void initLights()
{
    vec3 col = vec3(0.925, 0.968, 0.972);
    
    ambientcolor = col;
    
    // Center Up
    lights[0].pos = vec3(0., 13.0, 0.);
    lights[0].color = col*0.25;
    
    // Window
    lights[1].pos = vec3(14.0, 15.5, -12.0);
    lights[1].color = col*0.25;
    
    lights[2].pos = vec3(14.0, 15.5, -6.0);
    lights[2].color = col*0.25;
}

void initPortals()
{
    portalsA[0].pos = vec3(-140.5, 10., 23.0);		portalsA[0].front = vec3(1., 0., 0.);	portalsA[0].rotateY = PI*0.5; portalsA[0].time = 0.;
    portalsA[1].pos = vec3(-14.5, 10., 23.0);		portalsA[1].front = vec3(1., 0., 0.);	portalsA[1].rotateY = PI*0.5; portalsA[1].time = 6.0;
    portalsA[2].pos = vec3(-14.5, 10., -2.5);		portalsA[2].front = vec3(1., 0., 0.);	portalsA[2].rotateY = PI*0.5; portalsA[2].time = 13.5;
    portalsA[3].pos = vec3(10.5, 9.6, -20.5);		portalsA[3].front = vec3(0., 0., 1.);	portalsA[3].rotateY = 0.; 	  portalsA[3].time = 18.;
    portalsA[4].pos = vec3(14.5, 9.5, -12.0);		portalsA[4].front = vec3(-1., 0., 0.);	portalsA[4].rotateY = PI*0.5; portalsA[4].time = 23.5;
    
    portalA = portalsA[0]; 

    portalB.pos = vec3(14.5, 10., 7.5);
    portalB.front = vec3(-1., 0., 0.);
    portalB.rotateY = -PI*0.5; portalB.time = 0.;
}

void initState()
{  
    states[0].posStart = vec3(0., 11.0, 26.0); states[0].posEnd = vec3(0., 11.0, 26.0); states[0].duration = 0.; states[0].lerptime = 0.; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    states[1].posStart = vec3(0., 11.0, 26.0); states[1].posEnd = vec3(0., 11.0, 23.0); states[1].duration = 6.75; states[1].lerptime = 0.5; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    states[2].posStart = vec3(0., 11.0, 23.0); states[2].posEnd = vec3(-12.6, 11.0, 23.0); states[2].duration = 4.0; states[2].lerptime = 1.0; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    
    // Go to Portal B
    states[3].posStart = vec3(-12.6, 11.0, 23.0); states[3].posEnd = vec3(-14.5, 11., 23.0); states[3].duration = 0.1; states[3].lerptime = 0.1; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    states[4].posStart = vec3(14.5, 11., 7.5); states[4].posEnd = vec3(13.5, 11., 7.5); states[4].duration = 3.0; states[4].lerptime = 0.5; states[0].isEnterTime = 1.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    
    // Go to Portal A
    states[5].posStart = vec3(13.5, 11., 7.5); states[5].posEnd = vec3(14.5, 11., 7.5); states[5].duration = 0.1; states[5].lerptime = 1.25; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    states[6].posStart = vec3(-14.5, 11., -2.5); states[6].posEnd = vec3(-11.6, 11., -2.5); states[6].duration = 15.; states[6].lerptime = 1.0; states[0].isEnterTime = 1.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    
    states[7].posStart = vec3(-11.6, 11., -2.5); states[7].posEnd = vec3(-11.6, 11., -23.0); states[7].duration = 7.5; states[7].lerptime = 7.0; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    states[8].posStart = vec3(-11.6, 11., -23.0); states[8].posEnd = vec3(0.5, 11., -23.0); states[8].duration = 3.0; states[8].lerptime = 2.5; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    states[9].posStart = vec3(0.5, 11., -23.0); states[9].posEnd = vec3(0.5, 11., -28.0); states[9].duration = 10.; states[9].lerptime = 0.5; states[0].isEnterTime = 0.; states[0].enterPortal = 0.;  states[0].exitPortal = 1.;
    
    t = mod(t, 40.);
}

void initOrientations()
{
    orientations[0].orientation = vec3(0., 0., -1.); 				orientations[0].lerptime = 0.; orientations[0].duration = 0.;
    orientations[1].orientation = vec3(0., 0., -1.); 				orientations[1].lerptime = 0.0; orientations[1].duration = 1.0;
    orientations[2].orientation = normalize(vec3(0.1, 0.25, -0.5)); orientations[2].lerptime = 1.50; orientations[2].duration = 2.0;
    orientations[3].orientation = normalize(vec3(1., 0., -1.5)); 	orientations[3].lerptime = 0.75; orientations[3].duration = 2.0;
    orientations[4].orientation = vec3(0., 0., -1.); 				orientations[4].lerptime = 1.25; orientations[4].duration = 1.25;
    orientations[4].orientation = vec3(-1., 0., 0.); 				orientations[4].lerptime = 1.0; orientations[4].duration = 6.25;
    orientations[5].orientation = vec3(0., 0., -1.); 				orientations[5].lerptime = 0.75; orientations[5].duration = 1.25;
    orientations[6].orientation = vec3(-1., 0., -0.4);				orientations[6].lerptime = 0.60; orientations[6].duration = 3.25;
    orientations[7].orientation = vec3(0., 0., 1.);					orientations[7].lerptime = 0.75; orientations[7].duration = 0.75;
    orientations[8].orientation = vec3(1., 0., -0.65);				orientations[8].lerptime = 0.75; orientations[8].duration = 3.8;
    orientations[9].orientation = vec3(0.0, 0., 1.);				orientations[9].lerptime = 1.0; orientations[9].duration = 1.5;
    orientations[10].orientation = vec3(1.0, 0., -0.2);				orientations[10].lerptime = 0.75; orientations[10].duration = 3.2;
    orientations[11].orientation = vec3(-0.2, 0., -1.);				orientations[11].lerptime = 2.0; orientations[11].duration = 5.0;
    orientations[12].orientation = normalize(vec3(0.6, -0.5, -1.));	orientations[12].lerptime = 0.5; orientations[12].duration = 5.0;
    orientations[13].orientation = vec3(1., 0., 0.);				orientations[13].lerptime = 0.5; orientations[13].duration = 3.5;
    orientations[14].orientation = vec3(0., 0., -1.);				orientations[14].lerptime = 0.5; orientations[14].duration = 10.0;

    orientation = orientations[0].orientation;
}

vec3 modOrientationToPortals(vec3 or, vec3 enterFront, vec3 exitFront)
{
    vec3 enterRight = cross(vec3(0., 1., 0.), enterFront);
    vec3 enterUp = cross(enterFront, enterRight);
    
    vec3 exitRight = cross(vec3(0., 1., 0.), exitFront);
    vec3 exitUp = cross(exitFront, exitRight);
    
    vec3 enterProjection = vec3
    (
        dot(or, enterRight),
        dot(or, enterUp),
        dot(or, enterFront)
    );
    
    return exitFront*enterProjection.z + exitRight*enterProjection.x + exitUp*enterProjection.y;
}

void updateState()
{   
    vec3 axisEnterFront = vec3(0., 0., -1.);
	vec3 axisExitFront = vec3(0., 0., -1.);
    float portalEnterTime = 0.;
    float at = 0.;
    position = vec3(-5., 11.0, 26.);
    
    for(int i = 4; i >= 0; i--) { // Set current Portal
        if(t > portalsA[i].time) {
            portalA = portalsA[i];
            break;
        }
    }
    for(int i = 1; i < 10; ++i) { // Set Camera position
        at += states[i].duration;
        portalEnterTime = mix(portalEnterTime, at-states[i].duration, step(0.5, states[i].isEnterTime));
        
        vec3 axisEnterFrontA = -mix(portalA.front, portalB.front, step(0.5, states[i].enterPortal));
        axisEnterFront = mix(axisEnterFront, axisEnterFrontA, step(0.5, states[i].isEnterTime));
        
        vec3 axisExitFrontA = mix(portalA.front, portalB.front, step(0.5, states[i].exitPortal));
        axisExitFront = mix(axisExitFront, axisExitFrontA, step(0.5, states[i].isEnterTime));
        if(t < at) {
            position = mix(states[i].posStart, states[i].posEnd, clamp((t - (at-states[i].duration)) / (states[i].lerptime), 0., 1.));
            break;
        }
    }
    at = 0.;
    for(int i = 1; i < 15; ++i) { // Set Camera orientation
        at += orientations[i].duration;
        if(t < at) {
            vec3 prevOrientation = mix(modOrientationToPortals(orientations[i-1].orientation, axisEnterFront, axisExitFront), orientations[i-1].orientation, step(portalEnterTime, at-orientations[i].duration-orientations[i-1].duration));
            vec3 currentOrientation = mix(modOrientationToPortals(orientations[i].orientation, axisEnterFront, axisExitFront), orientations[i].orientation, step(portalEnterTime, at-orientations[i].duration));
            orientation = normalize(mix(prevOrientation, currentOrientation, clamp((t - (at-orientations[i].duration)) / (orientations[i].lerptime), 0., 1.)));
            break;
        }
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    t = iGlobalTime;

    initState();
    initPortals();
    initOrientations();
    initLights();
    
    updateState();
    
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 p = (uv * 2. -1.) * vec2(iResolution.x/iResolution.y, 1.);
#ifdef MOUSE
    vec2 mo = mix(vec2(0.), iMouse.xy / iResolution.xy, step(0.001, length(iMouse.zw)));
#else
    vec2 mo = mix(vec2(0.), 0.0 / iResolution.xy, step(0.001, length(0.0)));
#endif
    
    vec3 right = cross(vec3(0., 1., 0.), orientation);
    orientation += right*mo.x + vec3(0., 1., 0)*mo.y;
    
    vec3 ww = normalize(orientation);
    vec3 uu = normalize(cross(vec3(0., 1., 0.), ww));
	vec3 vv = normalize(cross(ww, uu));
    
    vec3 rd = normalize(p.x*uu*0.7 + p.y*vv + 1.*ww);
    
    float fov = tan(0.46);
    rd = normalize(p.x*uu*fov + p.y*vv*fov + 1.*ww);
    vec3 color = vec3(0.);
    float att = 1.;
    for(int i = 0; i < 3; ++i)
    {
        vec4 res = intersect(position, rd, 0.40, 9999.);
        if(res.y > 0.0)
        {
            vec3 point = position + rd*res.x;
            
            vec3 normal = calcnormal(point);
            vec3 refposition;
            vec3 refdir;
            vec4 lparams;
            vec4 colorrgba = mapcolor(point, res, normal, rd, refposition, refdir, lparams);
            
            float latt = 1.-ambient;
            vec3 acolor = colorrgba.rgb*ambientcolor*ambient;            
            for(int lidx = 0; lidx < 3; ++lidx)
            {
                vec3 ldir = lights[lidx].pos - point;
                float ldirLength = length(ldir);
                ldir /= ldirLength;
                latt *= 1.-clamp((ldirLength-5.)/35., 0., 1.);
                
                vec3 diffuse = colorrgba.rgb;
                float diffactor = max(0., pow(dot(ldir, normal), 1.0))*latt;

                vec3 reflec = reflect(rd, normal);
                float specfactor = pow(max(0., dot(ldir, reflec)), lparams.y) * lparams.x;

                float shadow = 1.;
                shadow =  max(calcshadow(point, ldir, 0.8, ldirLength), 0.01);
                
                acolor += diffuse * diffactor * lights[lidx].color* shadow;
                acolor += specfactor * lights[lidx].color* shadow;
                color += (acolor*colorrgba.a*att);
            }
            
            color = lparams.w*colorrgba.rgb + (1.-lparams.w)*color.rgb;;
            vec3 fireballcolor = mix(vec3(0.91, 0.46, 0.07), vec3(0.42, 0.90, 0.00), smoothstep(26.5, 27.0, t));
            vec3 bloomcolor = mix(fireballcolor, vec3(1.6, 1.65, 1.65), step(0.5, res.w));
    		color.rgb = mix(mix(bloomcolor, color.rgb, 0.5), color.rgb, smoothstep(0., 1.0, res.z));
            
            att = max(att*(1.-colorrgba.a), 0.);
            if(att < 0.01)
            {
                break;
            }
            
            rd = refdir;
            position = refposition;
        }
        else
        {
            att = 0.;
            break;
        }
    }
    
    float fadeFactor = mix(0., 1., smoothstep(39., 40., t));
    fadeFactor = mix(1., fadeFactor, smoothstep(0., 1., t));
    
	fragColor = mix(vec4(color, 1.), vec4(0.), fadeFactor);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragCoord = vTexCoord.xy*global.OutputSize.xy;
  FragCoord.y = -FragCoord.y;
  mainImage(FragColor,FragCoord);
}
