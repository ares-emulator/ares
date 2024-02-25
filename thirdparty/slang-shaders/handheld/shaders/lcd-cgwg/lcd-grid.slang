#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   float GRID_STRENGTH, gamma;
} global;

#pragma parameter GRID_STRENGTH "LCD Grid Strength" 0.05 0.0 1.0 0.01
#pragma parameter gamma "LCD Input Gamma" 2.2 1.0 5.0 0.1

#define round(x) floor( (x) + 0.5 )
#define TEX2D(c) pow(texture(Source, (c)), vec4(global.gamma))

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

float intsmear_func(float z)
{
  float z2 = z*z;
  float z4 = z2*z2;
  float z8 = z4*z4;
  return z - 2.0/3.0*z*z2 - 1.0/5.0*z*z4 + 4.0/7.0*z*z2*z4 - 1.0/9.0*z*z8
    - 2.0/11.0*z*z2*z8 + 1.0/13.0*z*z4*z8;
}

float intsmear(float x, float dx)
{
  const float d = 1.5;
  float zl = clamp((x-dx)/d,-1.0,1.0);
  float zh = clamp((x+dx)/d,-1.0,1.0);
  return d * ( intsmear_func(zh) - intsmear_func(zl) )/(2.0*dx);
}

void main()
{
  vec2 texelSize = global.SourceSize.zw;
  vec2 subtexelSize = texelSize / vec2(3.0,1.0);
  vec2 range;
  range = global.SourceSize.xy / (global.OutputSize.xy * global.SourceSize.xy);
  
  float left   = vTexCoord.x - texelSize.x*0.5;
  float top    = vTexCoord.y + range.y;
  float right  = vTexCoord.x + texelSize.x*0.5;
  float bottom = vTexCoord.y - range.y;
  
  vec4 lcol, rcol;
  float subpix = mod(vTexCoord.x/subtexelSize.x+1.5,3.0);
  float rsubpix = range.x/subtexelSize.x;
  lcol = vec4(intsmear(subpix+1.0,rsubpix),intsmear(subpix    ,rsubpix),
	      intsmear(subpix-1.0,rsubpix),0.0);
  rcol = vec4(intsmear(subpix-2.0,rsubpix),intsmear(subpix-3.0,rsubpix),
	      intsmear(subpix-4.0,rsubpix),0.0);
			  
  vec4 topLeftColor     = TEX2D((floor(vec2(left, top)     / texelSize) + 0.5) * texelSize) * lcol;
  vec4 bottomRightColor = TEX2D((floor(vec2(right, bottom) / texelSize) + 0.5) * texelSize) * rcol;
  vec4 bottomLeftColor  = TEX2D((floor(vec2(left, bottom)  / texelSize) + 0.5) * texelSize) * lcol;
  vec4 topRightColor    = TEX2D((floor(vec2(right, top)    / texelSize) + 0.5) * texelSize) * rcol;
  
  vec2 border = round(vTexCoord.st/subtexelSize);
  vec2 bordert = clamp((border+vec2(0.0,+global.GRID_STRENGTH)) * subtexelSize,
		       vec2(left, bottom), vec2(right, top));
  vec2 borderb = clamp((border+vec2(0.0,-global.GRID_STRENGTH)) * subtexelSize,
		       vec2(left, bottom), vec2(right, top));
  float totalArea = 2.0 * range.y;  

   vec4 averageColor;
  averageColor  = ((top - bordert.y)    / totalArea) * topLeftColor;
  averageColor += ((borderb.y - bottom) / totalArea) * bottomRightColor;
  averageColor += ((borderb.y - bottom) / totalArea) * bottomLeftColor;
  averageColor += ((top - bordert.y)    / totalArea) * topRightColor;
  
   FragColor = pow(averageColor,vec4(1.0/global.gamma));
}
