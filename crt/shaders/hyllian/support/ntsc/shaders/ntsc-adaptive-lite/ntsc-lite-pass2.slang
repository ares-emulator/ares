#version 450

// NTSC-Adaptive-Lite
// based on Themaister's NTSC shader


layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   float linearize;
   float ntsc_scale;
   float ntsc_phase;
   float auto_res;
   float chroma_scale;
} global;

#pragma parameter chroma_scale "NTSC Chroma Scaling" 1.0 0.2 4.0 0.1 
#pragma parameter ntsc_scale "NTSC Resolution Scaling" 1.0 0.20 3.0 0.05
#pragma parameter ntsc_phase "NTSC Phase: Auto | 2 phase | 3 phase" 1.0 1.0 3.0 1.0
#pragma parameter linearize "NTSC Linearize Output Gamma" 0.0 0.0 1.0 1.0 

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord - vec2(0.5 / global.SourceSize.x, 0.0); // Compensate for decimate-by-2.
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
//layout(set = 0, binding = 3) uniform sampler2D orig;

vec3 fetch_offset(float offset, vec2 one_x)
{
   /* Insert chroma scaling. Thanks to guest.r ideas. */

   vec3 yiq;

   yiq.x  = texture(Source, vTexCoord + vec2((offset) * (one_x.x), 0.0)).x;
   yiq.yz = texture(Source, vTexCoord + vec2((offset) * (one_x.y), 0.0)).yz;

   return yiq;

/*  Old code
   return texture(Source, vTexCoord + vec2((offset) * (one_x), 0.0)).xyz;
*/
}

const mat3 yiq2rgb_mat = mat3(
   1.0, 0.956, 0.6210,
   1.0, -0.2720, -0.6474,
   1.0, -1.1060, 1.7046);

vec3 yiq2rgb(vec3 yiq)
{
   return yiq * yiq2rgb_mat;
}

const mat3 yiq_mat = mat3(
      0.2989, 0.5870, 0.1140,
      0.5959, -0.2744, -0.3216,
      0.2115, -0.5229, 0.3114
);

vec3 rgb2yiq(vec3 col)
{
   return col * yiq_mat;
}


/* These are accurate and normalized coeffs. */
const int TAPS_3_phase = 24;
const float luma_filter_3_phase[25] = float[25](
-0.0000120203033684164,
-0.0000221465589348544,
-0.0000131553320142694,
-0.0000120203033684164,
-0.0000499802614018372,
-0.000113942875690297,
-0.000122153082899506,
-5.61214E-06,
0.000170520303591422,
0.000237204986579451,
0.000169644281482376,
0.000285695210375719,
0.000984598849305758,
0.0020187339488074,
0.00200232553469184,
-0.000909904964181485,
-0.00704925890919635,
-0.0132231937269633,
-0.0126072491817548,
0.00246092210875218,
0.0358691302651096,
0.0840185734607569,
0.135566921437963,
0.175265691355518,
0.190181351796957);

/* These are accurate and normalized coeffs. */
const float chroma_filter_3_phase[25] = float[25](
-0.000135741056915795,
-0.000568115749081878,
-0.00130605691082327,
-0.00231369942971182,
-0.00350569685928248,
-0.00474731062446688,
-0.00585980203774502,
-0.00663114046295865,
-0.00683148404964774,
-0.00623234997205773,
-0.00462792764511295,
-0.00185665431957684,
0.00217899013894782,
0.00749647783836479,
0.0140227874371299,
0.021590863169257,
0.0299437436530477,
0.0387464461271303,
0.0476049759842373,
0.0560911497485196,
0.0637713405314321,
0.0702368383153846,
0.0751333078160781,
0.0781868487834974,
0.0792244191487085);


/* These are accurate and normalized coeffs. Though they don't produce ideal smooth vertical lines transparency. */
const int TAPS_2_phase = 15;
const float luma_filter_2_phase[16] = float[16](
0.00134372867555492,
0.00294231678339247,
0.00399617683765551,
0.00303632635732925,
-0.00110556727614119,
-0.00839970341605087,
-0.0169515379999301,
-0.0229874881474188,
-0.0217113019865528,
-0.00889151239892142,
0.0173269874254282,
0.0550969075027442,
0.098655909675851,
0.139487291941771,
0.168591277052964,
0.17914037794465);


/* 

Use these coeffs if you want smoother transparencies (vertical lines genesis dithering).

const int TAPS_2_phase = 15;
const float luma_filter_2_phase[16] = float[16](
0.00358674124928555,
0.00516185579845568,
0.00480461826123886,
0.00132260688600227,
-0.00546931642681805,
-0.014242227285043,
-0.0221097992725951,
-0.025171977770877,
-0.0195669333236999,
-0.00275082885707516,
0.0253995033240388,
0.0620582992351114,
0.101744778642171,
0.137437283264631,
0.162237083549672,
0.171116625451004);

*/

/* These are accurate and normalized coeffs. */
const float chroma_filter_2_phase[16] = float[16](
0.00406084767413046,
0.00578573638571078,
0.00804447474387669,
0.0109152541019797,
0.0144533032717188,
0.0186765858322351,
0.0235518468184291,
0.0289834149989225,
0.034807373222651,
0.0407934139180355,
0.0466558344725586,
0.0520737649339226,
0.0567190701585739,
0.0602887575746322,
0.0625375226221969,
0.0633055985408521);


void main()
{
   float res = global.ntsc_scale;
   float OriginalSize = global.OriginalSize.x;
   vec3 signal = vec3(0.0);
   float phase = (global.ntsc_phase < 1.5) ? ((OriginalSize > 300.0) ? 2.0 : 3.0) : ((global.ntsc_phase > 2.5) ? 3.0 : 2.0);

   float chroma_scale = phase > 2.5 ? min(global.chroma_scale, 2.2) : global.chroma_scale/2.0;
   vec2 one_x = (global.SourceSize.z / res) * vec2(1.0, 1.0 / chroma_scale);


   if(phase < 2.5)
   {
      vec3 sums = fetch_offset(0.0 - 15.0, one_x) + fetch_offset(15.0 - 0.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[0], chroma_filter_2_phase[0], chroma_filter_2_phase[0]);
      sums = fetch_offset(1.0 - 15.0, one_x) + fetch_offset(15.0 - 1.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[1], chroma_filter_2_phase[1], chroma_filter_2_phase[1]);
      sums = fetch_offset(2.0 - 15.0, one_x) + fetch_offset(15.0 - 2.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[2], chroma_filter_2_phase[2], chroma_filter_2_phase[2]);
      sums = fetch_offset(3.0 - 15.0, one_x) + fetch_offset(15.0 - 3.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[3], chroma_filter_2_phase[3], chroma_filter_2_phase[3]);
      sums = fetch_offset(4.0 - 15.0, one_x) + fetch_offset(15.0 - 4.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[4], chroma_filter_2_phase[4], chroma_filter_2_phase[4]);
      sums = fetch_offset(5.0 - 15.0, one_x) + fetch_offset(15.0 - 5.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[5], chroma_filter_2_phase[5], chroma_filter_2_phase[5]);
      sums = fetch_offset(6.0 - 15.0, one_x) + fetch_offset(15.0 - 6.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[6], chroma_filter_2_phase[6], chroma_filter_2_phase[6]);
      sums = fetch_offset(7.0 - 15.0, one_x) + fetch_offset(15.0 - 7.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[7], chroma_filter_2_phase[7], chroma_filter_2_phase[7]);
      sums = fetch_offset(8.0 - 15.0, one_x) + fetch_offset(15.0 - 8.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[8], chroma_filter_2_phase[8], chroma_filter_2_phase[8]);
      sums = fetch_offset(9.0 - 15.0, one_x) + fetch_offset(15.0 - 9.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[9], chroma_filter_2_phase[9], chroma_filter_2_phase[9]);
      sums = fetch_offset(10.0 - 15.0, one_x) + fetch_offset(15.0 - 10.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[10], chroma_filter_2_phase[10], chroma_filter_2_phase[10]);
      sums = fetch_offset(11.0 - 15.0, one_x) + fetch_offset(15.0 - 11.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[11], chroma_filter_2_phase[11], chroma_filter_2_phase[11]);
      sums = fetch_offset(12.0 - 15.0, one_x) + fetch_offset(15.0 - 12.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[12], chroma_filter_2_phase[12], chroma_filter_2_phase[12]);
      sums = fetch_offset(13.0 - 15.0, one_x) + fetch_offset(15.0 - 13.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[13], chroma_filter_2_phase[13], chroma_filter_2_phase[13]);
      sums = fetch_offset(14.0 - 15.0, one_x) + fetch_offset(15.0 - 14.0, one_x);
      signal += sums * vec3(luma_filter_2_phase[14], chroma_filter_2_phase[14], chroma_filter_2_phase[14]);
      
      signal += texture(Source, vTexCoord).xyz *
         vec3(luma_filter_2_phase[TAPS_2_phase], chroma_filter_2_phase[TAPS_2_phase], chroma_filter_2_phase[TAPS_2_phase]);
   }
   else if(phase > 2.5)
   {
      for (int i = 0; i < TAPS_3_phase; i++)
      {
         float offset = float(i);

         vec3 sums = fetch_offset(offset - float(TAPS_3_phase), one_x) +
            fetch_offset(float(TAPS_3_phase) - offset, one_x);
         signal += sums * vec3(luma_filter_3_phase[i], chroma_filter_3_phase[i], chroma_filter_3_phase[i]);
      }
      signal += texture(Source, vTexCoord).xyz *
         vec3(luma_filter_3_phase[TAPS_3_phase], chroma_filter_3_phase[TAPS_3_phase], chroma_filter_3_phase[TAPS_3_phase]);
   }

   vec3 rgb = yiq2rgb(signal);
   FragColor = vec4(rgb, 1.0);
   if(global.linearize < 0.5) return;
   else FragColor = pow(FragColor, vec4(2.2));
}  
