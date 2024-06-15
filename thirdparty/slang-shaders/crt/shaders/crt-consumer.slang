#version 450

/*

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OutputSize;
    uint FrameCount;
    float rg;
    float rb;
    float gr;
    float gb;
    float br;
    float bg;
    float Downscale;
    float quality;
    float palette_fix;
} params;

// Parameter lines go here:
#pragma parameter PRE_SCALE "Pre-Scale Sharpening" 1.5 1.0 4.0 0.1
#pragma parameter blurx "Convergence X" 0.25 -4.0 4.0 0.05
#pragma parameter blury "Convergence Y" -0.1 -4.0 4.0 0.05
#pragma parameter warpx "  Curvature X" 0.03 0.0 0.12 0.01
#pragma parameter warpy "  Curvature Y" 0.04 0.0 0.12 0.01
#pragma parameter corner "  Corner size" 0.03 0.0 0.10 0.01
#pragma parameter smoothness "  Border Smoothness" 400.0 100.0 600.0 5.0
#pragma parameter inter "Interlacing Toggle" 1.0 0.0 1.0 1.0
#pragma parameter Downscale "Interlacing Downscale Scanlines" 2.0 1.0 8.0 1.0  
#pragma parameter scanlow "Beam low" 6.0 1.0 15.0 1.0
#pragma parameter scanhigh "Beam high" 8.0 1.0 15.0 1.0
#pragma parameter beamlow "Scanlines dark" 1.45 0.5 2.5 0.05 
#pragma parameter beamhigh "Scanlines bright" 1.05 0.5 2.5 0.05 
#pragma parameter preserve "  Protect White On Masks" 0.98 0.0 1.0 0.01
#pragma parameter brightboost1 "  Bright boost dark pixels" 1.25 0.0 3.0 0.05
#pragma parameter brightboost2 "  Bright boost bright pixels" 1.0 0.0 3.0 0.05
#pragma parameter glow "  Glow pixels per axis" 3.0 1.0 6.0 1.0
#pragma parameter quality "  Glow quality" 1.0 0.25 4.0 0.05
#pragma parameter glow_str "  Glow intensity" 0.3 0.0001 2.0 0.05
#pragma parameter nois "  Add Noise" 0.0 0.0 32.0 1.0
#pragma parameter postbr "  Post Brightness" 1.0 0.0 2.5 0.02
#pragma parameter palette_fix "Palette Fixes. Sega, PUAE Atari ST dark colors " 0.0 0.0 2.0 1.0
#pragma parameter Shadowmask "Mask Type" 0.0 -1.0 8.0 1.0 
#pragma parameter masksize "Mask Size" 1.0 1.0 2.0 1.0
#pragma parameter MaskDark "Mask dark" 0.2 0.0 2.0 0.1
#pragma parameter MaskLight "Mask light" 1.5 0.0 2.0 0.1
#pragma parameter slotmask "Slot Mask Strength" 0.0 0.0 1.0 0.05
#pragma parameter slotwidth "Slot Mask Width" 2.0 1.0 6.0 0.5
#pragma parameter double_slot "Slot Mask Height: 2x1 or 4x1" 1.0 1.0 2.0 1.0
#pragma parameter slotms "Slot Mask Size" 1.0 1.0 2.0 1.0
#pragma parameter GAMMA_OUT "  Gamma Out" 2.25 0.0 4.0 0.05
#pragma parameter sat "  Saturation" 1.0 0.0 2.0 0.05
#pragma parameter contrast "  Contrast, 1.0:Off" 1.0 0.00 2.00 0.05
#pragma parameter WP "  Color Temperature %" 0.0 -100.0 100.0 5.0 
#pragma parameter rg             "  Red-Green Tint"       0.0 -1.0 1.0 0.005
#pragma parameter rb             "  Red-Blue Tint"        0.0 -1.0 1.0 0.005
#pragma parameter gr             "  Green-Red Tint"       0.0 -1.0 1.0 0.005
#pragma parameter gb             "  Green-Blue Tint"      0.0 -1.0 1.0 0.005
#pragma parameter br             "  Blue-Red Tint"        0.0 -1.0 1.0 0.005
#pragma parameter bg             "  Blue-Green Tint"      0.0 -1.0 1.0 0.005
#pragma parameter vignette "Vignette On/Off" 0.0 0.0 1.0 1.0
#pragma parameter vpower "Vignette Power" 0.15 0.0 1.0 0.01
#pragma parameter vstr "Vignette strength" 40.0 0.0 50.0 1.0
#pragma parameter alloff "Switch off shader" 0.0 0.0 1.0 1.0

#define rg              params.rg
#define rb              params.rb
#define gr              params.gr
#define gb              params.gb
#define br              params.br
#define bg              params.bg
#define Downscale       params.Downscale
layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    float blurx;
    float blury;
    float warpx;
    float warpy;
    float corner;
    float smoothness;
    float scanlow;
    float scanhigh;
    float beamlow;
    float beamhigh;
    float brightboost1;
    float brightboost2;
    float Shadowmask; 
    float masksize;
    float MaskDark;
    float MaskLight;
    float slotmask;
    float slotwidth;
    float double_slot;
    float slotms;
    float GAMMA_OUT;
    float glow;
    float glow_str;
    float sat;
    float contrast;
    float nois;
    float WP; 
    float inter; 
    float vignette;
    float vpower;
    float vstr;
    float alloff;
    float postbr;
    float PRE_SCALE;
    float preserve;
} global;
 
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

#define iTime (float(params.FrameCount) / 2.0)
#define iTimer (float(params.FrameCount) / 60.0)

vec2 Warp(vec2 pos)
{
    pos  = pos * 2.0 - 1.0;    
    pos *= vec2(1.0 + (pos.y * pos.y) * global.warpx, 1.0 + (pos.x * pos.x) * global.warpy);
    return pos * 0.5 + 0.5;
} 

float sw(float y, float l)
{
    float beam = mix(global.scanlow, global.scanhigh, y);
    float scan = mix(global.beamlow,  global.beamhigh, l);
    float ex = y * scan;
    return exp2(-beam * ex * ex);
}

vec3 mask(vec2 x, vec3 col, float l)
{
    x = floor(x / global.masksize);        
  
    if (global.Shadowmask == 0.0)
    {
        float m = fract(x.x * 0.4999);
        if (m < 0.4999) return vec3(1.0,             global.MaskDark, 1.0);
        else            return vec3(global.MaskDark, 1.0,             global.MaskDark);
    }
   
    else if (global.Shadowmask == 1.0)
    {
        vec3 Mask = vec3(global.MaskDark, global.MaskDark, global.MaskDark);
        float line = global.MaskLight;
        float odd  = 0.0;

        if (fract(x.x / 6.0) < 0.5) odd = 1.0;
        if (fract((x.y + odd) / 2.0) < 0.5) line = global.MaskDark;

        float m = fract(x.x / 3.0);
        if      (m < 0.333) Mask.b = global.MaskLight;
        else if (m < 0.666) Mask.g = global.MaskLight;
        else                Mask.r = global.MaskLight;
        
        Mask *= line; 
        return Mask; 
    } 
    
    else if (global.Shadowmask == 2.0)
    {
        float m = fract(x.x*0.3333);
        if (m < 0.3333) return vec3(global.MaskDark,  global.MaskDark,  global.MaskLight);
        if (m < 0.6666) return vec3(global.MaskDark,  global.MaskLight, global.MaskDark);
        else            return vec3(global.MaskLight, global.MaskDark,  global.MaskDark);
    }

    if (global.Shadowmask == 3.0)
    {
        float m = fract(x.x * 0.5);
        if (m < 0.5) return vec3(1.0, 1.0, 1.0);
        else         return vec3(global.MaskDark, global.MaskDark, global.MaskDark);
    }
   
    else if (global.Shadowmask == 4.0)
    {   
        vec3 Mask = vec3(col.rgb);
        float line = global.MaskLight;
        float odd  = 0.0;

        if (fract(x.x / 4.0) < 0.5) odd = 1.0;
        if (fract((x.y + odd) / 2.0) < 0.5) line = global.MaskDark;

        float m = fract(x.x / 2.0);
        if  (m < 0.5) { Mask.r = 1.0; Mask.b = 1.0; }
        else  Mask.g = 1.0;   

        Mask *= line;  
        return Mask;
    } 

    else if (global.Shadowmask == 5.0)
    {
        vec3 Mask = vec3(1.0, 1.0, 1.0);

        if (fract(x.x / 4.0) < 0.5)   
        {
            if (fract(x.y / 3.0) < 0.666)
            {
                if (fract(x.x / 2.0) < 0.5) Mask = vec3(1.0,             global.MaskDark, 1.0);
                else                        Mask = vec3(global.MaskDark, 1.0,             global.MaskDark);
            }
            else Mask *= l;
        }
        else if (fract(x.x / 4.0) >= 0.5)   
        {
            if (fract(x.y / 3.0) > 0.333) 
            {
                if (fract(x.x / 2.0) < 0.5) Mask = vec3(1.0,             global.MaskDark, 1.0); 
                else                        Mask = vec3(global.MaskDark, 1.0,             global.MaskDark);
            }
            else Mask *= l;
        }

        return Mask;
    }

    else if (global.Shadowmask == 6.0)
    {
        vec3 Mask = vec3(global.MaskDark, global.MaskDark, global.MaskDark);
        if (fract(x.x / 6.0) < 0.5)   
        {
            if (fract(x.y / 4.0) < 0.75)  
            {
                if      (fract(x.x / 3.0) < 0.3333) Mask.r = global.MaskLight; 
                else if (fract(x.x / 3.0) < 0.6666) Mask.g = global.MaskLight; 
                else                                Mask.b = global.MaskLight;
            }
            else Mask * l * 0.9;
        }
        else if (fract(x.x / 6.0) >= 0.5)   
        {
            if (fract(x.y / 4.0) >= 0.5 || fract(x.y / 4.0) < 0.25)  
            {
                if      (fract(x.x / 3.0) < 0.3333) Mask.r = global.MaskLight; 
                else if (fract(x.x / 3.0) < 0.6666) Mask.g = global.MaskLight;
                else                                Mask.b = global.MaskLight;
            }
            else Mask * l * 0.9;
        }
        return Mask;
    }

    else if (global.Shadowmask == 7.0)
    {
        float m = fract(x.x * 0.3333);

        if (m < 0.3333) return vec3(global.MaskDark,          global.MaskLight,         global.MaskLight * col.b); //Cyan
        if (m < 0.6666) return vec3(global.MaskLight * col.r, global.MaskDark,          global.MaskLight);         //Magenta
        else            return vec3(global.MaskLight,         global.MaskLight * col.g, global.MaskDark);          //Yellow
    }

    else if (global.Shadowmask == 8.0)
    {
        vec3 Mask = vec3(global.MaskDark, global.MaskDark, global.MaskDark);

        float bright = global.MaskLight;
        float left   = 0.0;
        if (fract(x.x / 6.0) < 0.5) left = 1.0;
             
        float m = fract(x.x / 3.0);
        if      (m < 0.333) Mask.b = 0.9;
        else if (m < 0.666) Mask.g = 0.9;
        else                Mask.r = 0.9;
        
        if (mod(x.y, 2.0) == 1.0 && left == 1.0 || mod(x.y, 2.0) == 0.0 && left == 0.0) 
            Mask *= bright; 
      
        return Mask; 
    } 
    
    else return vec3(1.0, 1.0, 1.0);
}

float SlotMask(vec2 pos, vec3 c)
{
    if (global.slotmask == 0.0) return 1.0;
    
    pos = floor(pos / global.slotms);
    float mx = pow(max(max(c.r, c.g), c.b), 1.33);
    float mlen = global.slotwidth * 2.0;
    float px = fract(pos.x / mlen);
    float py = floor(fract(pos.y / (2.0 * global.double_slot)) * 2.0 * global.double_slot);
    float slot_dark = mix(1.0 - global.slotmask, 1.0 - 0.80 * global.slotmask, mx);
    float slot = 1.0 + 0.7 * global.slotmask * (1.0 - mx);
    
    if      (py == 0.0                && px <  0.5) slot = slot_dark; 
    else if (py == global.double_slot && px >= 0.5) slot = slot_dark;       
    
    return slot;
}

mat4 contrastMatrix(float contrast)
{   
    float t = (1.0 - global.contrast) / 2.0;
    
    return mat4(global.contrast, 0,               0,               0,
                0,               global.contrast, 0,               0,
                0,               0,               global.contrast, 0,
                t,               t,               t,               1);
}

mat3 vign(float l)
{
    vec2 vpos = vTexCoord;
    vpos *= 1.0 - vpos.xy;
    
    float vig = vpos.x * vpos.y * global.vstr;
    vig = min(pow(vig, global.vpower), 1.0); 
    if (global.vignette == 0.0) vig = 1.0;
   
    return mat3(vig, 0,   0,
                0,   vig, 0,
                0,   0,   vig);
}

vec3 saturation(vec3 textureColor)
{
    float luminance = length(textureColor.rgb) * 0.5775;

    vec3 luminanceWeighting = vec3(0.4, 0.5, 0.1);
    if (luminance < 0.5) luminanceWeighting.rgb = (luminanceWeighting.rgb * luminanceWeighting.rgb) 
                                                + (luminanceWeighting.rgb * luminanceWeighting.rgb);

    luminance = dot(textureColor.rgb, luminanceWeighting);
    vec3 greyScaleColor = vec3(luminance, luminance, luminance);

    vec3 res = vec3(mix(greyScaleColor, textureColor.rgb, global.sat));
    return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

vec3 glow0 (vec2 texcoord, vec3 col)
{

   // the more params.quality, the smaller the offset and better quality, less visible glow too
     vec2 size = params.SourceSize.zw/params.quality;
     
     vec3 c01;
     vec3 sum = vec3(0.0);
   
   // global.glow = pixels per axis, the more the slower!

    for (float x = -global.glow; x <= global.glow; x = x+1.0)
     {

   // multiply texture, the more far away the less pronounced
        float factor = 1.0/global.glow;
        for (float y = -global.glow; y <= global.glow; y = y+1.0)
        {

        vec2 offset = vec2(x, y) * size;

         c01 = texture(Source, texcoord + offset).rgb*factor; c01 = c01*c01;
          
                sum += c01;
        }
    }
  
    return (global.glow_str * sum / (global.glow * global.glow )) ;
}
    
///////////////////////////////////////////////////////////////////////////////////////////////////////////

float noise(vec2 co)
{
    return fract(sin(iTimer * dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float corner0(vec2 coord)
{
    coord = (coord - vec2(0.5, 0.5)) * 1.0 + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0, 1.0) - coord) * vec2(1.0, params.SourceSize.y / params.SourceSize.x);
    
    vec2 cdist = vec2(global.corner, global.corner);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));

    return clamp((cdist.x - dist) * global.smoothness, 0.0, 1.0);
}  

const mat3 D65_to_XYZ = mat3(
           0.4306190,  0.2220379,  0.0201853,
           0.3415419,  0.7066384,  0.1295504,
           0.1783091,  0.0713236,  0.9390944);

const mat3 XYZ_to_D65 = mat3(
           3.0628971, -0.9692660,  0.0678775,
          -1.3931791,  1.8760108, -0.2288548,
          -0.4757517,  0.0415560,  1.0693490);
           
const mat3 D50_to_XYZ = mat3(
           0.4552773,  0.2323025,  0.0145457,
           0.3675500,  0.7077956,  0.1049154,
           0.1413926,  0.0599019,  0.7057489);
           
const mat3 XYZ_to_D50 = mat3(
           2.9603944, -0.9787684,  0.0844874,
          -1.4678519,  1.9161415, -0.2545973,
          -0.4685105,  0.0334540,  1.4216174);         




void main()
{
    vec2 pos = Warp(vTexCoord.xy);
    vec2 tex_size = params.SourceSize.xy;  

    vec2 pC4 = (pos + 0.5/tex_size);
    vec2 fp = fract(pos * tex_size);
    if (global.inter < 0.5 && tex_size.y > 400.0){ fp.y = fract(pos.y * tex_size.y*1.0/Downscale);} 

    vec4 res = vec4(1.0);
    
    if (global.alloff == 1.0) 
        res = texture(Source, pC4); 
    else
    {

   vec2 texel = pos * tex_size;
   vec2 texel_floored = floor(texel);

   float scale = global.PRE_SCALE;
   float region_range = 0.5 - 0.5 / scale;

   // Figure out where in the texel to sample to get correct pre-scaled bilinear.
   // Uses the hardware bilinear interpolator to avoid having to sample 4 times manually.

   vec2 center_dist = fp - 0.5;

   vec2 fpp = (center_dist - clamp(center_dist, -region_range, region_range)) * scale + 0.5;

   vec2 mod_texel = texel_floored + fpp;
   vec2 coords = mod_texel / params.SourceSize.xy;

        vec3 sample1 = texture(Source, vec2(coords.x + global.blurx*params.SourceSize.z, coords.y - global.blury*params.SourceSize.w)).rgb;
        vec3 sample2 = texture(Source, coords).rgb;
        vec3 sample3 = texture(Source, vec2(coords.x - global.blurx*params.SourceSize.z, coords.y + global.blury*params.SourceSize.w )).rgb;
        
        vec3 color = vec3(sample1.r * 0.5  + sample2.r * 0.5, 
                          sample1.g * 0.25 + sample2.g * 0.5 + sample3.g * 0.25,
                          sample2.b * 0.5  + sample3.b * 0.5);
        if (params.palette_fix != 0.0) 
        {
            if (params.palette_fix == 1.0) color = color* 1.0667;
            else if (params.palette_fix == 2.0) color = color * 2.0;
        }

        //COLOR TEMPERATURE FROM GUEST.R-DR.VENOM
        if (global.WP != 0.0)
        {
            vec3 warmer = D50_to_XYZ * color;
            warmer = XYZ_to_D65 * warmer; 
            
            vec3 cooler = D65_to_XYZ * color;
            cooler = XYZ_to_D50 * cooler;
            
            float m = abs(global.WP) / 100.0;
            vec3 comp = (global.WP < 0.0) ? cooler : warmer;
            comp = clamp(comp, 0.0, 1.0);   
            
            color = vec3(mix(color, comp, m));
        }

     mat3 hue = mat3 (1., rg,  rb,                 //red tint
                      gr,  1., gb,                  //green tint
                      br,  bg,  1.);                //blue tint

        color = hue * color;

        color = (2.0*pow(color,vec3(2.8))) - pow(color,vec3(3.6));

        float lum = color.r * 0.3 + color.g * 0.6 + color.b * 0.1;

        float f = fract(fp.y -0.5);
        
        if (global.inter > 0.5 && tex_size.y > 400.0) color = color; 
        else
        {color = color * sw(f,lum) + color * sw (1.0-f,lum);}
        
        float lum1 = color.r * 0.3 + color.g * 0.6 + color.b * 0.1;

        
        color *= mix(mask((vTexCoord * params.OutputSize.xy), color,lum1), vec3(1.0), lum1*global.preserve);
        

        if (global.slotmask != 0.0) color *= SlotMask((vTexCoord * params.OutputSize.xy) * 1.0001, color);
        
        color *= mix(global.brightboost1, global.brightboost2, max(max(color.r, color.g), color.b));    

    

        color = pow(color,vec3(1.0 / global.GAMMA_OUT));
                if (global.glow_str != 0.0) color += glow0(coords,color);

        if (global.sat    != 1.0) color  = saturation(color);
        if (global.corner != 0.0) color *= corner0(pC4);
        if (global.nois   != 0.0) color *= 1.0 + noise(coords * 2.0) / global.nois;

        color *= mix(1.0, global.postbr, lum);
        res = vec4(color, 1.0);
        if (global.contrast != 1.0) res = contrastMatrix(global.contrast) * res;
        if (global.inter > 0.5 && params.SourceSize.y > 400.0 && fract(iTime) < 0.5) res = res * 0.95;
        res.rgb *= vign(lum);

    }
    
    FragColor = res;
}
