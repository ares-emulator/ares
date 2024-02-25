#version 450
// PBR_Test -  2v_S - 2015-05-24
// https://www.shadertoy.com/view/MIB3DV

// Physically Based Rendering Tes

/*
* References : 
*
* http://renderwonk.com/publications/s2010-shading-course/hoffman/s2010_physically_based_shading_hoffman_b_notes.pdf
* 
* http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
*
* http://graphicrants.blogspot.fr/2013/08/specular-brdf-reference.html
*
* http://www.filmicworlds.com/2014/04/21/optimizing-ggx-shaders-with-dotlh/
*
* http://blog.selfshadow.com/publications/s2013-shading-course/#course_content
*
*	Ray marching code from iq
*/

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

#define NB_LIGHTS 3

// Metals values in linear space
#define GOLD vec3(1.0, 0.71, 0.29)
#define COPPER vec3(0.95, 0.64, 0.54)
#define IRON vec3(0.56, 0.57, 0.58)
#define ALUMINIUM vec3(0.91, 0.92, 0.92)
#define SILVER vec3(0.95, 0.93, 0.88)



float fPlane( vec3 p, vec4 n )
{
  // n must be normalized
  return dot(p,n.xyz) + n.w;
}

float fSphere( vec3 p, float s )
{
    return length(p)-s;
}
float opS( float d1, float d2 )
{
    return max(-d2,d1);
}

vec2 opU( vec2 d1, vec2 d2 )
{
	return (d1.x<d2.x) ? d1 : d2;
}

vec3 opRep( vec3 p, vec3 c )
{
    return mod(p,c)-0.5*c;
}
// ---- Scene definition
vec2 fScene(vec3 p) {
    
    vec3 pSphere = p/*opRep(p, vec3( 2.0, 0.0, 2.0))*/;
    
 	vec2 sphere0 = vec2(fSphere(p, 1.0), 0.5); 	
    vec2 sphere1 = vec2(fSphere(p+vec3(2.1, 0.0, 2.0), 1.0), 2.5);
	vec2 sphere2 = vec2(fSphere(p+vec3(-2.1, 0.0, 2.0), 1.0), 3.5);
    vec2 sphere3 = vec2(fSphere(p+vec3(2.1, 0.0, -2.0), 1.0), 4.5);
    vec2 sphere4 = vec2(fSphere(p+vec3(-2.1, 0.0, -2.0), 1.0), 5.5);
	


    vec2 plane = vec2(fPlane(p, vec4(0, 1, 0, 1.0)), 1.5);
    
    return opU(opU(opU(opU(opU(plane, sphere0), sphere1), sphere2), sphere3), sphere4);
}
// -----

vec2 castRay( in vec3 ro, in vec3 rd )
{
    float tmin = 1.0;
    float tmax = 100.0;
    
    
	float precis = 0.00001;
    float t = tmin;
    float m = -1.0;
    for( int i=0; i<50; i++ )
    {
	    vec2 res = fScene( ro+rd*t );
        if( res.x<precis || t>tmax ) break;
        t += res.x;
	    m = res.y;
    }

    if( t>tmax ) m=-1.0;
    return vec2( t, m );
}


float softshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax )
{
	float res = 1.0;
    float t = mint;
    for( int i=0; i<16; i++ )
    {
		float h = fScene( ro + rd*t ).x;
        res = min( res, 8.0*h/t );
        t += clamp( h, 0.02, 0.10 );
        if( h<0.001 || t>tmax ) break;
    }
    return clamp( res, 0.0, 1.0 );

}

vec3 calcNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 nor = vec3(
	    fScene(pos+eps.xyy).x - fScene(pos-eps.xyy).x,
	    fScene(pos+eps.yxy).x - fScene(pos-eps.yxy).x,
	    fScene(pos+eps.yyx).x - fScene(pos-eps.yyx).x );
	return normalize(nor);
}


struct Light {
 	vec3 pos;
    vec3 color;
};
Light lights[NB_LIGHTS];


float G1V ( float dotNV, float k ) {
	return 1.0 / (dotNV*(1.0 - k) + k);
}

vec3 computePBRLighting ( in Light light, in vec3 position, in vec3 N, in vec3 V, in vec3 albedo, in float roughness, in vec3 F0 ) {

	float alpha = roughness*roughness;
	vec3 L = normalize(light.pos.xyz - position);
	vec3 H = normalize (V + L);

	float dotNL = clamp (dot (N, L), 0.0, 1.0);
	float dotNV = clamp (dot (N, V), 0.0, 1.0);
	float dotNH = clamp (dot (N, H), 0.0, 1.0);
	float dotLH = clamp (dot (L, H), 0.0, 1.0);

	float D, vis;
	vec3 F;

	// NDF : GGX
	float alphaSqr = alpha*alpha;
	float pi = 3.1415926535;
	float denom = dotNH * dotNH *(alphaSqr - 1.0) + 1.0;
	D = alphaSqr / (pi * denom * denom);

	// Fresnel (Schlick)
	float dotLH5 = pow (1.0 - dotLH, 5.0);
	F = F0 + (1.0 - F0)*(dotLH5);

	// Visibility term (G) : Smith with Schlick's approximation
	float k = alpha / 2.0;
	vis = G1V (dotNL, k) * G1V (dotNV, k);

	vec3 specular = /*dotNL **/ D * F * vis;

	vec3 ambient = vec3(.01);

	float invPi = 0.31830988618;
	vec3 diffuse = (albedo * invPi);


	return ambient + (diffuse + specular) * light.color.xyz * dotNL ;
}

vec3 addPBR( in vec3 position, in vec3 N, in vec3 V, in vec3 baseColor, in float metalMask, in float smoothness, in float reflectance) {
	vec3 color = vec3(0.0);

    float roughness = 1.0 - smoothness*smoothness;
    vec3 F0 = 0.16*reflectance*reflectance * (1.0-metalMask) + baseColor*metalMask;
    vec3 albedo = baseColor;
    
    float s = 0.0;
    
    
	for ( int i = 0; i < NB_LIGHTS; ++i ) {
		vec3 col = computePBRLighting ( lights[i], position, N, V, albedo, roughness, F0);
		color += col;	
        s += softshadow( position, normalize(lights[i].pos.xyz - position), 0.02, 2.5 );
	}

	return color*s;
}

vec3 render( in vec3 ro, in vec3 rd )
{ 
    vec3 col = vec3(0.8, 0.9, 1.0)*8.0; // Sky color
    
    vec2 res = castRay( ro, rd );
    float t = res.x;
    float m = res.y;
    vec3 p = ro + t*rd;
    
    if(m>-0.5) { // Intersection found
        if( m < 1.0 ) {
           // float f = mod( floor( 5.0*p.z ) + floor( 5.0*p.x ), 2.0 );
            vec3 sur = vec3(1.0,1.0,1.0)*smoothstep(-1.0,-0.6,sin(16.0*p.x));
            col = addPBR( p, calcNormal( p ), -rd, GOLD*sur, sur.x, 0.3+0.6*sur.x, 0.5 );
        } 
        else if( m < 2.0 ) {
            float f = mod( floor( 5.0*p.z ) + floor( 5.0*p.x ), 2.0 );
            col = addPBR(p, calcNormal( p ), -rd, vec3(0.5), 0.0, 0.3+0.6*f, 0.5 );
        }
        else if( m < 3.0 ) {
            vec3 sur = vec3(1.0,1.0,1.0)*smoothstep(-1.0,-0.4,sin(18.0*p.x));
            col = addPBR( p, calcNormal( p ), -rd, COPPER*sur, sur.x, 0.3+0.6*sur.x, 0.5 );
        }
        else if( m < 4.0 ) {
            vec3 sur = vec3(1.0,1.0,1.0)*smoothstep(-1.0,-0.0995,sin(106.0*p.x))*smoothstep(-1.0,-0.9,sin(47.0*p.z));
            col = addPBR( p, calcNormal( p ), -rd, vec3(0.2), 1.0-sur.x, 0.9*sur.x, 0.5 );
        }
        else if( m < 5.0 ) {
            vec3 sur = vec3(1.0)*smoothstep(-1.0,-0.765,sin(24.0*p.x))*smoothstep(-1.0,-0.4,sin(70.9*p.z));
            col = addPBR( p, calcNormal( p ), -rd, GOLD*(1.0-sur), sur.x, 0.3+0.6*sur.x, 0.5 );
        }
        else if( m < 6.0 ) {
            vec3 sur = vec3(1.0,1.0,1.0)*smoothstep(-1.0,-0.4,sin(18.0*p.x));
            col = addPBR( p, calcNormal( p ), -rd, ALUMINIUM*sur, sur.x, 0.3+0.6*sur.x, 0.5 );
        }
    }
    
	return col;
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

vec4 hejlToneMapping (in vec4 color) {
 	vec4 x = max(vec4(0.0), color-vec4(0.004)); 
	return (x * ((6.2*x)+vec4(0.5))) / max(x * ((6.2*x)+vec4(1.7))+vec4(0.06), vec4(1e-8));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    float time = 0.25*iGlobalTime;
    
    lights[0] = Light(vec3(0.0, 5.0, .0), vec3(1.0));   
	lights[1] = Light(vec3(12.0*sin(iGlobalTime), 8.0, 12.0*cos(iGlobalTime)), vec3(1.0));   	
    lights[2] = Light(vec3(-12.0*cos(-iGlobalTime), 8.0, 12.0*sin(-iGlobalTime)), vec3(.05));   

   

	vec2 q = fragCoord.xy/iResolution.xy;
    vec2 p = -1.0+2.0*q;
	p.x *= iResolution.x/iResolution.y;
#ifdef MOUSE
    vec2 mo = iMouse.xy/iResolution.xy;
#else
    vec2 mo = 0.0/iResolution.xy;
#endif
		 
	

	// camera	
	vec3 ro = vec3( 7.0*sin(time), 3.6 , -7.0*cos(time) );
	vec3 ta = vec3( 0.0 );
	
	// camera-to-world transformation
    mat3 ca = setCamera( ro, ta, 0.0 );
    
    // ray direction
	vec3 rd = ca * normalize( vec3(p.xy,2.5) );

    // render	
    vec3 col = render( ro, rd );

    #if 0
        col = pow( col, vec3(0.4545) );
        fragColor=vec4( col, 1.0 );
    #else 
    	float exposure = 0.032 + 0.023*sin(time-3.14);
   		fragColor = hejlToneMapping(vec4(col, 1.0) * exposure) ;
    #endif
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
