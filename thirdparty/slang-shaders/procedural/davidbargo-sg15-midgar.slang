#version 450
// Fully procedural Midgar city from Final Fantasy VII
// Created by David Bargo - davidbargo/2015
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

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

#define ITR 100
#define FAR 150.
#define PI 3.141592
#define precis 0.001
#define time iGlobalTime

mat3 rot_x(float a)
{
    float sa = sin(a); 
    float ca = cos(a); 
    return mat3(1.,0.,0., 0.,ca,sa, 0.,-sa,ca);
}

mat3 rot_z(float a)
{
    float sa = sin(a); 
    float ca = cos(a); 
    return mat3(ca,sa,0., -sa,ca,0., 0.,0.,1.);
}

//----------------------------------------------------------------------------------------
// From Dave_Hoskins
vec2 hash22(vec2 p){
    p  = fract(p * vec2(5.3983, 5.4427));
    p += dot(p.yx, p.xy +  vec2(21.5351, 14.3137));
    return fract(vec2(p.x * p.y * 95.4337, p.x * p.y * 97.597));
}

vec3 hash33(vec3 p){
    p  = fract(p * vec3(5.3983, 5.4427, 6.9371));
    p += dot(p.yzx, p.xyz  + vec3(21.5351, 14.3137, 15.3219));
    return fract(vec3(p.x * p.z * 95.4337, p.x * p.y * 97.597, p.y * p.z * 93.8365));
}

float hash12(vec2 p)
{
    p  = fract(p * vec2(443.8975,397.2973));
    p += dot(p.xy, p.yx+19.19);
    return fract(p.x * p.y);
}
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
// Distance functions by iq
float ssphere( vec3 p, float s )
{
  return length(p)-s;
}

float sbox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float ubox( vec3 p, vec3 b )
{
  return length(max(abs(p)-b,0.0));
}

float urbox( vec3 p, vec3 b, float r )
{
  return length(max(abs(p)-b,0.0))-r;
}

float cyl( vec3 p, vec2 h )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - h;
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

float caps( vec3 p, vec3 a, vec3 b, float r )
{
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float torus( vec3 p, vec2 t )
{
  return length( vec2(length(p.xz)-t.x,p.y) )-t.y;
}

float length8(vec2 p)
{
    p *= p;
    p *= p;
    return pow(dot(p, p), 0.125);
}

float tube( vec3 p, vec2 t )
{
  return length(vec2(length8(p.xz)-t.x,p.y))-t.y;
}

//----------------------------------------------------------------------------------------
// From iq
mat3 m = mat3( 0.00,  0.80,  0.60,
              -0.80,  0.36, -0.48,
              -0.60, -0.48,  0.64 );

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);

    float n = p.x + p.y*57.0 + 113.0*p.z;

    float res = mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                        mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
                    mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                        mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
    return res;
}
//----------------------------------------------------------------------------------------

float fbm(vec3 p) 
{
    return 0.65*noise(p) + 0.35*noise(m*p*2.02);
}


float matid = 0.;
float light = 0.;

float map(vec3 p)
{    
    // center bottom
    float mn2 = cyl(p+vec3(p.x*0.35, 0.0, 0.2-0.1*p.y), vec2(0.64,0.98));
    matid = 1.;
    
    // center middle
    float mn = cyl(p+vec3(p.x*0.5,-1.,0.2-0.1*p.y), vec2(0.4,0.45));
    matid += step(mn, mn2);
    mn2 = min(mn, mn2);
    
    // side towers
    float signx = sign(-p.x);
    float stw = mix(0.2, 0.24, smoothstep(0.65, 0.7, p.y));
    stw = mix(stw, 0.2, smoothstep(0.9, 0.95, p.y));
    stw = mix(stw, 0.24, smoothstep(1.15, 1.22, p.y));
    mn = caps(p + vec3( signx*0.75, 2.,0.3), vec3(0.,0.0,0.), vec3(0.0,3.2,0.), stw);   
    matid = mix(matid, 3., step(mn, mn2));
    mn2 = min(mn, mn2);
    
    // top main
    mn = urbox(p+vec3(0.,-2.64,-0.09), vec3(0.06, 0.39, 0.2), 0.1);
    
    // top support
    mn = min(mn, urbox(p+ vec3(0.12*signx, -1.8,-0.21 - 0.2*smoothstep(2.2, 1.8,p.y)), vec3(-0.01, 1., 0.06), 0.05));
    matid = mix(matid, 4., step(mn, mn2));
    mn2 = min(mn, mn2);
    
    // polar coords
    vec3 pc = vec3(length(p.xz)*0.5, p.y*0.5, atan(p.z,p.x) - PI/16.);
    
    // mini towers
    vec3 ps = pc;
    float mtl = step(-3.22, pc.z)*step(pc.z, -2.67);
    ps.z = (0.5+0.5*sin((ps.z)*48. - 1.))*0.15;
    ps.x += -.87;
    ps.x*=2.;
    mn = caps(ps, vec3(0.), mtl*vec3(0.0,0.23,0.0), 0.07*mtl);
    ps.x += -.17;
    mn = min(mn, caps(ps, vec3(0.), mtl*vec3(0.0,0.17,0.0), 0.07*mtl));
    matid = mix(matid, 5., mtl*step(mn, mn2));
    mn2 = min(mn, mn2);
    
    // small wall lights
    ps = pc;
    ps.z = (0.5+0.5*sin((ps.z)*40. - 2.3))*0.06;
    ps.y -= 0.14;
    float wlc = smoothstep(0.78, 0.68, ps.x);
    ps.x += -0.69;
    float ss = 0.06*smoothstep(0.06, 0.03, ps.x) - 0.025*wlc;
    float sls = step(-1.65, pc.z)*step(pc.z, -1.46) + step(-2.45, pc.z)*step(pc.z, -1.95);
    mn = ssphere(ps, ss*sls);
    
    // big wall lights
    ps = pc;
    ps.z = (0.5+0.5*sin((ps.z)*48. + 1.))*0.12;
    ps.y -= 0.27;
    float wl = smoothstep(2.01, 1.9, ps.x);
    ps.x += -2. + 0.08*wl;
    float sb = 0.1*smoothstep(0.08, 0.03, ps.x) - 0.025*wl;
    float slb = step(2.215, abs(pc.z))*step(abs(pc.z), 2.495) + step(-0.14, pc.z)*step(pc.z, 0.15);
    mn = min(mn, ssphere(ps, sb*slb) + FAR*(1. - slb));
    
    matid = mix(matid, 6., step(mn, mn2));
    mn2 = min(mn, mn2);
    
    // platforms
    vec3 pl = pc;
    float plr = step(abs(pl.z + 0.6), 0.33);
    pl.z = 0.5+0.5*sin((pl.z)*8. + PI);
    
    vec3 hashp = hash33(floor(p*7.)/7.);
    vec2 polarhash = hash22(floor(pc.zz*8.)/8. + 0.75)*0.6;
    float bra = smoothstep(0., 1.2, pc.x);
    bra *= bra; bra *= bra; bra *= bra;
    float br = step(0.15*bra*bra, abs(pc.z + 0.55 + 0.1*hashp.z));
    float bdgs = hash12(floor(pc.xz*32.)/32.);
    bdgs *= step(bdgs, 0.5)*0.04;
    mn = sbox(pl + vec3(-2.8+mix(polarhash.x, 1.5, plr),0.0,-0.3),
              br*vec3(2.15 - mix(polarhash.x, 1.5, plr), 0.05 + step(pc.x, 1.9)*bdgs, 0.6));
    matid = mix(matid, 7., step(mn, mn2));
    mn2 = min(mn, mn2);
    
    // wall columns
    ps = pc;
    ps.z = 0.5+0.5*sin((ps.z)*8.);
    float w = smoothstep(0.57, 0., pc.y);
    ps.x += -2.;
    mn = max(urbox(ps, vec3(0.075+0.025*w,0.38,-0.06 + 0.05*w), 0.1), -urbox(ps, vec3(0.07, 1.,-0.048), 0.05));
	matid = mix(matid, 8., step(mn, mn2));
    mn2 = min(mn, mn2);
    
    // back main building
    mn = sbox(p+vec3(0.,0.0,-0.25+0.1*p.y), vec3(0.472, 0.98, 0.34));
    mn = min(mn, sbox(p+vec3(0.,-.81,-0.4+0.1*p.y), vec3(0.262, 0.64, 0.36)));
    
    // building center tubes
    mn = min(mn, cyl(p+ vec3(0.34*signx, -1.1+0.6*abs(p.x), 0.3), vec2(0.07, 0.5)));
    mn = min(mn, cyl(p+ vec3(0.35*signx, -1.05+0.6*abs(p.x), 0.15), vec2(0.07, 0.5)));
    mn = min(mn, cyl(p+ vec3(0.36*signx, -1.+0.6*abs(p.x), 0.), vec2(0.07, 0.5)));
    
    // building center cylinder
    float sinpx = sin(p.x*100.);
    float tw = mix(0.2, 0.25 + 0.005*sinpx, smoothstep( -0.4, -0.35, p.x));
    tw =       mix( tw, 0.3,                smoothstep(-0.25, -0.15, p.x));
    tw =       mix( tw, 0.3  + 0.005*sinpx, smoothstep( 0.15,   0.3, p.x));
    tw =       mix( tw, 0.2,                smoothstep(  0.3,  0.42, p.x));
    
    mn = min(mn, cyl(p.yxz+vec3(-1.8, 0., 0.1), vec2(tw, 0.45)));
    
    mn = min(mn, torus((p.xzy+vec3(-0.04, 0.1, -1.8))*rot_z(1.2), vec2(0.32, 0.044)));
    
    // building back
    float ba = smoothstep(0.925, 0., p.y);
    mn = min(mn, max( urbox(p+ vec3(0.2*signx, -0.9,-1.2 + ba*1.5),
                             vec3(0.1-p.y*0.08, 0.45, 0.2 + ba*0.88), 0.1),
                     -urbox(p+vec3(0.2*signx, -1.2, -1.2), vec3(0.02, 0.3, 0.15), 0.01))); // bottom back
    mn = min(mn, cyl(p.yxz+vec3(-0.6, 0.28*signx, -1.), vec2(0.12,0.2))); // back tube
    
    vec3 pos = (p+ vec3(0.4*signx, -1.1,-0.58))*rot_x(-PI*0.22);
    mn = min(mn, urbox(pos, vec3(-0.01, 0.66, 0.04 + 0.16*smoothstep(-0.85, 0.85, pos.y)), 0.05));
       
    pos = (p+vec3(0.,-1.95,-0.4))*rot_x(PI*0.15);
    pos.z -= 0.2*smoothstep(0.5,0.0,pos.y);
    mn = min(mn, urbox(pos, vec3(0.01, 0.76, 0.1 + 0.12*smoothstep(0.5,0.0, pos.y)), 0.1)); // top back
    
    mn = min(mn, urbox(p+vec3(0.,-2.94,-0.2), vec3(0.01, 0.4, 0.01), 0.001)); // antenna
    
    mn = min(mn, tube(p+vec3(-signx*0.25,-2.34 - 0.3*p.z*step(0., p.z),-0.2), vec2(0.5, 0.05)));
    
    // center tubes
    mn = min(mn, tube(p.zxy+vec3(1.43 ,0., -0.1), vec2(0.4, 0.1))); //center
    mn = min(mn, cyl(p+vec3(0.,0.,1.85), vec2(0.46, 0.22)));
    mn = min(mn, cyl(p+vec3(0.,0.,1.85), vec2(0.24, 0.3)));
    
    mn = min(mn, tube((p.zxy+vec3(0.25 ,1.2, -0.1))*rot_z(PI/6.), vec2(0.4, 0.14))); // right 1  
    mn = min(mn, tube((p.xzy+vec3(0.7 , -0.3, -0.1))*rot_z(-PI/8.), vec2(0.8, 0.1))); // right 2   
    mn = min(mn, tube((p.xzy+vec3(0.5, -0.5, -0.4))*rot_z(-PI/4.), vec2(0.8, 0.1))); // right 3   
    mn = min(mn, tube((p.xzy+vec3(-1., 1., -0.4))*rot_z(-PI/4.), vec2(0.4, 0.1))); // left 1
    mn = min(mn, tube(p.xzy+vec3(-1.3, 0., -0.2), vec2(0.4, 0.08))); // left 2
    
    // back towers 
    mn = min(mn,  cyl(p + vec3(0.76, 2., -0.3),   vec2(0.2-smoothstep(1.6, 2.1, p.y)*0.14, 4.0)));  
    mn = min(mn,  cyl(p + vec3(0.64, 2., -0.64), vec2(0.2-smoothstep(1.8, 2.3, p.y)*0.14, 4.2)));
    
    // small platforms
    float cp = step(-2., pc.z)*0.06;
    mn = min(mn, ubox(pl + vec3(-0.83+cp,0.0,0.), vec3(0.17-cp,0.09*(step(pc.z, -0.1)+step(2.1, pc.z)),0.85)));
    
    // sectors walls
    ps = pc;
    ps.z = 0.5+0.5*sin((ps.z)*8. +0.7);
    ps.x += -1.325;
    mn = min(mn, sbox(ps, vec3(0.6,0.065+pc.x*0.052,0.02)));

    ps.z = 0.5+0.5*sin((pc.z)*8. -0.7);
    mn = min(mn, sbox(ps, vec3(0.6,0.065+pc.x*0.052,0.02)));
    
	// first wall
    ps = pc;
    ps.z = 0.;
    ps.y += 1.;
    ps.x += -0.65;  
    mn = min(mn, urbox(ps, vec3(0.0,1.13,2.), 0.05));
    
    // second wall
    ps = pc;
    ps.z = 0.5+0.5*sin((ps.z)*72.);
    ps.x += -2.;   
    mn = min(mn, sbox(ps, br*vec3(0.1, min(0.25, 0.24 - ps.x*1.1), 0.993+2.*step(ps.y, 0.13))));
    
    // presecond wall
    mn = min(mn, sbox(pl + vec3(-1.9,-0.06,-0.35), br*vec3(0.1, 0.005+pc.x*0.052+step(1.86, pc.x)*0.005, 0.6)));

    // sectors connector tube
    ps.x += 0.7;
    ps.y += -0.02;
    mn = min(mn, sbox(ps, br*vec3(0.03,0.03,2.)));
    
    matid = mix(matid, 0., step(mn, mn2));
    return min(mn, mn2);
}

float march(in vec3 ro, in vec3 rd)
{
    float h=precis*2.0;
    float d = 0.;
    for( int i=0; i<ITR; i++ )
    {
        if( abs(h)<precis || d>FAR ) break;
        d += h;
        h = map(ro+rd*d);
    }
        
    return d;
}

float smalllights(vec3 pc)
{
    vec3 ps = pc;
    ps.z = (0.5+0.5*sin((ps.z)*40. - 2.3))*0.06;
    ps.y -= 0.14;
    float wlc = smoothstep(0.78, 0.68, ps.x);
    ps.x += -0.69;
    ps.x = min(ps.x-0.05, 0.);
    float sc = step(-1.65, pc.z)*step(pc.z, -1.46) + step(-2.45, pc.z)*step(pc.z, -1.95);
    return ssphere(ps, 0.02*(2.*sc -1.));
}

float biglights(vec3 pc)
{
    vec3 ps = pc;
    ps.z = (0.5+0.5*sin((ps.z)*48. + 1.))*0.1;
    ps.y -= 0.27;
    float wl = smoothstep(1.62, 1.52, ps.x);
    ps.x += -1.95 + 0.08*wl;
    ps.x = min(ps.x-0.005, 0.);
    
    float sl = step(2.215, abs(pc.z))*step(abs(pc.z), 2.495) + step(-0.14, pc.z)*step(pc.z, 0.15);
    return ssphere(ps, 0.04*(2.*sl -1.)+.01*(pc.x-1.95));
}

float lights(vec3 p)
{
    vec3 pc = vec3(length(p.xz)*0.5, p.y*0.5, atan(p.z,p.x) - PI/16.);
    float sl = smalllights(pc);
    
    float bl = biglights(pc);
    light = step(bl, sl);
    
    return min(bl, sl);
}

vec3 marchlights(in vec3 ro, in vec3 rd, float far)
{
    light = 0.;
    
    float h=precis*2.0;
    float d = 0.;
    for( int i=0; i<60; i++ )
    {
        if( abs(h)<precis || d>far ) break;
        d += h;
        h = lights(ro+rd*d);
    }
    
    if (d < far)
    {
        vec3 lpos = (ro+rd*d);
        float dist = length(lpos.xz);
        
        if (light == 0.)
        {
            float ld = precis*2.;
            
            for( int i=0; i<30; i++ )
            {
                if( h > precis*2.) break;
                vec3 lposb = (lpos+rd*ld);
                vec3 pc = vec3(length(lposb.xz)*0.5, lposb.y*0.5, atan(lposb.z,lposb.x) - PI/16.);
                h = smalllights(pc);
                ld += max(abs(h), precis*2.);
            }
			
            light = smoothstep(2.2, 0., dist)*ld*ld*10.;
        }
        else if (light == 1.)
        {
            float ld = precis*2.;
            
            for( int i=0; i<40; i++ )
            {
                if( h > precis*2.) break;
                vec3 lposb = (lpos+rd*ld);
                vec3 pc = vec3(length(lposb.xz)*0.5, lposb.y*0.5, atan(lposb.z,lposb.x) - PI/16.);
                h = biglights(pc);
                ld += max(abs(h), precis*2.);
            }
			
            light = smoothstep(6.7, 0., dist)*ld*ld*1.5;
        }
    }

    return vec3(1.,1.,0.7)*fract(light);   
}

float makolight(vec3 p)
{
    p.y *= 0.3;
    return ssphere(p, 1.15);
}

vec3 makocolor(vec3 p)
{
    float n = noise(p.xzz);
        
    vec3 pc = vec3(length(p.xz)*0.5, p.y, atan(p.z,p.x));
	
    float hl = mix(0.4, 1., smoothstep(-2.8, -2.7, pc.z));
    hl = mix(hl, 0.4, smoothstep(-1.8, -1.65, pc.z));
    hl = mix(hl, 1., smoothstep(-1.5, -1.4, pc.z));
    hl = mix(hl, 0.6, smoothstep(-1., -0.9, pc.z));
    hl = mix(hl, 1., smoothstep(-0.7, -0.6, pc.z));
    hl = mix(hl, 0.5, smoothstep(-0.2, -0.05, pc.z));
    hl = mix(hl, 1., smoothstep(0.05, 0.2, pc.z));
    hl = mix(hl, 0.4, smoothstep(1., 1.2, pc.z));
    hl = mix(hl, 0.9, smoothstep(1.9, 2.2, pc.z));
    hl = mix(hl, 0.4, smoothstep(2.8, 3., pc.z));
    
    float intensity = 0.25*smoothstep((0.7+0.8*n)*hl, 0., p.y)*step(pc.x, 0.6);
    

    float mp = intensity*fbm(vec3(30.*p.x,1.5*p.y-2.*time,p.z*30.));
    return mp*mix(vec3(0.0, 0.4, 0.8), 
              mix(vec3(0.2, 0.9, 0.6), 
                  vec3(0.5, 0.5, 0.0), smoothstep(0.3 + 0.3*n, 0.6 + 0.6*n, p.y)), smoothstep(-0.5, 0.3 + 0.3*n, p.y));

}

vec3 marchmako(in vec3 ro, in vec3 rd, float far)
{
    vec3 color = vec3(0.);
    float h=precis*2.0;
    float d = h;
    
    for( int i=0; i<50; i++ )
    {
        if( abs(h)<precis || d>far ) break;
        d += h;
        h = makolight(ro+rd*d);
    }
    
    if (d < far)
    {
        color = makocolor(ro+rd*d);
            
        // back face
        d += h;
        for(int i = 0; i < 50; i++)
        {
            if(h > precis*2. || d > far) break;           
            h = makolight(ro+rd*d);
            d += max(abs(h), precis*2.);
        }
        if (d < far) 
        {
            color += makocolor(ro+rd*d);
        }
    }
        
    return color;
}

float tex(in vec3 q)
{
    vec3 h = hash33(floor(q*4.)/4.);
    return (h.x+h.y+h.z)*0.1;
}

float mapN(in vec3 p)
{
    float d= map(p);
    d += tex(p*10.)*0.005;
    return d;
}

vec3 normal(const in vec3 p)
{  
    vec2 e = vec2(-1., 1.)*0.008;
	return normalize(e.yxx*mapN(p + e.yxx) + e.xxy*mapN(p + e.xxy) + 
					 e.xyx*mapN(p + e.xyx) + e.yyy*mapN(p + e.yyy) );   
}

//from iq
float getAO( in vec3 pos, in vec3 nor )
{
    float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.13*float(i)/3.;
        vec3 aopos =  nor * hr + pos;
        float dd = map( aopos );
        occ += -(dd-hr)*sca;
        sca *= 0.95;
    }
    return clamp( 1. - 3.5*occ, 0.0, 1.0 );    
}

vec3 shinralogo(vec2 pos, vec2 center, float size)
{
    vec2 absp = vec2(abs(pos.x - center.x), abs(pos.y - center.y)*1.3);
    float clogo = step(size, absp.y + absp.x);
    float blogo = step(absp.y, size*0.8)*step(absp.x, size*0.8);
    vec3 col = vec3(step(absp.y, size)*step(absp.x, size));
    col.r -= 0.8*clogo*blogo;
    float gb = 1. - (1.-blogo)*clogo;
    col.gb -= gb*step(2.*(pos.y - center.y) - absp.x*4., 0.4*size);
    return clamp(col, 0., 1.);
}

float tri(in float x)
{
    return abs(fract(x)-.5);
}

// From nimitz
vec3 stars(in vec3 p)
{
    vec3 c = vec3(0.);
    
    //Triangular deformation (used to break sphere intersection pattterns)
    p.x += (tri(p.z*50.)+tri(p.y*50.))*0.006;
    p.y += (tri(p.z*50.)+tri(p.x*50.))*0.006;
    p.z += (tri(p.x*50.)+tri(p.y*50.))*0.006;
    
	for (float i=0.;i<2.;i++)
    {
        vec3 q = fract(p*250.)-0.5;
        vec3 id = floor(p*250.);
        float rn = hash33(id).z;
        float c2 = 1.-smoothstep(-0.2,.4,length(q));
        c2 *= step(rn,0.005+i*0.014);
        c += c2*(mix(vec3(1.0,0.75,0.5),vec3(0.85,0.9,1.),rn*30.)*0.5 + 0.5);
        p *= 1.15;
    }
    return c*c*smoothstep(-0.1, 0., p.y);
}

float platformTex(vec2 p)
{
	float hp = hash12(floor(p));
    vec2 fp = fract(p);
    
    float s = sign(fp.x - hp);   
    float ha = hash12(floor(p-vec2(s, 0.0)));
    float hb = hash12(floor(p+vec2(0.0, s))) + s;
    float hc = hash12(floor(p+vec2(s, s))) + s;
    
    vec2 p1 = vec2(hp, ha);
    vec2 p2 = vec2(hb, hc);

    vec2 sp = sign(p1 - p2);
    vec2 sp1 = p1*sp;
	vec2 sp2 = -p2*sp;
    vec2 f = max(abs(fp - (p1+p2)*0.5) - (sp1+sp2)*0.5 + 0.033, 0.0);
    return clamp(1.0 - dot(f, f)*1000., 0.0, 1.0);
}

vec3 cam()
{
    float t = time*0.25 - 0.5;
    vec3 pos = vec3(-6.5, 7.3, -11.5);
    pos = mix(pos, vec3(-6.5, 1.2, -3.2), smoothstep(1., 10., time));
    pos = mix(pos, vec3(0., 3.5, -8.5), smoothstep(8., 18., time)); 
    pos = mix(pos, vec3(9.5, 7.2, -2.5), smoothstep(16., 26., time));    
    pos = mix(pos, vec3(9.5, 6.2, -2.5), smoothstep(25., 27., time));    
    pos = mix(pos, vec3(4.5, 1., 4.5), smoothstep(29., 36., time));    
    pos = mix(pos, vec3(-1., 1., 4.5), smoothstep(34., 42., time));
    pos = mix(pos, vec3(13.*sin(t),8.+2.5*sin(t+0.1),-13.*cos(t)), smoothstep(40., 50., time));
    
    return pos;
}

vec3 target()
{
    vec3 ta = vec3(0.0,0.7,0.0);
    ta = mix(ta, vec3(0.0,1.9,0.0), smoothstep(1., 10., time));
    ta = mix(ta, vec3(0.0,1.6,0.0), smoothstep(9., 18., time));    
    ta = mix(ta, vec3(0.0,1.,0.0), smoothstep(16., 26., time));    
    ta = mix(ta, vec3(0.0,-1.,0.0), smoothstep(24., 27., time));   
    ta = mix(ta, vec3(0.0,-3.,5.0), smoothstep(26.5, 30., time));  
    ta = mix(ta, vec3(0.0,1.,4.0), smoothstep(30.5, 36., time));   
    ta = mix(ta, vec3(-1.0,1.,0.0), smoothstep(34., 42., time));
    ta = mix(ta, vec3(0.0,0.7,0.0), smoothstep(40., 50., time));
    
    return ta;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{   
    vec2 p = fragCoord.xy/iResolution.xy-0.5;
    p.x *= iResolution.x/iResolution.y;
    
    vec3 ro = cam();
    vec3 ta = target();
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(0.0,1.0,0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
    vec3 rd = normalize( p.x*uu + p.y*vv + 2.0*ww );
    
    float d = march(ro,rd);
    
    vec3 col = stars(rd);
     
    if ( d < FAR )
    {
        vec3 pos = ro+d*rd;
        float mid = matid;
        vec3 nor = normal(pos);
        
        
        vec3 pc = vec3(length(pos.xz), pos.y, atan(pos.z,pos.x) - PI/16.);
        vec3 pl = pc;
        pl.z = 0.5+0.5*sin(pl.z*8. + PI);
          
        vec3 lpos = vec3(0., 0.38, 0.);
        lpos.xz = 1.25*normalize(pos.xz);
        vec3 lt = (lpos - pos);
        vec3 lgt = 0.15*lt;
        
        vec3 lgtp = (ta + 0.5*normalize(pos - ta) - pos)*0.5;
        
        float dif = clamp( dot( nor, lgt ), 0.0, 1.0 ) + 0.4*clamp( dot( nor, lgtp ), 0.0, 1.0 );
        float spe = pow(clamp( dot( reflect(rd,nor), lgt ), 0.0, 1.0 ),7.);
        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 3.0 );
        vec3 brdf = vec3(1.)*2.*dif;
        
        col = vec3(0.04,0.04,0.02);
        
        float ao = getAO(pos,nor);

        if (mid == 1.)
        {
            col = col*brdf +.03*fre;
            col *= ao;
            
            float l = 0.5 + 0.5*sin(pos.y*98.);
            l *= l*step(pos.y, 0.95);
            float h = hash12(floor(pos.xy*31.)/31.);
            h = 0.9*step(0.7, h);
            col += vec3(1. - h*0.7, 1. - h,0.)*l*l*0.1;
        }
        else if (mid == 2.)
        {
            col += shinralogo(pos.xy, vec2(0., 1.22), 0.12)*0.5;
            col = col*brdf +.03*fre;
            col *= ao;
            
            float l = 0.5 + 0.5*sin(pos.y*98.);
            
            l *= l*step(abs(pos.y - 1.19), 0.19)*step(0.15, abs(pos.x));
            float h = hash12(floor(pos.xy*31.)/31.);
            h = 0.9*step(0.8, h);
            col += vec3(1. - h*0.7, 1. - h,0.)*l*l*0.1;
        }
        else if (mid == 3.)
        {
            col = col*brdf +.03*fre;
            col *= ao;
            
            float l = 0.5 + 0.5*sin(pos.y*98.);
            l *= l*step(pos.y, 1.15)*(step(pos.y, 0.65) + step(0.95, pos.y));
            float h = hash12(floor(pos.xy*31.)/31.);
            h = 0.9*step(0.8, h);
            col += vec3(1. - h*0.7, 1. - h,0.)*l*l*0.1;
        }
        else if (mid == 4.)
        {
            col += shinralogo(pos.xy, vec2(0., 2.5), 0.06)*0.5;
            
            col = col*(0.05) +.03*fre;
            col *= ao;
            
            float l = 0.5 + 0.5*sin(pos.y*98.);
            l *= l*step(pos.z, -0.16*step(pos.y, 2.88))*step(pos.y, 3.05)*step(2.6, pos.y);
            float h = hash12(floor(pos.xy*23.)/23.);
            h = 0.9*step(0.5, h);
            col += vec3(1. - h*0.7, 1. - h,0.)*l*l*0.1;
        }    
        else if (mid == 5.)
        {
            col = col*brdf +.03*fre;
            col *= ao;
            
            float d = mix(step(abs(pos.y - 0.49), 0.01), step(abs(pos.y - 0.38), 0.01), step(1.82, pl.x));
            
            col += vec3(1., 1.,0.)*d*0.1;
        }       
        else if (mid == 6.)
        {
            col = col*brdf +.03*fre;
            col *= ao;
            
            col += 0.5*mix(smoothstep(1.448, 1.458, pl.x), smoothstep(4.08, 4.11, pl.x), step(2.5, pl.x));
            
        }        
        else if (mid == 7.)
        {
            float f = 0.;
            vec3 l = vec3(0.);
            
            if (pl.x < 3.6)
            {
                float br = step(0.4, abs(pc.z + 0.46));
            	col += vec3(0.5, 0.35, 0.)*noise(pos*5.5)*(0.3 + 0.7*br);
                l = vec3(0.5, 0.3, 0.)*step(0.993, hash12(floor(pos.xz*50.)/50.))*step(pl.x, 3.4)*br;
            }
            else
            {
				vec2 pt = vec2(pc.x, pc.z*7.);
                col += platformTex(pt)*0.04;
                float el = step(4.18, pl.x)*smoothstep(0.51, 0.42, pl.z*10.0/pl.x)*smoothstep(-0.2, -0.18, -pl.y)*smoothstep(5.7, 4.3, pl.x);       
            	l = vec3(1.,1.,0.7)*(0.02*el*smoothstep(0., 0.2, col.r));
            }
 
            col = col*0.05 + 0.03*fre;
            col *= ao;          
            col += l;
        }       
        else if (mid == 8.)
        {
            col += shinralogo(pl.xy, vec2(4.02, 0.6), .16)*0.5;
            
            float rl = hash12(floor((pos.xz )*0.25 + 1.5)/1.);
            float st = 0.5 + 0.5*sin(time*(2.+rl) + rl*10.); 
            col += vec3(0.0, 0.75, 0.3)*smoothstep(0.85, 1., pos.y);
            col = col*0.05 +.03*fre;
            col *= ao*(4. - 3.*mix(1., smoothstep(0.95, 0.75, pos.y), st));
            col += (0.25 + 0.75*st)*vec3(0.1, 1., 0.7)*smoothstep(0.95, 0.75, pos.y)*step(0.998, pl.z)*step(abs(pl.x - 4.), 0.16);
        }       
        else 
        {
            col = col*brdf +.03*fre;
            col *= ao;          
        }
          
        // fake platform lights
        float el = step(4.18, pl.x)*smoothstep(0.51, 0.42, pl.z*10.0/pl.x)*smoothstep(-0.2, -0.18, -pl.y)*smoothstep(5.7, 4.3, pl.x);       
        col += vec3(1.,1.,0.7)*(0.6*el*smoothstep(4.22, 4.2, pl.x));
                
        col *= smoothstep(90.0, 0.0, dot(pos, pos));
        col *= smoothstep(-0.6, -0.1, pos.y);
    }

    col += marchlights(ro, rd, d);
    col += marchmako(ro, rd, d);
    col = pow(clamp(col*1.5,0.,1.), vec3(0.416667))*1.055 - 0.055; //sRGB
        
    fragColor = vec4( col, 1.0 );
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
