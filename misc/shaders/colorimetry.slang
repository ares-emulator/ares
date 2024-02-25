#version 450

/*
   Colorimetry shader
   Ported from Drag's NES Palette Generator
   http://drag.wootest.net/misc/palgen.html
*/

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

layout(push_constant) uniform Push
{
   float color_mode;
   float white_point_d93;
   float clipping_method;
} params;

#pragma parameter color_mode "Colorimetry mode" 0.0 0.0 2.0 1.0
#pragma parameter white_point_d93 "Use D93 white point" 1.0 0.0 1.0 1.0
#pragma parameter clipping_method "Color clipping method" 0.0 0.0 2.0 1.0

#define color_mode params.color_mode
#define white_point_d93 params.white_point_d93
#define clipping_method params.clipping_method

vec3 huePreserveClipDarken(float r, float g, float b)
{
   float ratio = 1.0;
   if ((r > 1.0) || (g > 1.0) || (b > 1.0))
   {
      float max = r;
      if (g > max)
         max = g;
      if (b > max)
         max = b;
      ratio = 1.0 / max;
   }

   r *= ratio;
   g *= ratio;
   b *= ratio;

   r = clamp(r, 0.0, 1.0);
   g = clamp(g, 0.0, 1.0);
   b = clamp(b, 0.0, 1.0);

   return vec3(r, g, b);
}

vec3 huePreserveClipDesaturate(float r, float g, float b)
{
   float l = (.299 * r) + (0.587 * g) + (0.114 * b);
   bool ovr = false;
   float ratio = 1.0;

   if ((r > 1.0) || (g > 1.0) || (b > 1.0))
   {
      ovr = true;
      float max = r;
      if (g > max) max = g;
      if (b > max) max = b;
      ratio = 1.0 / max;
   }

   if (ovr)
   {
      r -= 1.0;
      g -= 1.0;
      b -= 1.0;
      r *= ratio;
      g *= ratio;
      b *= ratio;
      r += 1.0;
      g += 1.0;
      b += 1.0;
   }

   r = clamp(r, 0.0, 1.0);
   g = clamp(g, 0.0, 1.0);
   b = clamp(b, 0.0, 1.0);

   return vec3(r, g, b);
}

vec3 ApplyColorimetry(vec3 color)
{
   //http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
   //http://www.dr-lex.be/random/matrix_inv.html
   // Convert the (x,y) values to X Y Z.

   float R = color.r;
   float G = color.g;
   float B = color.b;

   float Rx = 0.0;
   float Ry = 0.0;
   float Gx = 0.0;
   float Gy = 0.0;
   float Bx = 0.0;
   float By = 0.0;
   float Wx = 0.0;
   float Wy = 0.0;

   if (white_point_d93 > 0.5)
   {
      Wx = 0.31;
      Wy = 0.316;
   }
   else
   {
      Wx = 0.3127;
      Wy = 0.329;
   }

   if (color_mode < 0.5)
   {
      // FCC 1953
      // Original FCC standard for the color of the phosphors
      Rx = 0.67;
      Ry = 0.33;
      Gx = 0.21;
      Gy = 0.71;
      Bx = 0.14;
      By = 0.08;
   }
   else if (color_mode == 1.0) 
   {
      // SMPTE C (1987)
      // A newer standard for the color of the phospors. (Not used in Japan)
      Rx = 0.63;
      Ry = 0.34;
      Gx = 0.31;
      Gy = 0.595;
      Bx = 0.155;
      By = 0.07;
   }
   else
   {
      //sRGB (PC Monitors)
      //The colorimetry used in PC monitors, like the one you're (probably) looking at right now.
      Rx = 0.64;
      Ry = 0.33;
      Gx = 0.3;
      Gy = 0.6;
      Bx = 0.15;
      By = 0.06;
   }

   float Xr = Rx / Ry;
   float Xg = Gx / Gy;
   float Xb = Bx / By;
   float Xw = Wx / Wy;
   float Yr = 1.0;
   float Yg = 1.0;
   float Yb = 1.0;
   float Yw = 1.0;
   float Zr = (1.0 - Rx - Ry) / Ry;
   float Zg = (1.0 - Gx - Gy) / Gy;
   float Zb = (1.0 - Bx - By) / By;
   float Zw = (1.0 - Wx - Wy) / Wy;

   // Get ready for a bunch of painful math. I need to invert a matrix, then multiply it by a vector.
   // Determinant for inverse matrix

   float sDet = (Xr*((Zb*Yg)-(Zg*Yb)))-(Yr*((Zb*Xg)-(Zg*Xb)))+(Zr*((Yb*Xg)-(Yg*Xb)));

   float Sr = ((((Zb*Yg)-(Zg*Yb))/sDet)*Xw) + ((-((Zb*Xg)-(Zg*Xb))/sDet)*Yw) + ((((Yb*Xg)-(Yg*Xb))/sDet)*Zw);
   float Sg = ((-((Zb*Yr)-(Zr*Yb))/sDet)*Xw) + ((((Zb*Xr)-(Zr*Xb))/sDet)*Yw) + ((-((Yb*Xr)-(Yr*Xb))/sDet)*Zw);
   float Sb = ((((Zg*Yr)-(Zr*Yg))/sDet)*Xw) + ((-((Zg*Xr)-(Zr*Xg))/sDet)*Yw) + ((((Yg*Xr)-(Yr*Xg))/sDet)*Zw);

   // This should be the completed RGB -> XYZ matrix.
   // Multiply each of the first three members by R, then add them together to get X
   float convMatrix[9] = float[] (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

   convMatrix[0] = Sr*Xr;
   convMatrix[1] = Sg*Xg;
   convMatrix[2] = Sb*Xb;
   convMatrix[3] = Sr*Yr;
   convMatrix[4] = Sg*Yg;
   convMatrix[5] = Sb*Yb;
   convMatrix[6] = Sr*Zr;
   convMatrix[7] = Sg*Zg;
   convMatrix[8] = Sb*Zb;

   // Convert RGB to XYZ using the matrix generated with the specified RGB and W points.	
   float X = (convMatrix[0] * R) + (convMatrix[1] * G) + (convMatrix[2] * B);
   float Y = (convMatrix[3] * R) + (convMatrix[4] * G) + (convMatrix[5] * B);
   float Z = (convMatrix[6] * R) + (convMatrix[7] * G) + (convMatrix[8] * B);

   // This is the conversion matrix for CIEXYZ -> sRGB. I nicked this from:
   // http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html
   // and I know it's right because when you use the sRGB colorimetry, this matrix produces identical results to
   // just using the raw R, G, and B above.
   float xyztorgb[9] = float[] (3.2404, -1.5371, -0.4985, -0.9693, 1.876, 0.0416, 0.0556, -0.204, 1.0572);

   // Convert back to RGB using the XYZ->sRGB matrix.
   R = (xyztorgb[0]*X) + (xyztorgb[1]*Y) + (xyztorgb[2]*Z);
   G = (xyztorgb[3]*X) + (xyztorgb[4]*Y) + (xyztorgb[5]*Z);
   B = (xyztorgb[6]*X) + (xyztorgb[7]*Y) + (xyztorgb[8]*Z);

   vec3 corrected_rgb;

   // Apply desired clipping method to out-of-gamut colors.
   if (clipping_method < 0.5)
   {
      //If a channel is out of range (> 1.0), it's simply clamped to 1.0. This may change hue, saturation, and/or lightness.
      R = clamp(R, 0.0, 1.0);
      G = clamp(G, 0.0, 1.0);
      B = clamp(B, 0.0, 1.0);
      corrected_rgb = vec3(R, G, B);
   }
   else if (clipping_method == 1.0)
   {
      //If any channels are out of range, the color is darkened until it is completely in range.
      corrected_rgb = huePreserveClipDarken(R, G, B);
   }
   else if (clipping_method == 2.0)
   {
      //If any channels are out of range, the color is desaturated towards the luminance it would've had.
      corrected_rgb = huePreserveClipDesaturate(R, G, B);
   }

   return corrected_rgb;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   vec4 c = texture(Source, vTexCoord.xy);
   vec3 rgb = vec3(c.r, c.g, c.b);

   vec3 out_color = ApplyColorimetry(rgb);
   FragColor = vec4(out_color, 1.0);
}
