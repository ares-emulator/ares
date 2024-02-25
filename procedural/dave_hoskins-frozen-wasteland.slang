#version 450
// Frozen Wasteland - 2015-03-02
// https://www.shadertoy.com/view/Xls3D2
// Frozen wasteland. Built on nimitz's model, although I didn't use his fog stuff afterall. It uses fog ray tracing included within the main ray march.

// Frozen wasteland
// https://www.shadertoy.com/view/Xls3D2
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// By Dave Hoskins

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

#define ITR 90
#define FAR 110.
#define time iGlobalTime
#define MOD3 vec3(.16532,.17369,.15787)
#define SUN_COLOUR  vec3(1., .95, .85)

#define TRIANGLE_NOISE	// .. This
//#define FOUR_D_NOISE	// ...Or this
//#define TEXTURE_NOISE		// .. Or this (faster, but not as sharp edged)
//#define VALUE_NOISE 		// .. or more normal noise.


float height(in vec2 p)
{
    float h = sin(p.x*.1+p.y*.2)+sin(p.y*.1-p.x*.2)*.5;
    h += sin(p.x*.04+p.y*.01+3.0)*4.;
    h -= sin(h*10.0)*.1;
    return h;
}

float camHeight(in vec2 p)
{
    float h = sin(p.x*.1+p.y*.2)+sin(p.y*.1-p.x*.2)*.5;
    h += sin(p.x*.04+p.y*.01+3.0)*4.;
    return h;
}

float smin( float a, float b)
{
	const float k = 2.7;
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

#define MOD2 vec2(.16632,.17369)
#define MOD3 vec3(.16532,.17369,.15787)
float tri(in float x){return abs(fract(x)-.5);}

float hash12(vec2 p)
{
	p  = fract(p * MOD2);
    p += dot(p.xy, p.yx+19.19);
    return fract(p.x * p.y);
}
float vine(vec3 p, in float c, in float h)
{
    p.y += sin(p.z*.5625+1.3)*3.5-.5;
    p.x += cos(p.z*2.)*1.;
    vec2 q = vec2(mod(p.x, c)-c/2., p.y);
    return length(q) - h*1.4 -sin(p.z*3.+sin(p.x*7.)*0.5)*0.1;
}

//========================================================================
// ################ DIFFERENT NOISE FUNCTIONS ################
#ifdef TRIANGLE_NOISE
vec3 tri3(in vec3 p){return vec3( tri(p.z+tri(p.y)), tri(p.z+tri(p.x)), tri(p.y+tri(p.x)));}
float Noise3d(in vec3 p)
{
    float z=1.4;
	float rz = 0.;
    vec3 bp = p;
	for (float i=0.; i<= 2.; i++ )
	{
        vec3 dg = tri3(bp);
        p += (dg);

        bp *= 2.;
		z *= 1.5;
		p *= 1.3;
        
        rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
        bp += 0.14;
	}
	return rz;
}
#endif

//--------------------------------------------------------------------------------
#ifdef FOUR_D_NOISE

vec4 quad(in vec4 p){return abs(fract(p.yzwx+p.wzxy)-.5);}

float Noise3d(in vec3 q)
{
    float z=1.4;
    vec4 p = vec4(q, iGlobalTime*.1);
	float rz = 0.;
    vec4 bp = p;
	for (float i=0.; i<= 2.; i++ )
	{
        vec4 dg = quad(bp);
        p += (dg);

		z *= 1.5;
		p *= 1.3;
        
        rz+= (tri(p.z+tri(p.w+tri(p.y+tri(p.x)))))/z;
        
        bp = bp.yxzw*2.0+.14;
	}
	return rz;
}
#endif

//--------------------------------------------------------------------------------
#ifdef TEXTURE_NOISE
float Noise3d(in vec3 x)
{

    x*=10.0;
    float h = 0.0;
    float a = .34;
    for (int i = 0; i < 4; i++)
    {
        vec3 p = floor(x);
        vec3 f = fract(x);
        f = f*f*(3.0-2.0*f);

        vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
        vec2 rg = textureLod( iChannel0, (uv+ 0.5)/256.0, 0.0 ).yx;
        h += mix( rg.x, rg.y, f.z )*a;
        a*=.5;
        x+=x;
    }
    return h;
}
#endif


//--------------------------------------------------------------------------------
#ifdef VALUE_NOISE
float Hash(vec3 p)
{
	p  = fract(p * MOD3);
    p += dot(p.xyz, p.yzx + 19.19);
    return fract(p.x * p.y * p.z);
}

float Noise3d(in vec3 p)
{
    vec2 add = vec2(1.0, 0.0);
	p *= 10.0;
    float h = 0.0;
    float a = .3;
    for (int n = 0; n < 4; n++)
    {
        vec3 i = floor(p);
        vec3 f = fract(p); 
        f *= f * (3.0-2.0*f);

        h += mix(
            mix(mix(Hash(i), Hash(i + add.xyy),f.x),
                mix(Hash(i + add.yxy), Hash(i + add.xxy),f.x),
                f.y),
            mix(mix(Hash(i + add.yyx), Hash(i + add.xyx),f.x),
                mix(Hash(i + add.yxx), Hash(i + add.xxx),f.x),
                f.y),
            f.z)*a;
         a*=.5;
        p += p;
    }
    return h;
}
#endif

//--------------------------------------------------------------------------------
float map(vec3 p)
{
    p.y += height(p.zx);
    float d = p.y+.5;
    
    d = smin(d, vine(p+vec3(.8,0.,0),30.,3.3) );
    d = smin(d, vine(p.zyx+vec3(0.,0,17.),33.,1.4) );
    d += Noise3d(p*.05)*(p.y*1.2);
    p.xz *=.2;
    d+= Noise3d(p*.3);
    return d;
}
float fogmap(in vec3 p, in float d)
{
    p.xz -= time*7.+sin(p.z*.3)*3.;
    p.y -= time*.5;
    return (max(Noise3d(p*.008+.1)-.1,0.0)*Noise3d(p*.1))*.3;
}

float march(in vec3 ro, in vec3 rd, out float drift, in vec2 scUV)
{
	float precis = 0.01;
    float h=precis*2.0;
    float d = hash12(scUV);
    drift = 0.0;
    for( int i=0; i<ITR; i++ )
    {
        vec3 p = ro+rd*d;
        if(h < precis || d > FAR) break;
        h = map(p);
        drift +=  fogmap(p, d);
        d += min(h*.65 + d * .002, 8.0);
	 }
    drift = min(drift, 1.0);
	return d;
}

vec3 normal( in vec3 pos, in float d )
{
	vec2 eps = vec2( d *d* .003+.01, 0.0);
	vec3 nor = vec3(
	    map(pos+eps.xyy) - map(pos-eps.xyy),
	    map(pos+eps.yxy) - map(pos-eps.yxy),
	    map(pos+eps.yyx) - map(pos-eps.yyx) );
	return normalize(nor);
}

float bnoise(in vec3 p)
{
    p.xz*=.4;
    float n = Noise3d(p*3.)*0.4;
    n += Noise3d(p*1.5)*0.2;
    return n*n*.2;
}

vec3 bump(in vec3 p, in vec3 n, in float ds)
{
    p.xz *= .4;
    //p *= 1.0;
    vec2 e = vec2(.01,0);
    float n0 = bnoise(p);
    vec3 d = vec3(bnoise(p+e.xyy)-n0, bnoise(p+e.yxy)-n0, bnoise(p+e.yyx)-n0)/e.x;
    n = normalize(n-d*10./(ds));
    return n;
}

float shadow(in vec3 ro, in vec3 rd, in float mint)
{
	float res = 1.0;
    
    float t = mint;
    for( int i=0; i<12; i++ )
    {
		float h = map(ro + rd*t);
        res = min( res, 4.*h/t );
        t += clamp( h, 0.1, 1.5 );
            }
    return clamp( res, 0., 1.0 );
}

vec3 Clouds(vec3 sky, vec3 rd)
{
    
    rd.y = max(rd.y, 0.0);
    float ele = rd.y;
    float v = (200.0)/rd.y;

    rd.y = v;
    rd.xz = rd.xz * v - time*8.0;
	rd.xz *= .0004;
    
	float f = Noise3d(rd.xzz*3.) * Noise3d(rd.zxx*1.3)*2.5;
    f = f*pow(ele, .5)*2.;
  	f = clamp(f-.15, 0.01, 1.0);

    return  mix(sky, vec3(1),f );
}


vec3 Sky(vec3 rd, vec3 ligt)
{
    rd.y = max(rd.y, 0.0);
    
    vec3 sky = mix(vec3(.1, .15, .25), vec3(.8), pow(.8-rd.y, 3.0));
    return  mix(sky, SUN_COLOUR, min(pow(max(dot(rd,ligt), 0.0), 4.5)*1.2, 1.0));
}
float Occ(vec3 p)
{
    float h = 0.0;
    h  = clamp(map(p), 0.5, 1.0);
 	return sqrt(h);   
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	vec2 p = fragCoord.xy/iResolution.xy-0.5;
    vec2 q = fragCoord.xy/iResolution.xy;
	p.x*=iResolution.x/iResolution.y;
#ifdef MOUSE
    vec2 mo = iMouse.xy / iResolution.xy-.5;
#else
    vec2 mo = 0.0 / iResolution.xy-.5;
#endif
    mo = (mo==vec2(-.5))?mo=vec2(-0.1,0.07):mo;
	mo.x *= iResolution.x/iResolution.y;
	
	vec3 ro = vec3(0.+smoothstep(0.,1.,tri(time*1.5)*.3)*1.5, smoothstep(0.,1.,tri(time*3.)*3.)*0.08, -time*3.5-130.0);
    ro.y -= camHeight(ro.zx)-.4;
    mo.x += smoothstep(0.7,1.,sin(time*.35))*.5-1.5 - smoothstep(-.7,-1.,sin(time*.35))*.5;
 
    vec3 eyedir = normalize(vec3(cos(mo.x),mo.y*2.-.05+sin(time*.5)*0.1,sin(mo.x)));
    vec3 rightdir = normalize(vec3(cos(mo.x+1.5708),0.,sin(mo.x+1.5708)));
    vec3 updir = normalize(cross(rightdir,eyedir));
	vec3 rd=normalize((p.x*rightdir+p.y*updir)*1.+eyedir);
	
    vec3 ligt = normalize( vec3(1.5, .9, -.5) );
    float fg;
	float rz = march(ro,rd, fg, fragCoord);
	vec3 sky = Sky(rd, ligt);
    
    vec3 col = sky;
   
    if ( rz < FAR )
    {
        vec3 pos = ro+rz*rd;
        vec3 nor= normal( pos, rz);
        float d = distance(pos,ro);
        nor = bump(pos,nor,d);
        float shd = (shadow(pos,ligt,.04));
        
        float dif = clamp( dot( nor, ligt ), 0.0, 1.0 );
        vec3 ref = reflect(rd,nor);
        float spe = pow(clamp( dot( ref, ligt ), 0.0, 1.0 ),5.)*2.;

        float fre = pow( clamp(1.+dot(rd, nor),0.0,1.0), 3. );
        col = vec3(.8);
	    col = col*dif*shd + fre*spe*shd*SUN_COLOUR +abs(nor.y)*vec3(.12, .13, .13);
        // Fake the red absorption of ice...
        d = Occ(pos+nor*3.);
        col *= vec3(d, d, min(d*1.4, 1.0));
        // Fog from ice storm...
        col = mix(col, sky, smoothstep(FAR-25.,FAR,rz));
        
    }
    else
    {
        col = Clouds(col, rd);
    }
    

    // Fog mix...
    col = mix(col, vec3(0.7, .7, .7), fg);
  
    // Post...
    col = mix(col, vec3(.5), -.1);
	//col = clamp(pow(col*1.,vec3(0.4)),0.0, 1.0);

	col = sqrt(col);
    
    
    // Borders...
    float f = smoothstep(0.0, 3.0, iGlobalTime)*.5;
    col *= f+f*pow(70. *q.x*q.y*(1.0-q.x)*(1.0-q.y), .2);
    
    
	fragColor = vec4( col, 1.0 );
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
