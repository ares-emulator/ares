#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
const vec2 madd = vec2(0.5, 0.5);
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = gl_Position.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
float iGlobalTime = float(global.FrameCount)*0.025;
vec2 iResolution = global.OutputSize.xy;

// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// These are 316 gaussian points (201 brushes by using local symmetry) forming the picture
// of Audrey Hepburn. They pack down to 800 bytes (34 bits per point) before huffman/arithmetic
// compression.
//
// The points were chosen by (very quickly) runing this 
//
// http://www.iquilezles.org/www/articles/genetic/genetic.htm
//
// with some importance sampling for the eyes, nouse and mouth.

float brush( float col, vec2 p, in vec4 b, float an )
{
	p += an*cos( iGlobalTime + 100.0*b.yz );
				
    vec2 dd = p - b.yz;
    col = mix( col, b.x, exp( -b.w*b.w*dot(dd,dd) ) );
    if( abs(b.z-0.5)<0.251 )
    {
        dd.x = p.x - 1.0 + b.y;
        col =  mix( col, b.x, exp( -b.w*b.w*dot(dd,dd) ) );
    }
    return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.y;
    uv.x -=  0.5*(iResolution.x / iResolution.y - 1.0);

    float an = smoothstep( 0.0, 1.0, cos(iGlobalTime) );

	float col = 0.0;

    col = brush( col, uv, vec4(1.000,0.371,0.379,11.770), an );
    col = brush( col, uv, vec4(0.992,0.545,0.551,8.359), an );
    col = brush( col, uv, vec4(0.749,0.623,0.990,36.571), an );
    col = brush( col, uv, vec4(1.000,0.510,0.395,11.315), an );
    col = brush( col, uv, vec4(1.000,0.723,0.564,15.170), an );
    col = brush( col, uv, vec4(0.953,0.729,0.750,14.629), an );
    col = brush( col, uv, vec4(0.706,0.982,0.033,16.254), an );
    col = brush( col, uv, vec4(1.000,0.855,0.652,26.256), an );
    col = brush( col, uv, vec4(1.000,0.664,0.623,81.920), an );
    col = brush( col, uv, vec4(0.000,0.881,0.750,8.031), an );
    col = brush( col, uv, vec4(0.686,0.682,0.900,27.676), an );
    col = brush( col, uv, vec4(1.000,0.189,0.684,18.618), an );
    col = brush( col, uv, vec4(0.000,0.904,0.750,8.031), an );
    col = brush( col, uv, vec4(1.000,0.422,0.195,44.522), an );
    col = brush( col, uv, vec4(1.000,0.779,0.750,16.787), an );
    col = brush( col, uv, vec4(1.000,0.645,0.330,14.222), an );
    col = brush( col, uv, vec4(1.000,0.197,0.648,22.505), an );
    col = brush( col, uv, vec4(0.702,0.512,0.393,35.310), an );
    col = brush( col, uv, vec4(1.000,0.744,0.621,14.949), an );
    col = brush( col, uv, vec4(0.671,0.885,0.092,24.675), an );
    col = brush( col, uv, vec4(0.000,0.344,0.750,8.031), an );
    col = brush( col, uv, vec4(1.000,0.760,0.465,40.960), an );
    col = brush( col, uv, vec4(0.008,0.908,0.311,8.031), an );
    col = brush( col, uv, vec4(0.016,0.959,0.750,10.039), an );
    col = brush( col, uv, vec4(0.004,0.930,0.750,12.800), an );
    col = brush( col, uv, vec4(1.000,0.555,0.250,19.883), an );
    col = brush( col, uv, vec4(1.000,0.770,1.018,15.876), an );
    col = brush( col, uv, vec4(0.000,0.828,0.756,36.571), an );
    col = brush( col, uv, vec4(0.580,0.566,0.424,89.043), an );
    col = brush( col, uv, vec4(0.988,0.162,0.691,40.157), an );
    col = brush( col, uv, vec4(0.000,0.314,0.750,8.031), an );
    col = brush( col, uv, vec4(0.000,0.947,0.125,32.000), an );
    col = brush( col, uv, vec4(0.914,0.844,0.725,52.513), an );
    col = brush( col, uv, vec4(1.000,0.313,0.762,42.667), an );
    col = brush( col, uv, vec4(0.996,0.676,0.689,85.333), an );
    col = brush( col, uv, vec4(0.980,0.346,0.559,24.675), an );
    col = brush( col, uv, vec4(1.000,0.553,0.250,18.789), an );
    col = brush( col, uv, vec4(0.004,0.258,0.248,8.031), an );
    col = brush( col, uv, vec4(1.000,0.420,0.742,30.567), an );
    col = brush( col, uv, vec4(0.906,0.543,0.250,22.756), an );
    col = brush( col, uv, vec4(0.863,0.674,0.322,20.078), an );
    col = brush( col, uv, vec4(0.753,0.357,0.686,78.769), an );
    col = brush( col, uv, vec4(0.906,0.795,0.705,37.236), an );
    col = brush( col, uv, vec4(0.933,0.520,0.365,38.642), an );
    col = brush( col, uv, vec4(0.996,0.318,0.488,14.734), an );
    col = brush( col, uv, vec4(0.337,0.486,0.281,81.920), an );
    col = brush( col, uv, vec4(0.965,0.691,0.516,16.650), an );
    col = brush( col, uv, vec4(0.808,0.582,0.973,52.513), an );
    col = brush( col, uv, vec4(0.012,0.240,0.928,8.063), an );
    col = brush( col, uv, vec4(1.000,0.496,0.217,31.508), an );
    col = brush( col, uv, vec4(0.000,0.658,0.953,34.133), an );
    col = brush( col, uv, vec4(0.871,0.582,0.172,62.061), an );
    col = brush( col, uv, vec4(0.855,0.346,0.342,17.504), an );
    col = brush( col, uv, vec4(0.878,0.787,0.648,28.845), an );
    col = brush( col, uv, vec4(0.000,0.984,0.111,35.310), an );
    col = brush( col, uv, vec4(0.855,0.514,0.965,66.065), an );
    col = brush( col, uv, vec4(0.561,0.613,0.350,81.920), an );
    col = brush( col, uv, vec4(0.992,0.818,0.902,21.558), an );
    col = brush( col, uv, vec4(0.914,0.746,0.615,40.157), an );
    col = brush( col, uv, vec4(0.557,0.580,0.125,60.235), an );
    col = brush( col, uv, vec4(0.475,0.547,0.414,70.621), an );
    col = brush( col, uv, vec4(0.843,0.680,0.793,20.277), an );
    col = brush( col, uv, vec4(1.000,0.230,0.758,56.889), an );
    col = brush( col, uv, vec4(1.000,0.299,0.691,68.267), an );
    col = brush( col, uv, vec4(0.737,0.518,0.100,68.267), an );
    col = brush( col, uv, vec4(0.996,0.227,0.514,41.796), an );
    col = brush( col, uv, vec4(0.929,0.850,0.770,62.061), an );
    col = brush( col, uv, vec4(0.682,0.834,0.111,30.118), an );
    col = brush( col, uv, vec4(0.996,0.854,0.793,58.514), an );
    col = brush( col, uv, vec4(0.490,0.736,0.889,19.321), an );
    col = brush( col, uv, vec4(0.980,0.465,0.725,16.126), an );
    col = brush( col, uv, vec4(0.992,0.484,1.010,23.273), an );
    col = brush( col, uv, vec4(0.008,0.949,0.727,23.540), an );
    col = brush( col, uv, vec4(0.012,0.086,0.086,8.031), an );
    col = brush( col, uv, vec4(1.000,0.121,0.750,44.522), an );
    col = brush( col, uv, vec4(0.427,0.617,0.891,27.676), an );
    col = brush( col, uv, vec4(0.804,0.693,0.633,78.769), an );
    col = brush( col, uv, vec4(0.012,0.711,0.084,13.745), an );
    col = brush( col, uv, vec4(0.082,0.584,0.338,107.789), an );
    col = brush( col, uv, vec4(0.929,0.613,0.268,19.692), an );
    col = brush( col, uv, vec4(0.200,0.549,0.420,128.000), an );
    col = brush( col, uv, vec4(1.000,0.402,0.717,26.947), an );
    col = brush( col, uv, vec4(0.000,0.551,0.168,45.511), an );
    col = brush( col, uv, vec4(0.992,0.627,0.621,56.889), an );
    col = brush( col, uv, vec4(0.902,0.361,0.748,40.960), an );
    col = brush( col, uv, vec4(0.984,0.344,0.754,38.642), an );
    col = brush( col, uv, vec4(0.902,0.203,0.818,51.200), an );
    col = brush( col, uv, vec4(1.000,0.230,0.803,52.513), an );
    col = brush( col, uv, vec4(0.922,0.738,0.691,47.628), an );
    col = brush( col, uv, vec4(0.000,0.385,0.797,43.574), an );
    col = brush( col, uv, vec4(0.000,0.725,0.305,62.061), an );
    col = brush( col, uv, vec4(0.000,0.150,0.750,45.511), an );
    col = brush( col, uv, vec4(1.000,0.742,0.408,47.628), an );
    col = brush( col, uv, vec4(0.000,0.645,0.643,60.235), an );
    col = brush( col, uv, vec4(1.000,0.645,0.438,35.310), an );
    col = brush( col, uv, vec4(0.510,0.564,0.789,18.450), an );
    col = brush( col, uv, vec4(0.863,0.211,0.781,30.567), an );
    col = brush( col, uv, vec4(0.106,0.508,0.328,89.043), an );
    col = brush( col, uv, vec4(0.012,0.410,0.875,14.629), an );
    col = brush( col, uv, vec4(1.000,0.871,0.877,48.762), an );
    col = brush( col, uv, vec4(1.000,0.258,0.779,37.926), an );
    col = brush( col, uv, vec4(0.000,0.436,0.807,28.845), an );
    col = brush( col, uv, vec4(0.918,0.861,0.836,49.951), an );
    col = brush( col, uv, vec4(1.000,0.291,0.770,40.960), an );
    col = brush( col, uv, vec4(0.000,0.750,0.283,27.676), an );
    col = brush( col, uv, vec4(0.965,0.596,0.572,28.055), an );
    col = brush( col, uv, vec4(0.902,0.803,0.953,24.976), an );
    col = brush( col, uv, vec4(0.957,0.498,0.600,16.126), an );
    col = brush( col, uv, vec4(0.914,0.322,0.432,15.634), an );
    col = brush( col, uv, vec4(0.008,0.025,0.621,17.809), an );
    col = brush( col, uv, vec4(0.000,0.916,0.713,56.889), an );
    col = brush( col, uv, vec4(0.914,0.547,0.971,47.628), an );
    col = brush( col, uv, vec4(0.000,0.207,0.432,37.926), an );
    col = brush( col, uv, vec4(0.875,0.176,0.793,46.545), an );
    col = brush( col, uv, vec4(0.000,0.646,0.668,41.796), an );
    col = brush( col, uv, vec4(1.000,0.721,0.691,51.200), an );
    col = brush( col, uv, vec4(0.451,0.559,0.754,49.951), an );
    col = brush( col, uv, vec4(0.969,0.846,0.750,58.514), an );
    col = brush( col, uv, vec4(0.000,0.900,0.146,36.571), an );
    col = brush( col, uv, vec4(1.000,0.613,0.635,85.333), an );
    col = brush( col, uv, vec4(0.596,0.807,0.150,58.514), an );
    col = brush( col, uv, vec4(0.898,0.330,0.760,40.157), an );
    col = brush( col, uv, vec4(0.694,0.594,0.012,51.200), an );
    col = brush( col, uv, vec4(0.698,0.592,0.055,53.895), an );
    col = brush( col, uv, vec4(0.902,0.268,0.773,39.385), an );
    col = brush( col, uv, vec4(0.925,0.838,0.660,58.514), an );
    col = brush( col, uv, vec4(0.843,0.670,0.242,28.444), an );
    col = brush( col, uv, vec4(0.243,0.465,0.285,85.333), an );
    col = brush( col, uv, vec4(0.816,0.588,0.674,44.522), an );
    col = brush( col, uv, vec4(0.008,0.283,0.115,8.031), an );
    col = brush( col, uv, vec4(0.247,0.414,0.691,60.235), an );
    col = brush( col, uv, vec4(1.000,0.104,0.781,60.235), an );
    col = brush( col, uv, vec4(0.000,0.619,0.660,60.235), an );
    col = brush( col, uv, vec4(0.584,0.650,0.994,46.545), an );
    col = brush( col, uv, vec4(0.000,0.219,0.393,36.571), an );
    col = brush( col, uv, vec4(1.000,0.307,0.645,97.524), an );
    col = brush( col, uv, vec4(0.953,0.639,0.771,38.642), an );
    col = brush( col, uv, vec4(0.000,0.238,0.357,34.712), an );
    col = brush( col, uv, vec4(0.922,0.713,0.352,53.895), an );
    col = brush( col, uv, vec4(0.965,0.387,0.748,43.574), an );
    col = brush( col, uv, vec4(0.000,0.898,0.633,41.796), an );
    col = brush( col, uv, vec4(0.941,0.352,0.488,14.734), an );
    col = brush( col, uv, vec4(0.933,0.439,0.725,30.567), an );
    col = brush( col, uv, vec4(0.310,0.541,0.906,47.628), an );
    col = brush( col, uv, vec4(0.941,0.502,0.689,24.094), an );
    col = brush( col, uv, vec4(0.094,0.527,0.330,85.333), an );
    col = brush( col, uv, vec4(0.000,0.090,0.688,55.351), an );
    col = brush( col, uv, vec4(0.000,0.652,0.713,75.852), an );
    col = brush( col, uv, vec4(0.949,0.320,0.623,107.789), an );
    col = brush( col, uv, vec4(0.890,0.775,0.750,22.505), an );
    col = brush( col, uv, vec4(0.012,0.918,0.490,14.322), an );
    col = brush( col, uv, vec4(1.000,0.871,0.967,58.514), an );
    col = brush( col, uv, vec4(0.000,0.324,0.676,64.000), an );
    col = brush( col, uv, vec4(0.008,0.141,0.248,8.031), an );
    col = brush( col, uv, vec4(0.000,0.633,0.707,75.852), an );
    col = brush( col, uv, vec4(0.910,0.385,0.207,44.522), an );
    col = brush( col, uv, vec4(0.012,0.703,0.182,31.508), an );
    col = brush( col, uv, vec4(0.000,0.617,0.703,73.143), an );
    col = brush( col, uv, vec4(0.890,0.352,0.225,45.511), an );
    col = brush( col, uv, vec4(0.933,0.826,0.604,44.522), an );
    col = brush( col, uv, vec4(0.914,0.777,0.574,25.924), an );
    col = brush( col, uv, vec4(0.631,0.781,0.182,68.267), an );
    col = brush( col, uv, vec4(1.000,0.873,0.916,48.762), an );
    col = brush( col, uv, vec4(0.694,0.520,0.113,81.920), an );
    col = brush( col, uv, vec4(0.000,0.900,0.926,58.514), an );
    col = brush( col, uv, vec4(0.184,0.598,0.344,146.286), an );
    col = brush( col, uv, vec4(0.863,0.678,0.250,35.310), an );
    col = brush( col, uv, vec4(0.090,0.566,0.332,78.769), an );
    col = brush( col, uv, vec4(0.420,0.445,0.301,56.889), an );
    col = brush( col, uv, vec4(0.973,0.617,0.516,18.124), an );
    col = brush( col, uv, vec4(0.000,0.191,0.500,39.385), an );
    col = brush( col, uv, vec4(0.000,0.240,0.326,31.508), an );
    col = brush( col, uv, vec4(0.000,0.264,0.322,55.351), an );
    col = brush( col, uv, vec4(0.000,0.604,0.699,70.621), an );
    col = brush( col, uv, vec4(0.000,0.113,0.604,43.574), an );
    col = brush( col, uv, vec4(0.894,0.760,0.697,49.951), an );
    col = brush( col, uv, vec4(0.914,0.725,0.383,55.351), an );
    col = brush( col, uv, vec4(0.000,0.199,0.467,48.762), an );
    col = brush( col, uv, vec4(0.000,0.904,0.660,52.513), an );
    col = brush( col, uv, vec4(0.922,0.611,0.191,45.511), an );
    col = brush( col, uv, vec4(0.059,0.789,0.869,30.118), an );
    col = brush( col, uv, vec4(0.976,0.641,0.213,40.960), an );
    col = brush( col, uv, vec4(0.918,0.402,0.742,47.628), an );
    col = brush( col, uv, vec4(0.945,0.717,0.582,40.157), an );
    col = brush( col, uv, vec4(0.000,0.299,0.672,58.514), an );
    col = brush( col, uv, vec4(0.000,0.719,0.666,48.762), an );
    col = brush( col, uv, vec4(0.882,0.697,0.271,58.514), an );
    col = brush( col, uv, vec4(0.929,0.752,0.436,64.000), an );
    col = brush( col, uv, vec4(1.000,0.867,0.813,56.889), an );
    col = brush( col, uv, vec4(0.643,0.588,0.090,64.000), an );
    col = brush( col, uv, vec4(0.012,0.063,0.922,10.952), an );
    col = brush( col, uv, vec4(0.878,0.186,0.750,31.508), an );
    col = brush( col, uv, vec4(0.953,0.648,0.613,120.471), an );
    col = brush( col, uv, vec4(0.973,0.180,0.576,45.511), an );
    col = brush( col, uv, vec4(0.741,0.943,0.076,52.513), an );
    col = brush( col, uv, vec4(0.059,0.545,0.332,89.043), an );
    col = brush( col, uv, vec4(0.094,0.295,0.734,85.333), an );
    col = brush( col, uv, vec4(0.008,0.676,0.721,85.333), an );
    col = brush( col, uv, vec4(0.550,0.350,0.650,85.000), an );

	fragColor = vec4(col,col,col,1.0);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
