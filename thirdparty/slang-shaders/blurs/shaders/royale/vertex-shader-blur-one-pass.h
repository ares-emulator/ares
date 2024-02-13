#ifndef VERTEX_SHADER_BLUR_ONE_PASS_H
#define VERTEX_SHADER_BLUR_ONE_PASS_H

/////////////////////////////////  MIT LICENSE  ////////////////////////////////

//  Copyright (C) 2014 TroggleMonkey
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.


/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

//  PASS SETTINGS:
//  Pass settings should be set by the shader file that #includes this one.


//////////////////////////////////  INCLUDES  //////////////////////////////////

//#include "../../../include/gamma-management.h"
//#include "../../../include/blur-functions.h"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 tex_uv;
layout(location = 1) out vec2 blur_dxdy;

void main()
{
   gl_Position = global.MVP * Position;
   tex_uv = TexCoord;

	//  Get the uv sample distance between output pixels.  Blurs are not generic
    //  Gaussian resizers, and correct blurs require:
    //  1.) OutputSize.xy == SourceSize.xy * 2^m, where m is an integer <= 0.
    //  2.) mipmap_inputN = "true" for this pass in the preset if m < 0
    //  3.) filter_linearN = "true" for all one-pass blurs
    //  Gaussian resizers would upsize using the distance between input texels
    //  (not output pixels), but we avoid this and consistently blur at the
    //  destination size.  Otherwise, combining statically calculated weights
    //  with bilinear sample exploitation would result in terrible artifacts.
   const vec2 dxdy_scale = params.SourceSize.xy * params.OutputSize.zw;
   blur_dxdy = dxdy_scale * params.SourceSize.zw;
}

#endif  //  VERTEX_SHADER_BLUR_ONE_PASS_H