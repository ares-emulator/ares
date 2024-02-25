#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#define vFragCoord vTexCoord.xy * params.OutputSize.xy
#define iResolution params.OutputSize.xy
#define iTime params.FrameCount / 60.0
#define iChannel0 Source
#define iChannel1 cubeMap
#define iChannel2 table
#define iMouse vec3(0.0)
#define vFragColor FragColor

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D cubeMap;
layout(set = 0, binding = 4) uniform sampler2D table;

// Meta CRT - @P_Malin
// https://www.shadertoy.com/view/4dlyWX#
// In which I add and remove aliasing

// Temporal Anti-aliasing Pass

#define ENABLE_TAA

///////////////////////////
// Hash Functions
///////////////////////////

// From: Hash without Sine by Dave Hoskins
// https://www.shadertoy.com/view/4djSRW

// *** Use this for integer stepped ranges, ie Value-Noise/Perlin noise functions.
//#define HASHSCALE1 .1031
//#define HASHSCALE3 vec3(.1031, .1030, .0973)
//#define HASHSCALE4 vec4(1031, .1030, .0973, .1099)

// For smaller input rangers like audio tick or 0-1 UVs use these...
#define HASHSCALE1 443.8975
#define HASHSCALE3 vec3(443.897, 441.423, 437.195)
#define HASHSCALE4 vec3(443.897, 441.423, 437.195, 444.129)


//----------------------------------------------------------------------------------------
//  2 out, 1 in...
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * HASHSCALE3);
	p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}

///  2 out, 3 in...
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}

//  1 out, 3 in...
float hash13(vec3 p3)
{
	p3  = fract(p3 * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}


///////////////////////////
// Data Storage
///////////////////////////

vec4 LoadVec4( sampler2D tex, in ivec2 vAddr )
{
    return texelFetch( tex, vAddr, 0 );
}

vec3 LoadVec3( sampler2D tex, in ivec2 vAddr )
{
    return LoadVec4( tex, vAddr ).xyz;
}

bool AtAddress( ivec2 p, ivec2 c ) { return all( equal( p, c ) ); }

void StoreVec4( in ivec2 vAddr, in vec4 vValue, inout vec4 fragColor, in ivec2 fragCoord )
{
    fragColor = AtAddress( fragCoord, vAddr ) ? vValue : fragColor;
}

void StoreVec3( in ivec2 vAddr, in vec3 vValue, inout vec4 fragColor, in ivec2 fragCoord )
{
    StoreVec4( vAddr, vec4( vValue, 0.0 ), fragColor, fragCoord);
}

///////////////////////////
// Camera
///////////////////////////

struct CameraState
{
    vec3 vPos;
    vec3 vTarget;
    float fFov;
    vec2 vJitter;
    float fPlaneInFocus;
};
    
void Cam_LoadState( out CameraState cam, sampler2D tex, ivec2 addr )
{
    vec4 vPos = LoadVec4( tex, addr + ivec2(0,0) );
    cam.vPos = vPos.xyz;
    vec4 targetFov = LoadVec4( tex, addr + ivec2(1,0) );
    cam.vTarget = targetFov.xyz;
    cam.fFov = targetFov.w;
    vec4 jitterDof = LoadVec4( tex, addr + ivec2(2,0) );
    cam.vJitter = jitterDof.xy;
    cam.fPlaneInFocus = jitterDof.z;
}

void Cam_StoreState( ivec2 addr, const in CameraState cam, inout vec4 fragColor, in ivec2 fragCoord )
{
    StoreVec4( addr + ivec2(0,0), vec4( cam.vPos, 0 ), fragColor, fragCoord );
    StoreVec4( addr + ivec2(1,0), vec4( cam.vTarget, cam.fFov ), fragColor, fragCoord );    
    StoreVec4( addr + ivec2(2,0), vec4( cam.vJitter, cam.fPlaneInFocus, 0 ), fragColor, fragCoord );    
}

mat3 Cam_GetWorldToCameraRotMatrix( const CameraState cameraState )
{
    vec3 vForward = normalize( cameraState.vTarget - cameraState.vPos );
	vec3 vRight = normalize( cross(vec3(0, 1, 0), vForward) );
	vec3 vUp = normalize( cross(vForward, vRight) );
    
    return mat3( vRight, vUp, vForward );
}

vec2 Cam_GetViewCoordFromUV( const in vec2 vUV )
{
	vec2 vWindow = vUV * 2.0 - 1.0;
	vWindow.x *= iResolution.x / iResolution.y;

	return vWindow;	
}

void Cam_GetCameraRay( const vec2 vUV, const CameraState cam, out vec3 vRayOrigin, out vec3 vRayDir )
{
    vec2 vView = Cam_GetViewCoordFromUV( vUV );
    vRayOrigin = cam.vPos;
    float fPerspDist = 1.0 / tan( radians( cam.fFov ) );
    vRayDir = normalize( Cam_GetWorldToCameraRotMatrix( cam ) * vec3( vView, fPerspDist ) );
}

vec2 Cam_GetUVFromWindowCoord( const in vec2 vWindow )
{
    vec2 vScaledWindow = vWindow;
    vScaledWindow.x *= iResolution.y / iResolution.x;

    return (vScaledWindow * 0.5 + 0.5);
}

vec2 Cam_WorldToWindowCoord(const in vec3 vWorldPos, const in CameraState cameraState )
{
    vec3 vOffset = vWorldPos - cameraState.vPos;
    vec3 vCameraLocal;

    vCameraLocal = vOffset * Cam_GetWorldToCameraRotMatrix( cameraState );
	
    vec2 vWindowPos = vCameraLocal.xy / (vCameraLocal.z * tan( radians( cameraState.fFov ) ));
    
    return vWindowPos;
}

float EncodeDepthAndObject( float depth, int objectId )
{
    //depth = max( 0.0, depth );
    //objectId = max( 0, objectId + 1 );
    //return exp2(-depth) + float(objectId);
    return depth;
}

float DecodeDepthAndObjectId( float value, out int objectId )
{
    objectId = 0;
    return max(0.0, value);
    //objectId = int( floor( value ) ) - 1; 
    //return abs( -log2(fract(value)) );
}

///////////////////////////////


#define iChannelCurr iChannel0
#define iChannelHistory iChannel1

vec3 Tonemap( vec3 x )
{
    float a = 0.010;
    float b = 0.132;
    float c = 0.010;
    float d = 0.163;
    float e = 0.101;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );
}

vec3 TAA_ColorSpace( vec3 color )
{
    return Tonemap(color);
}

void main()
{
    CameraState camCurr;
	Cam_LoadState( camCurr, iChannelCurr, ivec2(0) );
    
    CameraState camPrev;
	Cam_LoadState( camPrev, iChannelHistory, ivec2(0) );

    vec2 vUV = vFragCoord.xy / iResolution.xy;
 	vec2 vUnJitterUV = vUV - camCurr.vJitter / iResolution.xy;    
    
    vFragColor = textureLod(iChannelCurr, vUnJitterUV, 0.0);
    
    
#ifdef ENABLE_TAA
    vec3 vRayOrigin, vRayDir;
    Cam_GetCameraRay( vUV, camCurr, vRayOrigin, vRayDir );    
    float fDepth;
    int iObjectId;
    vec4 vCurrTexel = texelFetch( iChannelCurr, ivec2(vFragCoord.xy), 0);
    fDepth = DecodeDepthAndObjectId( vCurrTexel.w, iObjectId );
    vec3 vWorldPos = vRayOrigin + vRayDir * fDepth;
    
    vec2 vPrevUV = Cam_GetUVFromWindowCoord( Cam_WorldToWindowCoord(vWorldPos, camPrev) );// + camPrev.vJitter / iResolution.xy;
        
    if ( all( greaterThanEqual( vPrevUV, vec2(0) )) && all( lessThan( vPrevUV, vec2(1) )) )
	{
        vec3 vMin = vec3( 10000);
        vec3 vMax = vec3(-10000);
        
	    ivec2 vCurrXY = ivec2(floor(vFragCoord.xy));    
        
        int iNeighborhoodSize = 1;
        for ( int iy=-iNeighborhoodSize; iy<=iNeighborhoodSize; iy++)
        {
            for ( int ix=-iNeighborhoodSize; ix<=iNeighborhoodSize; ix++)
            {
                ivec2 iOffset = ivec2(ix, iy);
		        vec3 vTest = TAA_ColorSpace( texelFetch( iChannelCurr, vCurrXY + iOffset, 0 ).rgb );
                                
                vMin = min( vMin, vTest );
                vMax = max( vMax, vTest );
            }
        }
        
        float epsilon = 0.001;
        vMin -= epsilon;
        vMax += epsilon;
        
        float fBlend = 0.0f;
        
        //ivec2 vPrevXY = ivec2(floor(vPrevUV.xy * iResolution.xy));
        vec4 vHistory = textureLod( iChannelHistory, vPrevUV, 0.0 );

        vec3 vPrevTest = TAA_ColorSpace( vHistory.rgb );
        if( all( greaterThanEqual(vPrevTest, vMin ) ) && all( lessThanEqual( vPrevTest, vMax ) ) )
        {
            fBlend = 0.9;
            //vFragColor.r *= 0.0;
        }
        
        vFragColor.rgb = mix( vFragColor.rgb, vHistory.rgb, fBlend);
    }  
    else
    {
        //vFragColor.gb *= 0.0;
    }

#endif
    
    vFragColor.rgb += (hash13( vec3( vFragCoord, iTime ) ) * 2.0 - 1.0) * 0.03;
    
	Cam_StoreState( ivec2(0), camCurr, vFragColor, ivec2(vFragCoord.xy) );    
	Cam_StoreState( ivec2(3,0), camPrev, vFragColor, ivec2(vFragCoord.xy) ); 
}