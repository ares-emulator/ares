#version 450
// Mountain Peak -  TDM - 2014-09-26
// https://www.shadertoy.com/view/llK3WR

// Terrain with procedural hydraulic erosion.

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

// "Mountain Peak" by Alexander Alekseev aka TDM - 2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

#define WIND

const int NUM_STEPS = 38;
const int NUM_STEPS_VOLUME = 10;
const float STRIDE = 0.75;
const float STRIDE_VOLUME = 1.0;
const float PI	 	= 3.1415;
const float EPSILON	= 1e-3;

// terrain
const int ITER_GEOMETRY = 4;
const int ITER_FRAGMENT = 7;

const float TERR_HEIGHT = 12.0;
const float TERR_WARP = 0.7;
const float TERR_OCTAVE_AMP = 0.55;
const float TERR_OCTAVE_FREQ = 2.5;
const float TERR_MULTIFRACT = 0.27;
const float TERR_CHOPPY = 1.9;
const float TERR_FREQ = 0.24;
const vec2 TERR_OFFSET = vec2(13.5,15.);

const vec3 SKY_COLOR = vec3(0.5,0.59,0.75) * 0.6;
const vec3 SUN_COLOR = vec3(1.,1.,0.98) * 0.75;
const vec3 COLOR_SNOW = vec3(1.0,1.0,1.1) * 2.0;
const vec3 COLOR_ROCK = vec3(0.0,0.1,0.1);
vec3 light = normalize(vec3(1.0,1.0,-0.3));

// math
mat3 fromEuler(vec3 ang) {
	vec2 a1 = vec2(sin(ang.x),cos(ang.x));
    vec2 a2 = vec2(sin(ang.y),cos(ang.y));
    vec2 a3 = vec2(sin(ang.z),cos(ang.z));
    mat3 m;
    m[0] = vec3(a1.y*a3.y+a1.x*a2.x*a3.x,a1.y*a2.x*a3.x+a3.y*a1.x,-a2.y*a3.x);
	m[1] = vec3(-a2.y*a1.x,a1.y*a2.y,a2.x);
	m[2] = vec3(a3.y*a1.x*a2.x+a1.y*a3.x,a1.x*a3.x-a1.y*a3.y*a2.x,a2.y*a3.y);
	return m;
}
float saturate(float x) { return clamp(x,0.,1.); }

/*float hash(vec2 p) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}*/
float hash(vec2 p) {
    uint n = floatBitsToUint(p.x * 122.0 + p.y);
	n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return uintBitsToFloat( (n>>9U) | 0x3f800000U ) - 1.0;
}

float hash3(vec3 p) {
    return fract(sin(p.x*p.y*p.z)*347624.531834);
}

// 3d noise
float noise_3(in vec3 p) {
    vec3 i = floor( p );
    vec3 f = fract( p );	
	vec3 u = f*f*(3.0-2.0*f);
    
    float a = hash3( i + vec3(0.0,0.0,0.0) );
	float b = hash3( i + vec3(1.0,0.0,0.0) );    
    float c = hash3( i + vec3(0.0,1.0,0.0) );
	float d = hash3( i + vec3(1.0,1.0,0.0) ); 
    float v1 = mix(mix(a,b,u.x), mix(c,d,u.x), u.y);
    
    a = hash3( i + vec3(0.0,0.0,1.0) );
	b = hash3( i + vec3(1.0,0.0,1.0) );    
    c = hash3( i + vec3(0.0,1.0,1.0) );
	d = hash3( i + vec3(1.0,1.0,1.0) );
    float v2 = mix(mix(a,b,u.x), mix(c,d,u.x), u.y);
        
    return abs(mix(v1,v2,u.z));
}

// noise with analytical derivatives (thanks to iq)
vec3 noise_deriv(in vec2 p) {
    vec2 i = floor( p );
    vec2 f = fract( p );	
	vec2 u = f*f*(3.0-2.0*f);
    
    float a = hash( i + vec2(0.0,0.0) );
	float b = hash( i + vec2(1.0,0.0) );    
    float c = hash( i + vec2(0.0,1.0) );
	float d = hash( i + vec2(1.0,1.0) );    
    float h1 = mix(a,b,u.x);
    float h2 = mix(c,d,u.x);
                                  
    return vec3(abs(mix(h1,h2,u.y)),
               6.0*f*(1.0-f)*(vec2(b-a,c-a)+(a-b-c+d)*u.yx));
}

// lighting
float diffuse(vec3 n,vec3 l,float p) { return pow(max(dot(n,l),0.0),p); }
float specular(vec3 n,vec3 l,vec3 e,float s) {    
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

// terrain
vec3 octave(vec2 uv) {
    vec3 n = noise_deriv(uv);
    return vec3(pow(n.x,TERR_CHOPPY), n.y, n.z);
}

float map(vec3 p) {
    float frq = TERR_FREQ;
    float amp = 1.0;
    vec2 uv = p.xz * frq + TERR_OFFSET;
    vec2 dsum = vec2(0.0);
    
    float h = 0.0;    
    for(int i = 0; i < ITER_GEOMETRY; i++) {          
    	vec3 n = octave((uv - dsum * TERR_WARP) * frq);
        h += n.x * amp;       
        
        dsum += n.yz * (n.x*2.0-1.0) * amp;
        frq *= TERR_OCTAVE_FREQ;
        amp *= TERR_OCTAVE_AMP;        
        amp *= pow(n.x,TERR_MULTIFRACT);
    }
    h *= TERR_HEIGHT / (1.0 + dot(p.xz,p.xz) * 1e-3);
    return p.y - h;
}
float map_detailed(vec3 p) {
    float frq = TERR_FREQ;
    float amp = 1.0;
    vec2 uv = p.xz * frq + TERR_OFFSET;
    vec2 dsum = vec2(0.0);
    
    float h = 0.0;    
    for(int i = 0; i < ITER_FRAGMENT; i++) {        
    	vec3 n = octave((uv - dsum * TERR_WARP) * frq);
        h += n.x * amp;
        
        dsum += n.yz * (n.x*2.0-1.0) * amp;
        frq *= TERR_OCTAVE_FREQ;
        amp *= TERR_OCTAVE_AMP;
        amp *= pow(n.x,TERR_MULTIFRACT);
    }
    h *= TERR_HEIGHT / (1.0 + dot(p.xz,p.xz) * 1e-3);
    return p.y - h;
}
float map_noise(vec3 p) {
    p *= 0.5;    
    float ret = noise_3(p);
    ret += noise_3(p * 2.0) * 0.5;
    ret = (ret - 1.0) * 5.0;
    return saturate(ret * 0.5 + 0.5);
}

// tracing
vec3 getNormal(vec3 p, float eps) {
    vec3 n;
    n.y = map_detailed(p);    
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - n.y;
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - n.y;
    n.y = eps;
    return normalize(n);
}

float hftracing(vec3 ori, vec3 dir, out vec3 p, out float t) {
    float d = 0.0;
    t = 0.0;
    for(int i = 0; i < NUM_STEPS; i++) {
        p = ori + dir * t;
        d = map(p);
        if(d < 0.0) break;
        t += d*0.6;
    }
    return d;
}

float volume_tracing(vec3 ori, vec3 dir, float maxt) { 
    float d = 0.0;
    float t = 0.0;
    float count = 0.0;
    for(int i = 0; i < NUM_STEPS_VOLUME; i++) {
        vec3 p = ori + dir * t;
        d += map_noise(p);
        if(t >= maxt) break;  
        t += STRIDE_VOLUME;
        count += 1.0;
    }
    return d / count;
}

// color
vec3 sky_color(vec3 e) {
    e.y = max(e.y,0.0);
    vec3 ret;
    ret.x = pow(1.0-e.y,3.0);
    ret.y = pow(1.0-e.y, 1.2);
    ret.z = 0.7+(1.0-e.y)*0.3;    
    return ret;
}
vec3 terr_color(in vec3 p, in vec3 n, in vec3 eye, in vec3 dist) {
    float slope = 1.0-dot(n,vec3(0.,1.,0.));     
    vec3 ret = mix(COLOR_SNOW,COLOR_ROCK,smoothstep(0.0,0.2,slope*slope));
    ret = mix(ret,COLOR_SNOW,saturate(smoothstep(0.6,0.8,slope+(p.y-TERR_HEIGHT*0.5)*0.05)));
    return ret;
}

// main
void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	vec2 uv = fragCoord.xy / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;    
    float time = iGlobalTime * 0.1;
        
    // ray
#ifdef MOUSE
    vec3 ang = vec3(sin(time*6.0)*0.1,0.1,-time + iMouse.x*0.01);
#else
    vec3 ang = vec3(sin(time*6.0)*0.1,0.1,-time + 0.0*0.01);
#endif
	mat3 rot = fromEuler(ang);
    
    vec3 ori = vec3(0.0,5.0,40.0);
    vec3 dir = normalize(vec3(uv.xy,-2.0));
    dir.z += length(uv) * 0.12;
    dir = normalize(dir) * rot;
    ori = ori * rot;
    ori.y -= map(ori) * 0.75 - 3.0;
    
    // tracing
    vec3 p;
    float t;
    float dens = hftracing(ori,dir,p,t);
    vec3 dist = p - ori;
    vec3 n = getNormal(p, dot(dist,dist)* (1e-1 / iResolution.x));
             
    // terrain
    vec3 color = terr_color(p,n,dir,dist) * SKY_COLOR;
    color += vec3(diffuse(n,light,2.0) * SUN_COLOR);
    color += vec3(specular(n,light,dir,20.0) * SUN_COLOR*0.4);
        
    // fog
    vec3 fog = sky_color(vec3(dir.x,0.,dir.z));
    color = mix(color,fog,saturate(min(length(dist)*0.018, dot(p.xz,p.xz)*0.001)));
    
    // sky
    color = mix(sky_color(dir),color,step(dens,4.0));
    color += pow(max(dot(dir,light),0.0),3.0)*0.3;
    
    // wind
#ifdef WIND
    float wind = volume_tracing(ori,dir,t) * saturate(1.8 - p.y * 0.2);
    color = mix(color,fog, wind * 1.6);
#endif
    
    // post
    color = (1.0 - exp(-color)) * 1.5;
    color = pow(color,vec3(0.85));
	fragColor = vec4(color,1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
