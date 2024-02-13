
#define kPi    3.1415926536f
#define kEuler 2.718281828459f
#define kMax   1.0f

#define kBeamWidth 0.5f

const vec4 kFallOffControlPoints    = vec4(0.0f, 0.0f, 0.0f, 1.0f);
const vec4 kAttackControlPoints     = vec4(0.0f, 1.0f, 1.0f, 1.0f);
//const vec4 kScanlineControlPoints = vec4(1.0f, 1.0f, 0.0f, 0.0f);

const mat4 kCubicBezier = mat4( 1.0f,  0.0f,  0.0f,  0.0f,
                               -3.0f,  3.0f,  0.0f,  0.0f,
                                3.0f, -6.0f,  3.0f,  0.0f,
                               -1.0f,  3.0f, -3.0f,  1.0f );

float Bezier(const float t0, const vec4 control_points)
{
   vec4 t = vec4(1.0, t0, t0*t0, t0*t0*t0);
   return dot(t, control_points * kCubicBezier);
}

vec4 BeamControlPoints(const float beam_attack, const bool falloff)
{
   const float inner_attack = clamp(beam_attack, 0.0f, 1.0);
   const float outer_attack = clamp(beam_attack - 1.0f, 0.0f, 1.0);

   return falloff ? kFallOffControlPoints + vec4(0.0f, outer_attack, inner_attack, 0.0f) : kAttackControlPoints - vec4(0.0f, inner_attack, outer_attack, 0.0f);
}

float ScanlineColour(const uint channel, 
                     const vec2 tex_coord,
                     const vec2 source_size, 
                     const float scanline_size, 
                     const float source_tex_coord_x, 
                     const float narrowed_source_pixel_offset, 
                     const float vertical_convergence, 
                     const float beam_attack, 
                     const float scanline_min, 
                     const float scanline_max, 
                     const float scanline_attack, 
                     inout float next_prev)
{
   const float current_source_position_y  = ((tex_coord.y * source_size.y) - vertical_convergence) + next_prev;
   const float current_source_center_y    = floor(current_source_position_y) + 0.5f; 
   
   const float source_tex_coord_y         = current_source_center_y / source_size.y; 

   const float scanline_delta             = fract(current_source_position_y) - 0.5f;

   // Slightly increase the beam width to get maximum brightness
   float beam_distance                    = abs(scanline_delta - next_prev) - (kBeamWidth / scanline_size);
   beam_distance                          = beam_distance < 0.0f ? 0.0f : beam_distance;
   const float scanline_distance          = beam_distance * 2.0f;

   next_prev = scanline_delta > 0.0f ? 1.0f : -1.0f;

   const vec2 tex_coord_0                 = vec2(source_tex_coord_x, source_tex_coord_y);
   const vec2 tex_coord_1                 = vec2(source_tex_coord_x + (1.0f / source_size.x), source_tex_coord_y);

   const float sdr_channel_0              = COMPAT_TEXTURE(SourceSDR, tex_coord_0)[channel];
   const float sdr_channel_1              = COMPAT_TEXTURE(SourceSDR, tex_coord_1)[channel];

   const float hdr_channel_0              = COMPAT_TEXTURE(SourceHDR, tex_coord_0)[channel];
   const float hdr_channel_1              = COMPAT_TEXTURE(SourceHDR, tex_coord_1)[channel];

   /* Horizontal interpolation between pixels */ 
   const float horiz_interp               = Bezier(narrowed_source_pixel_offset, BeamControlPoints(beam_attack, sdr_channel_0 > sdr_channel_1));  

   const float hdr_channel                = mix(hdr_channel_0, hdr_channel_1, horiz_interp);
   const float sdr_channel                = mix(sdr_channel_0, sdr_channel_1, horiz_interp);

   const float channel_scanline_distance  = clamp(scanline_distance / ((sdr_channel * (scanline_max - scanline_min)) + scanline_min), 0.0f, 1.0f);

   const vec4 channel_control_points      = vec4(1.0f, 1.0f, sdr_channel * scanline_attack,    0.0f);

   const float luminance                  = Bezier(channel_scanline_distance, channel_control_points);

   return luminance * hdr_channel;
}

float GenerateScanline( const uint channel, 
                        const vec2 tex_coord,
                        const vec2 source_size, 
                        const float scanline_size, 
                        const float horizontal_convergence, 
                        const float vertical_convergence, 
                        const float beam_sharpness, 
                        const float beam_attack, 
                        const float scanline_min, 
                        const float scanline_max, 
                        const float scanline_attack)
{
   const float current_source_position_x      = (tex_coord.x * source_size.x) - horizontal_convergence;
   const float current_source_center_x        = floor(current_source_position_x) + 0.5f; 
   
   const float source_tex_coord_x             = current_source_center_x / source_size.x; 

   const float source_pixel_offset            = fract(current_source_position_x);

   const float narrowed_source_pixel_offset   = clamp(((source_pixel_offset - 0.5f) * beam_sharpness) + 0.5f, 0.0f, 1.0f);

   float next_prev = 0.0f;

   const float scanline_colour0  = ScanlineColour( channel, 
                                                   tex_coord,
                                                   source_size, 
                                                   scanline_size, 
                                                   source_tex_coord_x, 
                                                   narrowed_source_pixel_offset, 
                                                   vertical_convergence,  
                                                   beam_attack, 
                                                   scanline_min, 
                                                   scanline_max, 
                                                   scanline_attack, 
                                                   next_prev);

   // Optionally sample the neighbouring scanline
   float scanline_colour1 = 0.0f;
   if(scanline_max > 1.0f)
   {
      scanline_colour1           = ScanlineColour( channel, 
                                                   tex_coord,
                                                   source_size, 
                                                   scanline_size, 
                                                   source_tex_coord_x, 
                                                   narrowed_source_pixel_offset,
                                                   vertical_convergence,  
                                                   beam_attack, 
                                                   scanline_min, 
                                                   scanline_max, 
                                                   scanline_attack,  
                                                   next_prev);
   }

   return scanline_colour0 + scanline_colour1;
}