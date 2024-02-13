#version 450

// Chroma / YIQ
// by torridgristle

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BrightenLevel;
	float BrightenAmount;
	float ContrastAmount;
	float ChromaLevel;
	float IAmount;
	float QAmount;
	float ITilt;
	float QTilt;
	float ITiltHigh;
	float QTiltHigh;
	float ITiltMid;
	float QTiltMid;
	float ITiltLow;
	float QTiltLow;
	float LumBoost;
} params;

#pragma parameter BrightenLevel "Brighten Level" 2.0 1.0 10.0 1.0
#pragma parameter BrightenAmount "Brighten Amount" 0.0 0.0 1.0 0.1
#pragma parameter ContrastAmount "Contrast Amount" 0.0 0.0 1.0 0.1
#pragma parameter ChromaLevel "Chroma Level" 2.0 1.0 10.0 1.0
#pragma parameter IAmount "I Amount" 0.0 0.0 1.0 0.1
#pragma parameter QAmount "Q Amount" 0.0 0.0 1.0 0.1
#pragma parameter ITilt "I Tilt" 0.5 0.4 0.6 0.01
#pragma parameter QTilt "Q Tilt" 0.5 0.4 0.6 0.01
#pragma parameter ITiltHigh "I Tilt High" 0.5 0.4 0.6 0.01
#pragma parameter QTiltHigh "Q Tilt High" 0.5 0.4 0.6 0.01
#pragma parameter ITiltMid "I Tilt Mid" 0.5 0.4 0.6 0.01
#pragma parameter QTiltMid "Q Tilt Mid" 0.5 0.4 0.6 0.01
#pragma parameter ITiltLow "I Tilt Low" 0.5 0.4 0.6 0.01
#pragma parameter QTiltLow "Q Tilt Low" 0.5 0.4 0.6 0.01
#pragma parameter LumBoost "Luma Boost" 0.0 0.0 1.0 0.1

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

#define RGB_to_YIQ 	mat3x3( 0.299 , 0.595716 , 0.211456 ,	0.587    , -0.274453 , -0.522591 ,		0.114    , -0.321263 , 0.311135 )
#define YIQ_to_RGB 	mat3x3( 1.0   , 1.0      , 1.0      ,	0.9563   , -0.2721   , -1.1070   ,		0.6210   , -0.6474   , 1.7046   )

vec3 Chroma(vec3 Photo, float Level) {
    Photo *= RGB_to_YIQ;
    float I_Tilt = ((params.ITilt - 0.5) * 2);
    float Q_Tilt = ((params.QTilt - 0.5) * 2);
    float LumHigh = max(Photo.x-0.5,0.0)*2.0;
    float LumMid  = 1-abs(Photo.x-0.5)*2.0;
    float LumLow  = max((1-Photo.x)-0.5,0.0)*2.0;
    float I_TiltHigh = ((params.ITiltHigh - 0.5) * 2) * LumHigh;
    float Q_TiltHigh = ((params.QTiltHigh - 0.5) * 2) * LumHigh;
    float I_TiltMid = ((params.ITiltMid - 0.5) * 2) * LumMid;
    float Q_TiltMid = ((params.QTiltMid - 0.5) * 2) * LumMid;
    float I_TiltLow = ((params.ITiltLow - 0.5) * 2) * LumLow;
    float Q_TiltLow = ((params.QTiltLow - 0.5) * 2) * LumLow;
    Photo.yz = mix(Photo.yz,(1.0-pow(1.0-abs(Photo.yz),vec2(Level)))*sign(Photo.yz),vec2(params.IAmount,params.QAmount));
    Photo.y = Photo.y * (1-abs(I_Tilt)) + I_Tilt;
    Photo.z = Photo.z * (1-abs(Q_Tilt)) + Q_Tilt;
    Photo.y = Photo.y * (1-abs(I_TiltHigh)) + I_TiltHigh;
    Photo.z = Photo.z * (1-abs(Q_TiltHigh)) + Q_TiltHigh;
    Photo.y = Photo.y * (1-abs(I_TiltMid)) + I_TiltMid;
    Photo.z = Photo.z * (1-abs(Q_TiltMid)) + Q_TiltMid;
    Photo.y = Photo.y * (1-abs(I_TiltLow)) + I_TiltLow;
    Photo.z = Photo.z * (1-abs(Q_TiltLow)) + Q_TiltLow;
    Photo.x = mix(Photo.x,1-pow(1-Photo.x,2.0),params.LumBoost);
    Photo *= YIQ_to_RGB;
    Photo = clamp(Photo,0.0,1.0);
    Photo = mix(Photo,1.0-pow(1.0-Photo,vec3(params.BrightenLevel)),params.BrightenAmount);
    Photo = mix(Photo,Photo * Photo * (3 - 2 * Photo),params.ContrastAmount);
    return Photo;
}

void main()
{
    vec3 Picture = texture(Source, vTexCoord).xyz;

    FragColor = vec4(Chroma(Picture,params.ChromaLevel),1.0);
}
