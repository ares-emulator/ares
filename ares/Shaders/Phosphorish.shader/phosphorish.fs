/*    Phosphorish

    Copyright (C) 2011 Themaister

    This program is licensed in the public domain
*/

#version 150

uniform sampler2D source[];
uniform vec4 sourceSize[];
uniform vec4 targetSize;

in Vertex {
  vec2 texCoord;
};

out vec4 fragColor;

      vec3 to_focus(float pixel)
      {
         pixel = mod(pixel + 3.0, 3.0);
         if (pixel >= 2.0) // Blue
            return vec3(pixel - 2.0, 0.0, 3.0 - pixel);
         else if (pixel >= 1.0) // Green
            return vec3(0.0, 2.0 - pixel, pixel - 1.0);
         else // Red
            return vec3(1.0 - pixel, pixel, 0.0);
      }

      void main()
      {
         float y = mod(texCoord.y * sourceSize[0].y, 1.0);
         float intensity = exp(-0.2 * y);

         vec2 one_x = vec2(1.0 / (3.0 * sourceSize[0].x), 0.0);

         vec3 color = texture(source[0], texCoord.xy - 0.0 * one_x).rgb;
         vec3 color_prev = texture(source[0], texCoord.xy - 1.0 * one_x).rgb;
         vec3 color_prev_prev = texture(source[0], texCoord.xy - 2.0 * one_x).rgb;

         float pixel_x = 3.0 * texCoord.x * sourceSize[0].x;

         vec3 focus = to_focus(pixel_x - 0.0);
         vec3 focus_prev = to_focus(pixel_x - 1.0);
         vec3 focus_prev_prev = to_focus(pixel_x - 2.0);

         vec3 result =
            0.8 * color * focus +
            0.6 * color_prev * focus_prev +
            0.3 * color_prev_prev * focus_prev_prev;

         result = 2.3 * pow(result, vec3(1.4));

         fragColor = vec4(intensity * result, 1.0);
      }