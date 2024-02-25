#version 450

/*
   OSSC Slot Mask simulator
   by DariusG @2023

*/

layout(push_constant) uniform Push
{
 float MASK_INTENSITY;
 float InputGamma;
 float OutputGamma;
 float THRESH,size, msk_scale;
} params;

// Parameter lines go here:

#pragma parameter msk_scale "Mask Scale, to be equal to OSSC LINES" 4.0 3.0 5.0 1.0
#define msk_scale params.msk_scale

#pragma parameter MASK_INTENSITY "Mask Brightness" 0.2 0.0 1.0 0.05
#define MASK_INTENSITY params.MASK_INTENSITY

#pragma parameter size "Mask Size" 1.0 1.0 8.0 1.0
#define size params.size

#pragma parameter THRESH "Mask Threshold" 0.3 0.0 1.0 0.05
#define THRESH params.THRESH


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} global;

#define SourceSize global.SourceSize


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 ps;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord.xy = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 ps;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;

vec3 mask(vec2 pos, vec3 col)
{
    vec3 msk = vec3 (MASK_INTENSITY);
    float x,y;
    vec2 p = pos*global.OutputSize.xy;
    p = (p/size);

if (msk_scale == 3.0)
{
     x = fract(p.x/6.0);
     y = fract(p.y/3.0);
    if (x < 0.5){     
     if (x<1.0/6.0 &&                   y > 1.0/3.0) msk.b = 1.0;
     else if (x<2.0/6.0 && x>1.0/6.0 && y > 1.0/3.0 ) msk.g = 1.0;
     else if (x<3.0/6.0 && x>2.0/6.0 && y > 1.0/3.0 ) msk.r = 1.0;
    }

 else if (x > 0.4999){     
     if (x<4.0/6.0 &&                   y < 2.0/3.0 || x<4.0/6.0 &&              y >= 3.0/3.0 ) msk.b = 1.0;
     else if (x<5.0/6.0 && x>4.0/6.0 && y < 2.0/3.0 || x<5.0/6.0 && x>4.0/6.0 && y >= 3.0/3.0) msk.g = 1.0;
     else if (x<6.0/6.0 && x>5.0/6.0 && y < 2.0/3.0 || x<6.0/6.0 && x>5.0/6.0 && y >= 3.0/3.0) msk.r = 1.0;
    }
}
     

if (msk_scale == 4.0)
{
     x = fract(p.x/6.0);
     y = fract(p.y/4.0);
    if (x < 0.5){     
     if (x<1.0/6.0 &&                   y > 1.0/4.0) msk.b = 1.0;
     else if (x<2.0/6.0 && x>1.0/6.0 && y > 1.0/4.0 ) msk.g = 1.0;
     else if (x<3.0/6.0 && x>2.0/6.0 && y > 1.0/4.0 ) msk.r = 1.0;
    }

 else if (x > 0.4999){     
     if (x<4.0/6.0 &&                   y < 2.0/4.0 || x<4.0/6.0 && y >= 3.0/4.0 ) msk.b = 1.0;
     else if (x<5.0/6.0 && x>4.0/6.0 && y < 2.0/4.0 || x<5.0/6.0 && x>4.0/6.0 && y >= 3.0/4.0) msk.g = 1.0;
     else if (x<6.0/6.0 && x>5.0/6.0 && y < 2.0/4.0 || x<6.0/6.0 && x>5.0/6.0 && y >= 3.0/4.0) msk.r = 1.0;
    }
}

if (msk_scale == 5.0)
{
     x = fract(p.x/6.0);
     y = fract(p.y/5.0);
    if (x < 0.5){     
     if (x<1.0/6.0 &&                   y > 1.0/5.0) msk.b = 1.0;
     else if (x<2.0/6.0 && x>1.0/6.0 && y > 1.0/5.0 ) msk.g = 1.0;
     else if (x<3.0/6.0 && x>2.0/6.0 && y > 1.0/5.0 ) msk.r = 1.0;
    }

 else if (x > 0.4999){     
     if      (x<4.0/6.0              && y < 2.0/5.0 || x<4.0/6.0              && y >= 3.0/5.0) msk.b = 1.0;
     else if (x<5.0/6.0 && x>4.0/6.0 && y < 2.0/5.0 || x<5.0/6.0 && x>4.0/6.0 && y >= 3.0/5.0 ) msk.g = 1.0;
     else if (x<6.0/6.0 && x>5.0/6.0 && y < 2.0/5.0 || x<6.0/6.0 && x>5.0/6.0 && y >= 3.0/5.0 ) msk.r = 1.0;
    }
}

    col *= msk;
    
    return col;
}

void main()
{    
    vec3 col = texture(Source,vTexCoord).rgb;  
    float l = dot(col,vec3(0.22,0.70,0.08));

    col = col*col;
    col = mix(mask(vTexCoord,col), col,l*THRESH);
    col = sqrt(col);
    
    col *= mix(1.0,1.3,l);  //brightboost
    FragColor = vec4(col, 1.0);
} 
