#version 450
#include "config.inc"

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
layout(set = 0, binding = 2) uniform sampler2D Original;


void main() {
   FragColor = vec4(texture(Original, vTexCoord).xyz,1.0);

  /* //Use to debug:
      vec4 pixel;
      //pixel=vec4(abs(sin(params.FrameCount/3.14/20))); //white fade
      //pixel=vec4(abs(sin(params.FrameCount/3.14/20)),0.0,0.0,0.0); //red fade
      //pixel=vec4(0.2);
      if (mod(params.FrameCount,100) == 0.0) {
      pixel = vec4(0.6);
         } else {
         pixel = vec4(0.0);
      }
      FragColor = pixel;
*/
}
