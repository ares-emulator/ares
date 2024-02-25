#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float flip_image;
} params;

#define vFragCoord vTexCoord.xy * params.OutputSize.xy
#define iResolution params.OutputSize.xy
#define iTime params.FrameCount / 60.0
#define iChannel0 Source
#define iChannel1 cubeMap
#define iChannel2 table
#define iMouse vec3(0.0)
#define vFragColor FragColor
#define fragColor FragColor

#pragma parameter flip_image "Flip Image (for hardware-rendered cores)" 0.0 0.0 1.0 1.0

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
   vTexCoord = (bool(params.flip_image)) ? vec2(TexCoord.x, 1.0 - TexCoord.y) : TexCoord.xy;
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

// Uncomment different defines in Buf B to run different shaders on TV

// Postprocessing Pass
// Motion blur, Depth of Field, Vignetting & Tonemap


#define ENABLE_DOF
#define ENABLE_MOTION_BLUR

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

vec3 Tonemap( vec3 x )
{
#if 0 
    
    vec3 luminanceCoeffsBT709 = vec3( 0.2126f, 0.7152f, 0.0722f );
    float f = dot( x, luminanceCoeffsBT709 );
    x /= f;        
    f = 1.0f - exp(-f);    
    x *= f;    
    x = mix( x, vec3(f), f*f );
    
    return x;
#else       
    float a = 0.010;
    float b = 0.132;
    float c = 0.010;
    float d = 0.163;
    float e = 0.101;

    return ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e );    
#endif    
}


float GetVignetting( const in vec2 vUV, float fScale, float fPower, float fStrength )
{
	vec2 vOffset = (vUV - 0.5) * sqrt(2.0) * fScale;
	
	float fDist = max( 0.0, 1.0 - length( vOffset ) );
    
	float fShade = 1.0 - pow( fDist, fPower );
    
    fShade = 1.0 - fShade * fStrength;

	return fShade;
}




float GetCoC( float fDistance, float fPlaneInFocus )
{
#ifdef ENABLE_DOF    
	// http://http.developer.nvidia.com/GPUGems/gpugems_ch23.html

    float fAperture = min(1.0, fPlaneInFocus * fPlaneInFocus * 0.5);
    float fFocalLength = 0.03;
    
	return abs(fAperture * (fFocalLength * (fDistance - fPlaneInFocus)) /
          (fDistance * (fPlaneInFocus - fFocalLength)));  
#else
    return 0.0f;
#endif    
}

// Depth of field pass

#define BLUR_TAPS 64
float fGolden = 3.141592 * (3.0 - sqrt(5.0));

#define MOD2 vec2(4.438975,3.972973)

float Hash( float p ) 
{
    // https://www.shadertoy.com/view/4djSRW - Dave Hoskins
	vec2 p2 = fract(vec2(p) * MOD2);
    p2 += dot(p2.yx, p2.xy+19.19);
	return fract(p2.x * p2.y);    
}

void main()
{
   vec2 fragCoord = vTexCoord.xy * params.OutputSize.xy;
    CameraState camCurr;
	Cam_LoadState( camCurr, iChannel0, ivec2(0) );

    CameraState camPrev;
	Cam_LoadState( camPrev, iChannel0, ivec2(3,0) );
    
	vec2 vUV = fragCoord.xy / iResolution.xy;
 	//vUV -= camCurr.vJitter / iResolution.xy;    // TAA has removed jitter

    vec4 vSample = texelFetch( iChannel0, ivec2(fragCoord.xy), 0 ).rgba;
    
    int iObjectId;
    float fDepth = DecodeDepthAndObjectId( vSample.w, iObjectId );
    
    vec3 vRayOrigin, vRayDir;
    
    Cam_GetCameraRay( vUV, camCurr, vRayOrigin, vRayDir );    
    vec3 vWorldPos = vRayOrigin + vRayDir * fDepth;
        
    vec2 vPrevUV = Cam_GetUVFromWindowCoord( Cam_WorldToWindowCoord(vWorldPos, camPrev) );// - camPrev.vJitter / iResolution.xy;
        
	vec3 vResult = vec3(0.0);
    
    float fTot = 0.0;
    
    float fPlaneInFocus = camCurr.fPlaneInFocus;
        
	float fCoC = GetCoC( fDepth, camCurr.fPlaneInFocus );
        
    float r = 1.0;
    vec2 vangle = vec2(0.0,fCoC); // Start angle
    
    float fWeight = max( 0.001, fCoC );    
    vResult.rgb = vSample.rgb * fWeight;
    fTot += fWeight;
    
#if defined(ENABLE_DOF) || defined(ENABLE_MOTION_BLUR)    
    float fMotionBlurTaps = float(BLUR_TAPS);
    
    float fShutterAngle = 0.5;
    
    float f = 0.0;
    float fIndex = 0.0;
    for(int i=1; i<BLUR_TAPS; i++)
    {
        float fRandomT = Hash( iTime + fIndex + vUV.x + vUV.y * 12.345);
        float fOrderedT = fIndex / fMotionBlurTaps;
        
        float fDofT = fOrderedT;
        float fMotionT = fRandomT;
        
        vec2 vTapUV = vUV;
        #ifdef ENABLE_MOTION_BLUR
        vTapUV = mix( vTapUV, vPrevUV, (fMotionT - 0.5) * fShutterAngle );
        #endif
                
        // http://blog.marmakoide.org/?p=1
        
        float fTheta = fDofT * fGolden * fMotionBlurTaps;
        float fRadius = fCoC * sqrt( fDofT * fMotionBlurTaps ) / sqrt( fMotionBlurTaps );        
        
        vTapUV += vec2( sin(fTheta), cos(fTheta) ) * fRadius;
        
        vec4 vTapSample = textureLod( iChannel0, vTapUV, 0.0 ).rgba;
	    //vec4 vTapTexel = texelFetch( iChannel0, ivec2(vTapUV.xy * iResolution.xy), 0 ).rgba;
        
        int iTapObjectId;
        float fTapDepth = DecodeDepthAndObjectId( vTapSample.w, iTapObjectId );
        
        if ( fTapDepth > 0.0 )
        {            
  		  	float fCurrCoC = GetCoC( fTapDepth, fPlaneInFocus );
            
            float fCurrWeight = max( 0.001, fCurrCoC );
            
    		vResult += vTapSample.rgb * fCurrWeight;
        	fTot += fCurrWeight;
        }
        f += 1.0;
        fIndex += 1.0;
    }
#endif    
    vResult /= fTot;
    
	fragColor = vec4(vResult, 1.0);    
    
    float fShade = GetVignetting( vUV, 0.7, 2.0, 1.0 );
    
    fragColor.rgb *= fShade;
    
    fragColor.rgb = Tonemap( fragColor.rgb );
    fragColor.a = 1.0;
}
