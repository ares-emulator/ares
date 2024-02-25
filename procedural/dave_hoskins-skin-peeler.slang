#version 450
// Skin peeler -  2015-08-19
// https://www.shadertoy.com/view/XtfSWX

// Skin peeler
// by Dave Hoskins
// Originally from Xyptonjtroz by nimitz (twitter: @stormoid)
// Edited by Dave Hoskins, by changing atmosphere with quite a few lighting changes & added audio

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
#define FAR 30.
#define time iGlobalTime

/*
	Believable animated volumetric dust storm in 7 samples,
	blending each layer in based on geometry distance allows to
	render it without visible seams. 3d Triangle noise is 
	used for the dust volume.

	Further explanation of the dust generation...
		
	The basic idea is to have layers of gradient shaded volumetric
	animated noise. The problem is when geometry is intersected
	before the ray reaches the far plane. A way to smoothly blend
	the low sampled noise is needed.  So I am blending (smoothstep)
	each dust layer based on current ray distance and the solid 
	interesction distance. I am also scaling the noise taps	as a 
	function of the current distance so that the distant dust doesn't
	appear too noisy and as a function of current height to get some
	"ground hugging" effect.
	
*/

#define MOD3 vec3(.16532,.17369,.15787)
mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,s,-s,c);}

float height(in vec2 p)
{
    p *= 0.2;
    return sin(p.y)*0.4 + sin(p.x)*0.4;
}

//smooth min form iq
float smin( float a, float b)
{
    const float k = 0.7;
	float h = clamp( 0.5 + 0.5*(b-a)/k, 0.0, 1.0 );
	return mix( b, a, h ) - k*h*(1.0-h);
}

///  2 out, 2 in...
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3.zxy, p3.yxz+19.19);
    return fract(vec2(p3.x * p3.y, p3.z*p3.x));
}

float vine(vec3 p, in float c, in float h)
{
    p.y += sin(p.z*.5625+1.3)*1.5-.5;
    p.x += cos(p.z*.4575)*1.;
    //p.z -=.3;
    vec2 q = vec2(mod(p.x, c)-c/2., p.y);
    return length(q) - h -sin(p.z*3.+sin(p.x*7.)*0.5+time)*0.13;
}

float map(vec3 p)
{
    p.y += height(p.zx);
    
    vec3 bp = p;
    vec2 hs = hash22(floor(p.zx/4.));
    p.zx = mod(p.zx,4.)-2.;
    
    float d = p.y+0.5;
    p.y -= hs.x*0.4-0.15;
    p.zx += hs*1.3;
    d = smin(d, length(p)-hs.x*0.4);
    
    d = smin(d, vine(bp+vec3(1.8,0.,0),15.,.8) );
    d = smin(d, vine(bp.zyx+vec3(0.,0,17.),20.,0.75) );
    
    return d*1.1;
}

float march(in vec3 ro, in vec3 rd)
{
	float precis = 0.002;
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

float tri(in float x){return abs(fract(x)-.5);}
vec3 tri3(in vec3 p){return vec3( tri(p.z+tri(p.y*1.)), tri(p.z+tri(p.x*1.)), tri(p.y+tri(p.x*1.)));}
                                 
mat2 m2 = mat2(0.970,  0.242, -0.242,  0.970);

float triNoise3d(in vec3 p)
{
    float z=1.4;
	float rz = 0.;
    vec3 bp = p;
	for (float i=0.; i<=3.; i++ )
	{
        vec3 dg = tri3(bp);
        p += (dg);

        bp *= 2.;
		z *= 1.5;
		p *= 1.2;
        //p.xz*= m2;
        
        rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
        bp += 0.14;
	}
	return rz;
}

float fogmap(in vec3 p, in float d)
{
   p.x += time;
   p.z += time*.5;
    
    return triNoise3d(p*2.2/(d+8.0))*(smoothstep(.7,.0,p.y));
}

vec3 fog(in vec3 col, in vec3 ro, in vec3 rd, in float mt)
{
    float d = .5;
    for(int i=0; i<7; i++)
    {
        vec3  pos = ro + rd*d;
        float rz = fogmap(pos, d);
        col = mix(col,vec3(.85, .65, .5),clamp(rz*smoothstep(d,d*1.8,mt),0.,1.) );
        d *= 1.8;
        if (d>mt)break;
    }
    return col;
}

vec3 normal(in vec3 p)
{  
    vec2 e = vec2(-1., 1.)*0.005;   
	return normalize(e.yxx*map(p + e.yxx) + e.xxy*map(p + e.xxy) + 
					 e.xyx*map(p + e.xyx) + e.yyy*map(p + e.yyy) );   
}

float bnoise(in vec3 p)
{
    float n = sin(triNoise3d(p*3.)*7.)*0.4;
    n += sin(triNoise3d(p*1.5)*7.)*0.2;
    return (n*n)*0.01;
}

vec3 bump(in vec3 p, in vec3 n, in float ds)
{
    vec2 e = vec2(.005,0);
    float n0 = bnoise(p);
    vec3 d = vec3(bnoise(p+e.xyy)-n0, bnoise(p+e.yxy)-n0, bnoise(p+e.yyx)-n0)/e.x;
    n = normalize(n-d*2.5/sqrt(ds));
    return n;
}

float shadow(in vec3 ro, in vec3 rd, in float mint, in float tmax)
{
	float res = 1.0;
    float t = mint;
    for( int i=0; i<20; i++ )
    {
		float h = map(ro + rd*t);
        res = min( res, 4.*h/t );
        t += clamp( h, 0.2, 1.5 );
            }
    return clamp( res, 0.0, 1.0 );

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
	
	vec3 ro = vec3(smoothstep(0.,1.,tri(time*.6)*2.)*0.1, smoothstep(0.,1.,tri(time*1.2)*2.)*0.05, -time*0.6);
    ro.y -= height(ro.zx)+0.07;
    mo.x += smoothstep(0.7,1.,sin(time*.35))-1.5 - smoothstep(-.7,-1.,sin(time*.35));
 
    vec3 eyedir = normalize(vec3(cos(mo.x),mo.y*2.-0.2+sin(time*.75*1.37)*0.15,sin(mo.x)));
    vec3 rightdir = normalize(vec3(cos(mo.x+1.5708),0.,sin(mo.x+1.5708)));
    vec3 updir = normalize(cross(rightdir,eyedir));
	vec3 rd=normalize((p.x*rightdir+p.y*updir)*1.+eyedir);
	
    vec3 ligt = normalize( vec3(.5, .5, -.2) );
    
	float rz = march(ro,rd);
	
    vec3 fogb = mix(vec3(.5, .4,.4), vec3(1.,.9,.8), min(pow(max(dot(rd,ligt), 0.0), 1.5)*1.25, 1.0));
    vec3 col = fogb;
    
    if ( rz < FAR )
    {
        vec3 pos = ro+rz*rd;
        vec3 nor= normal( pos );
        float d = distance(pos,ro);
        nor = bump(pos,nor,d);
        float shd = shadow(pos,ligt,0.1,3.);
        float dif = clamp( dot( nor, ligt ), 0.0, 1.0 )*shd;
        float spe = pow(clamp( dot( reflect(rd,nor), ligt ), 0.0, 1.0 ),3.)*shd;
        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 1.5 );
        vec3 brdf = dif*vec3(1.00)+abs(nor.y)*.4;
        col = clamp(mix(vec3(.7,0.4,.3),vec3(.3, 0.1, 0.1),(pos.y+.5)*.25), .0, 1.0);
        col *= (sin(bnoise(pos*.1)*250.)*0.5+0.5);
        col = col*brdf + spe*fre;// + fre*vec3(.4,.4,0.4)*.5*crv;
    }
    
    //ordinary distance fog first
    col = mix(col, fogb, smoothstep(FAR-10.,FAR,rz));
    
    //then volumetric fog
    col = fog(col, ro, rd, rz);
    
    //post...

	col = pow(col,vec3(0.8));
    col = smoothstep(0.0, 1.0, col);
    
    col *= .5+.5*pow(70. *q.x*q.y*(1.0-q.x)*(1.0-q.y), .2);
    
    
	fragColor = vec4( col  * smoothstep(0.0, 3.0, time), 1.0 );
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
