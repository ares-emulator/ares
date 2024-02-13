#version 450

// CRT-torridgristle - ScanlineSimple pass
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float ScanlineSize;
   float YIQAmount;
} params;

#pragma parameter ScanlineSize "Scanline Size" 3.0 2.0 32.0 1.0
#pragma parameter YIQAmount "YIQ Amount" 1.0 0.0 1.0 0.05

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

#define Pi 3.1415926536
const mat3 RGB_to_YIQ = mat3( 0.299 , 0.595716 , 0.211456 ,	0.587    , -0.274453 , -0.522591 ,		0.114    , -0.321263 , 0.311135 );
const mat3  YIQ_to_RGB =   mat3( 1.0   , 1.0      , 1.0      ,	0.9563   , -0.2721   , -1.1070   ,		0.6210   , -0.6474   , 1.7046   );

void main()
{
	vec3 Picture = texture(Source,vTexCoord).xyz;
    
    float HSBrightness = max(max(Picture.x,Picture.y),max(Picture.y,Picture.z));
    float YIQLuminance = ((0.299*Picture.x) + (0.587*Picture.y) + (0.114*Picture.z));
    
    float HSBYIQHybrid = mix(HSBrightness,YIQLuminance,HSBrightness);
    
	float Scanline = mod(vTexCoord.y*params.OutputSize.y,params.ScanlineSize)/params.ScanlineSize;
    Scanline = 1.-abs(Scanline-0.5)*2.;
    Scanline = 1.-pow(1.-Scanline,2.0);
    
    Scanline = clamp(sqrt(Scanline)-(1-HSBYIQHybrid),0.0,1.0);
    Scanline /= HSBYIQHybrid;



    vec3 YIQApplication = Picture * RGB_to_YIQ;
         YIQApplication.x *= Scanline;
         YIQApplication *= YIQ_to_RGB;

    FragColor = vec4(mix(Picture*Scanline,YIQApplication*mix(Scanline,1.0,0.75),params.YIQAmount),1.0);
    //FragColor = vec4(Picture,1.0);
    //FragColor = vec4(Scanline);
    
}