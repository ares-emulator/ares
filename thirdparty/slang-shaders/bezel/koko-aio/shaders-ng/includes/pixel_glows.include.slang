




/*
 * This commented version is a blur that blurs only brighter colors.
 * I'm not using it, because a cheaper method is available in in_glow.slang
 * Very, very little visual differences but a small penalty in performance
 * so i prefer that one.
 * 
 * vec3 brighter(vec3 first, vec3 second){
    vec3 pixel_out;
    pixel_out.r = ((first.r - second.r));
    pixel_out.g = ((first.g - second.g));
    pixel_out.b = ((first.b - second.b));
    return clamp(pixel_out, IN_GLOW_BIAS,1.0);
}

vec4 pixel_glow_std_pow_bloom_only(sampler2D in_texture, float my_glowpix, float my_glowpixy, float my_glow, float glow_gamma, vec2 coords, vec4 SourceSize1, vec4 SourceSize2) {

    // Calculating texel coordinates
    vec2 tex = coords.xy * 1.00001;

    vec2 OGL2Pos = tex * SourceSize2.xy - vec2(0.5,0.5); // vec2 OGL2Pos = tex * SourceSize2.xy - vec2(0.5,0.5); // vec2 OGL2Pos = tex * SourceSize1.xy - vec2(0.5,0.5);  
    vec2 fp = fract(OGL2Pos);
    vec2 dx = vec2(SourceSize1.z,0.0) ; // vec2 dx = vec2(params.passthrough_and_cc_passSize.z,0.0) ; // 
    vec2 dy = vec2(0.0,SourceSize1.w) ; // vec2 dy = vec2(0.0,params.passthrough_and_cc_passSize.w);  //
    vec2 pC4 = (floor(OGL2Pos) + vec2(0.5)) * SourceSize2.zw;

    vec2 x2 = 2.0*dx; vec2 x3 = 3.0*dx;
    vec2 y2 = 2.0*dy;

    vec3 glow_gamma_vec3 = vec3(glow_gamma);
    
    my_glowpix = my_glowpix / scalemod_x * 2 ; // <-- Make it not dependant on the input resolution.
    my_glowpixy = my_glowpixy / scalemod_y *2 ;

    float wl3 = 2.0 + fp.x; wl3*=wl3; wl3 = exp2(-my_glowpix*wl3);
    float wl2 = 1.0 + fp.x; wl2*=wl2; wl2 = exp2(-my_glowpix*wl2);
    float wl1 =       fp.x; wl1*=wl1; wl1 = exp2(-my_glowpix*wl1);
    float wr1 = 1.0 - fp.x; wr1*=wr1; wr1 = exp2(-my_glowpix*wr1);
    float wr2 = 2.0 - fp.x; wr2*=wr2; wr2 = exp2(-my_glowpix*wr2);
    float wr3 = 3.0 - fp.x; wr3*=wr3; wr3 = exp2(-my_glowpix*wr3);

    float wt = 1.0/(wl3+wl2+wl1+wr1+wr2+wr3);

    vec3 center_pixel =  texture(in_texture, coords ).rgb;
    
    
    vec3 l3 = texture(in_texture, pC4 -x2 ).rgb;
    vec3 l2 = texture(in_texture, pC4 -dx ).rgb;
    vec3 l1 = texture(in_texture, pC4     ).rgb;
    vec3 r1 = texture(in_texture, pC4 +dx ).rgb;
    vec3 r2 = texture(in_texture, pC4 +x2 ).rgb;
    vec3 r3 = texture(in_texture, pC4 +x3 ).rgb;
    
                l3 = mix(center_pixel, l3, brighter(l3, center_pixel));
                l2 = mix(center_pixel, l2, brighter(l2, center_pixel));
                l1 = mix(center_pixel, l1, brighter(l1, center_pixel));
                r1 = mix(center_pixel, r1, brighter(r1, center_pixel));
                r2 = mix(center_pixel, r2, brighter(r2, center_pixel));
                r3 = mix(center_pixel, r3, brighter(r3, center_pixel));
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 t1 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    l3 = texture(in_texture, pC4 -x2 -dy).rgb;
    l2 = texture(in_texture, pC4 -dx -dy).rgb;
    l1 = texture(in_texture, pC4     -dy).rgb;
    r1 = texture(in_texture, pC4 +dx -dy).rgb;
    r2 = texture(in_texture, pC4 +x2 -dy).rgb;
    r3 = texture(in_texture, pC4 +x3 -dy).rgb;
    
                l3 = mix(center_pixel, l3, brighter(l3, center_pixel));
                l2 = mix(center_pixel, l2, brighter(l2, center_pixel));
                l1 = mix(center_pixel, l1, brighter(l1, center_pixel));
                r1 = mix(center_pixel, r1, brighter(r1, center_pixel));
                r2 = mix(center_pixel, r2, brighter(r2, center_pixel));
                r3 = mix(center_pixel, r3, brighter(r3, center_pixel));
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 t2 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    l3 = texture(in_texture, pC4 -x2 +dy).rgb;
    l2 = texture(in_texture, pC4 -dx +dy).rgb;
    l1 = texture(in_texture, pC4     +dy).rgb;
    r1 = texture(in_texture, pC4 +dx +dy).rgb;
    r2 = texture(in_texture, pC4 +x2 +dy).rgb;
    r3 = texture(in_texture, pC4 +x3 +dy).rgb;
    
                l3 = mix(center_pixel, l3, brighter(l3, center_pixel));
                l2 = mix(center_pixel, l2, brighter(l2, center_pixel));
                l1 = mix(center_pixel, l1, brighter(l1, center_pixel));
                r1 = mix(center_pixel, r1, brighter(r1, center_pixel));
                r2 = mix(center_pixel, r2, brighter(r2, center_pixel));
                r3 = mix(center_pixel, r3, brighter(r3, center_pixel));
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 b1 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    l3 = texture(in_texture, pC4 -x2 +y2).rgb;
    l2 = texture(in_texture, pC4 -dx +y2).rgb;
    l1 = texture(in_texture, pC4     +y2).rgb;
    r1 = texture(in_texture, pC4 +dx +y2).rgb;
    r2 = texture(in_texture, pC4 +x2 +y2).rgb;
    r3 = texture(in_texture, pC4 +x3 +y2).rgb;
    
                l3 = mix(center_pixel, l3, brighter(l3, center_pixel));
                l2 = mix(center_pixel, l2, brighter(l2, center_pixel));
                l1 = mix(center_pixel, l1, brighter(l1, center_pixel));
                r1 = mix(center_pixel, r1, brighter(r1, center_pixel));
                r2 = mix(center_pixel, r2, brighter(r2, center_pixel));
                r3 = mix(center_pixel, r3, brighter(r3, center_pixel));
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 b2 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    wl2 = 1.0 + fp.y; wl2*=wl2; wl2 = exp2(-my_glowpixy*wl2);
    wl1 =       fp.y; wl1*=wl1; wl1 = exp2(-my_glowpixy*wl1);
    wr1 = 1.0 - fp.y; wr1*=wr1; wr1 = exp2(-my_glowpixy*wr1);
    wr2 = 2.0 - fp.y; wr2*=wr2; wr2 = exp2(-my_glowpixy*wr2);

    wt = 1.0/(wl2+wl1+wr1+wr2);

    vec3 Bloom = (t2*wl2 + t1*wl1 + b1*wr1 + b2*wr2)*wt;

    return vec4(Bloom*my_glow,1.0);
}
*/




vec4 pixel_glow_std_pow(sampler2D in_texture, float my_glowpix, float my_glowpixy, float my_glow, float glow_gamma, vec2 coords, vec4 SourceSize1, vec4 SourceSize2) {

    // Calculating texel coordinates
    vec2 tex = coords.xy * 1.00001;

    vec2 OGL2Pos = tex * SourceSize2.xy - vec2(0.5,0.5); // vec2 OGL2Pos = tex * SourceSize2.xy - vec2(0.5,0.5); // vec2 OGL2Pos = tex * SourceSize1.xy - vec2(0.5,0.5);  
    vec2 fp = fract(OGL2Pos);
    vec2 dx = vec2(SourceSize1.z,0.0) ; // vec2 dx = vec2(params.passthrough_and_cc_passSize.z,0.0) ; // 
    vec2 dy = vec2(0.0,SourceSize1.w) ; // vec2 dy = vec2(0.0,params.passthrough_and_cc_passSize.w);  //
    vec2 pC4 = (floor(OGL2Pos) + vec2(0.5)) * SourceSize2.zw;

    vec2 x2 = 2.0*dx; vec2 x3 = 3.0*dx;
    vec2 y2 = 2.0*dy;

    vec3 glow_gamma_vec3 = vec3(glow_gamma);
    
    my_glowpix = my_glowpix / scalemod_x * 2 ; // <-- Make it not dependant on the input resolution.
    my_glowpixy = my_glowpixy / scalemod_y *2 ;

    float wl3 = 2.0 + fp.x; wl3*=wl3; wl3 = exp2(-my_glowpix*wl3);
    float wl2 = 1.0 + fp.x; wl2*=wl2; wl2 = exp2(-my_glowpix*wl2);
    float wl1 =       fp.x; wl1*=wl1; wl1 = exp2(-my_glowpix*wl1);
    float wr1 = 1.0 - fp.x; wr1*=wr1; wr1 = exp2(-my_glowpix*wr1);
    float wr2 = 2.0 - fp.x; wr2*=wr2; wr2 = exp2(-my_glowpix*wr2);
    float wr3 = 3.0 - fp.x; wr3*=wr3; wr3 = exp2(-my_glowpix*wr3);

    float wt = 1.0/(wl3+wl2+wl1+wr1+wr2+wr3);

    vec3 l3 = texture(in_texture, pC4 -x2 ).rgb;
    vec3 l2 = texture(in_texture, pC4 -dx ).rgb;
    vec3 l1 = texture(in_texture, pC4     ).rgb;
    vec3 r1 = texture(in_texture, pC4 +dx ).rgb;
    vec3 r2 = texture(in_texture, pC4 +x2 ).rgb;
    vec3 r3 = texture(in_texture, pC4 +x3 ).rgb;
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 t1 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    l3 = texture(in_texture, pC4 -x2 -dy).rgb;
    l2 = texture(in_texture, pC4 -dx -dy).rgb;
    l1 = texture(in_texture, pC4     -dy).rgb;
    r1 = texture(in_texture, pC4 +dx -dy).rgb;
    r2 = texture(in_texture, pC4 +x2 -dy).rgb;
    r3 = texture(in_texture, pC4 +x3 -dy).rgb;
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 t2 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    l3 = texture(in_texture, pC4 -x2 +dy).rgb;
    l2 = texture(in_texture, pC4 -dx +dy).rgb;
    l1 = texture(in_texture, pC4     +dy).rgb;
    r1 = texture(in_texture, pC4 +dx +dy).rgb;
    r2 = texture(in_texture, pC4 +x2 +dy).rgb;
    r3 = texture(in_texture, pC4 +x3 +dy).rgb;
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 b1 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    l3 = texture(in_texture, pC4 -x2 +y2).rgb;
    l2 = texture(in_texture, pC4 -dx +y2).rgb;
    l1 = texture(in_texture, pC4     +y2).rgb;
    r1 = texture(in_texture, pC4 +dx +y2).rgb;
    r2 = texture(in_texture, pC4 +x2 +y2).rgb;
    r3 = texture(in_texture, pC4 +x3 +y2).rgb;
    
    l3=pow(l3,glow_gamma_vec3); l2=pow(l2,glow_gamma_vec3); l1=pow(l1,glow_gamma_vec3);
    r1=pow(r1,glow_gamma_vec3); r2=pow(r2,glow_gamma_vec3); r3=pow(r3,glow_gamma_vec3);

    vec3 b2 = (l3*wl3 + l2*wl2 + l1*wl1 + r1*wr1 + r2*wr2 + r3*wr3)*wt;

    wl2 = 1.0 + fp.y; wl2*=wl2; wl2 = exp2(-my_glowpixy*wl2);
    wl1 =       fp.y; wl1*=wl1; wl1 = exp2(-my_glowpixy*wl1);
    wr1 = 1.0 - fp.y; wr1*=wr1; wr1 = exp2(-my_glowpixy*wr1);
    wr2 = 2.0 - fp.y; wr2*=wr2; wr2 = exp2(-my_glowpixy*wr2);

    wt = 1.0/(wl2+wl1+wr1+wr2);

    vec3 Bloom = (t2*wl2 + t1*wl1 + b1*wr1 + b2*wr2)*wt;

    return vec4(Bloom*my_glow,1.0);
}

/*vec4 dumb_glow(sampler2D in_texture, float my_glow, float glow_gamma, vec2 coords) {
    vec3 pixel_in = texture(in_texture, coords ).rgb;
    pixel_in = pow(pixel_in,vec3(glow_gamma))*my_glow;
    return vec4(pixel_in,0.0);
}
*/






vec4 pixel_glow(sampler2D in_texture, float my_glowpix, float my_glowpixy, float my_glow, float glow_gamma, vec2 coords, vec4 SourceSize1, vec4 SourceSize2) {

    
    if ( (my_glowpix >= GLOW_SHARP_MAX) && (my_glowpixy >= GLOW_SHARP_MAX) ) {
        return dumb_glow(in_texture, my_glow,glow_gamma,coords);
    }

    return pixel_glow_std_pow(in_texture,my_glowpix,my_glowpixy,my_glow,glow_gamma,coords,SourceSize1, SourceSize2);


}
 
