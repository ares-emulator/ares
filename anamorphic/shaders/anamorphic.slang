#version 450

/*
	Anamorphic v1.1b Shader 2020
	by Nerboruto 
	
	Permission is hereby granted, free of charge, to any person obtaining a copy of	this
	software and associated documentation files (the “Software”), to deal in the Software
	without restriction, including without limitation the rights to use, copy, modify, 
	merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to the following 
	conditions:

	The above copyright notice and this permission notice shall be included in all copies
	or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
	OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	float exc;
	float upc;
	float btc;
	float exp;
	float vuc;
	float vab;
} params;

#pragma parameter exc "orizontal correction hack (games where players stay at center)" 0.0 -10.0 10.0 0.25
#pragma parameter exp "border hack (hack for 2d games extra correction prepass)" 1.0 0.0 1.0 1.0
#pragma parameter vuc "vertical Upper resize hack (most important first pass)" 0.0 0.0 10.0 0.25
#pragma parameter vab "vertical Bottom resize hack (90-85 second pass)" 1.0 0.5 1.0 0.01
#pragma parameter upc "Upper  vertical Crop" 0.0 0.0 10.0 0.25
#pragma parameter btc "Bottom vertical Crop" 0.0 0.0 10.0 0.25

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
//orizontal
float AnamorphX(float CoordX)
{
	if (params.exc == 0.0) return 1.0;
	float rx = length(CoordX);
	float fx = -radians(params.exc);
	return (1.0-fx)/(1.0-fx*rx);
}
//vertical prepass
float AnamorphYP(float CoordY)
{
	float ryp = length(CoordY);
	float fyp = radians(6);
	if (ryp < 0.925) return 1.0;
	return (1.0-fyp)/((1.0-(1.0-0.925)*0.1)-fyp*ryp);
}
//Upper vertical
float AnamorphYU(float CoordY)
{
	if (params.vuc == 0.0) return 1.0;
	float ryu = CoordY;
	float fyu = -radians(params.vuc*0.5);
	if (ryu > 0.0) return 1.0;
	return (1.0-fyu)/(1.0-fyu*ryu);
}
//Bottom vertical
float AnamorphAB(float CoordY)
{
	if (params.vab == 1.0) return 1.0;
	float rab = CoordY;
	float fab = radians(6);
	if (rab < params.vab) return 1.0;
	return (1.0-fab)/((1.0-(1.0-params.vab)*0.1)-fab*rab);
}
//orizontal border
float AnamorphB(float CoordB)
{
	float rb = length(CoordB);
	float fb = -radians(15);
	if (rb < 0.99) return 1.0;
	return (1.0-fb)/(1.0060-fb*rb);
}

void main()
{
	vec2 RCoord = vTexCoord * 2.0 - 1.0;
	if (params.exp == 1.0)
	{
	RCoord.x *=1.0105;
	RCoord.x *= AnamorphB(RCoord.x);
	RCoord.y *= AnamorphYP(RCoord.y);
	RCoord.y *=0.9925;
	}
	RCoord.x *= AnamorphX(RCoord.x);
	RCoord.y *= AnamorphYU(RCoord.y);
	RCoord.y *= AnamorphAB(RCoord.y);
	RCoord = RCoord * 0.5 + 0.5;
	RCoord.y = (RCoord.y+(params.upc+(params.vuc*0.95))*0.01)/(1.0+(params.upc+params.btc+(params.vuc*0.95))*0.01);
	vec3 res = texture(Source, RCoord).rgb;
    FragColor = vec4(res,0.0);
}