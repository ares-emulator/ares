#version 450

//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main (void) {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PassOutput3;

void main()
{
	vec4 inputY = texture(PassOutput3, vTexCoord.xy);
	vec4 inputUV = texture(Source, vTexCoord.xy);

	vec4 yuva = vec4(inputY.x, (inputUV.y - 0.5), (inputUV.z - 0.5), 1.0);

	vec4 rgba = vec4(0.0);

	rgba.r = yuva.x * 1.0 + yuva.y * 0.0 + yuva.z * 1.4;
	rgba.g = yuva.x * 1.0 + yuva.y * -0.343 + yuva.z * -0.711;
	rgba.b = yuva.x * 1.0 + yuva.y * 1.765 + yuva.z * 0.0;
	rgba.a = 1.0;
	
	FragColor = rgba;
}