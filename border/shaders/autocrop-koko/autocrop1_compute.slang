#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////
//
//  autocrop-koko
//  Copyright (C) 2024 Antonio Orefice <kokoko3k@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 3 of the License, or any later version.
//
////////////////////////////////////////////////////////////////////////////////

#include "autocrop_config.inc"

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
layout(set = 0, binding = 2) uniform sampler2D autocrop_precut;
layout(set = 0, binding = 3) uniform sampler2D autocrop_precutFeedback;
layout(set = 0, binding = 4) uniform sampler2D autocrop_computeFeedback;


#define RGB2GRAY_VEC3 vec3(0.299, 0.587, 0.114)
float rgb_to_gray(vec3 rgb) {
    return dot(rgb, RGB2GRAY_VEC3);
}

bool scene_changed( sampler2D pre_smp, sampler2D cur_smp, float threshold) {
    
    vec3 pre_mip = textureLod( pre_smp, vec2(0.5), 20.0).rgb;
    vec3 cur_mip = textureLod( cur_smp, vec2(0.5), 20.0).rgb;
                
    float pre_v = rgb_to_gray(pre_mip);
    float cur_v = rgb_to_gray(cur_mip);

    float diff_v = cur_v - pre_v;
    
    // fadein: if the previous image is almost black and 
    // image is fading in, trigger a scene scange 
    if (pre_v < 2.0/255.0)
        if (pre_v < cur_v)
            return true;
    
    float diff_v_abs = abs(diff_v);
        
    // if lum is different enough trigger scene change
    if (diff_v_abs >= threshold)
        return true;
     
    /*
    // from blank screen to something else, trigger SC
    // This compares different lod mipmaps from the same samplig point
    // to guess if the screen is solid or not.
    vec3 pre_mip_l20 = pre_mip;    
    vec3 pre_mip_l1 =  textureLod( pre_smp, vec2(0.5), vLods.x).rgb;
    vec3 pre_mip_l2 =  textureLod( pre_smp, vec2(0.5), vLods.y).rgb;

    if ( pre_mip_l1 == pre_mip_l2 && pre_mip_l2 == pre_mip_l20 )
        return true;
    */  
    
    return false;
}

float quasirandom(float n, float dmin, float dmax) {
	// https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
	// Returns a number between dmin and dmax which would ideally
	// fills the range by iterating through n values.
    // n has to be integer for this to work.
	float x = fract(n*0.754877626895905);
	return ( (dmax-dmin) * x ) + dmin;
}

float get_next_autocrop_amount(float max_autocrop_amount) {
   
    //Implementation requires the line_step to be the same for vertical and horizontal sampling.
    //This could be a bit inefficient tho.
        const float size_max = max(params.OriginalSize.x,params.OriginalSize.y) ;
        const float next_line_step = 1/size_max;
        
    //This is needed to avoid sampling edges which may return out out of range values.
        const vec2 d = params.OriginalSize.zw * 0.5 * (1+AUTOCROP_MIN*2);
    //Reference color, topleft.
        vec3 ref = textureLod(autocrop_precut, vec2(d), 0.0).rgb; // FIXME, ref sample needs to be fuzzy too?
        float ref_lum = ref.r + ref.g + ref.b ;
    
    //convert zoom back to maximum cropped lines
        const float max_crop_lines = ( size_max * (max_autocrop_amount -1) )  / ( 2*max_autocrop_amount) ;
    
    //Sample top,bottom,left,right lines; stop when a line sum contain a different pixel color than the reference
    for (float croppedlines = AUTOCROP_MIN ; croppedlines < max_crop_lines ; croppedlines++) {
        
        vec3 smp_sum;
        float line_lum;
        float fuzzy = AUTOCROP_SAMPLE_SIZE;
        
        //Top
        smp_sum = vec3(0.0);
        for ( float x = params.FrameCount ; x < params.FrameCount+AUTOCROP_SAMPLES ;  x ++ ) {
            smp_sum += textureLod(autocrop_precut, vec2( quasirandom(x, d.x, 1-d.x ), 0.0+ next_line_step*croppedlines), fuzzy).rgb;
        }
        
        line_lum = (smp_sum.r + smp_sum.g + smp_sum.b) / (AUTOCROP_SAMPLES);
        
        if ( abs(ref_lum - line_lum) > AUTOCROP_TOLERANCE)
            return size_max / (size_max - (croppedlines) * 2) ;

        //BOTTOM
        smp_sum = vec3(0.0);
        for ( float x = params.FrameCount ; x < params.FrameCount+AUTOCROP_SAMPLES ; x ++ )  {   
            smp_sum += textureLod(autocrop_precut, vec2(  quasirandom(x, d.x, 1-d.x)  , (1.0-eps)-next_line_step*croppedlines), fuzzy).rgb;
        }

        line_lum = (smp_sum.r + smp_sum.g + smp_sum.b) / (AUTOCROP_SAMPLES);
        
        if (abs( ref_lum - line_lum) > AUTOCROP_TOLERANCE) {
            return size_max / (size_max - (croppedlines) * 2) ;
        }
        
        //LEFT
        smp_sum = vec3(0.0);
        for ( float y = params.FrameCount; y < params.FrameCount+AUTOCROP_SAMPLES; y ++ ) {
            smp_sum += textureLod(autocrop_precut, vec2(0.0+next_line_step*croppedlines, quasirandom(y, d.y, 1-d.y)), fuzzy).rgb;
        }
        
        line_lum = (smp_sum.r + smp_sum.g + smp_sum.b) / (AUTOCROP_SAMPLES);

        if (abs( ref_lum - line_lum) > AUTOCROP_TOLERANCE)
            return size_max / (size_max - (croppedlines) * 2)     ;

        //RIGHT
        smp_sum = vec3(0.0);
        for ( float y = params.FrameCount; y < params.FrameCount+AUTOCROP_SAMPLES; y ++ ) {
            smp_sum += textureLod(autocrop_precut, vec2((1.0-eps)-next_line_step*croppedlines, quasirandom(y, d.y, 1-d.y)), fuzzy).rgb;
        }

        line_lum = (smp_sum.r + smp_sum.g + smp_sum.b) / (AUTOCROP_SAMPLES);

        if (abs( ref_lum - line_lum) > AUTOCROP_TOLERANCE)
            return size_max / (size_max - (croppedlines) * 2);

    }

    return max_autocrop_amount;
}

float get_autocrop() {
    // This return a value that will be used  in the final pass to zoom the picture.
    float previous_autocrop_amount = AUTOCROP_MAX+1;
    if (params.FrameCount < 30.0) {
        previous_autocrop_amount =  AUTOCROP_MAX+1; //Good start point.
    } else {
        previous_autocrop_amount = texture(autocrop_computeFeedback, AUTOCROP_SAMPLING_POINT).a;
    }

    // Reset crop if scene has changed?
    float next_autocrop_amount;
    
    if (scene_changed( autocrop_precutFeedback, autocrop_precut, AUTOCROP_STEADINESS) ) {
        //When a scene changes, we must release the maximum crop amount and find a new one.
        
        //For entrance effect:
            //return AUTOCROP_MAX+1; //"entrance effect?
        
        //No frills way:
            next_autocrop_amount = get_next_autocrop_amount(AUTOCROP_MAX+1);
            return next_autocrop_amount;
			
    } else {
		
        previous_autocrop_amount = texture(autocrop_computeFeedback, AUTOCROP_SAMPLING_POINT).a;
        next_autocrop_amount = get_next_autocrop_amount(previous_autocrop_amount);
        float r =  mix( previous_autocrop_amount , next_autocrop_amount, AUTOCROP_TRANSITION_SPEED);
        return clamp(r, 1.0, AUTOCROP_MAX+1); // needed to sanitize output when Feedback is unavailable (eg: just switched autocrop on)
    }
}

bool is_first_inside_rect(vec2 point, vec4 rect) {
	vec2 bounded = clamp(point, rect.xy, rect.zw);
	return point == bounded;
}

void main() {
	if (is_first_inside_rect(vTexCoord, vec4(AUTOCROP_SAMPLING_POINT-0.01, AUTOCROP_SAMPLING_POINT+0.01)))
        if (AUTOCROP_MAX > 0.001)
            FragColor.a = get_autocrop();
}