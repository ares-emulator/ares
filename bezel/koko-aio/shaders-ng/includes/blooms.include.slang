#define bdirections 8

vec2 radius_blur_adapt(vec2 bsize){
    vec2 r;
    r.x = bsize.x / SCALEMOD_X;
    r.y = r.x *  get_in_aspect();
    return r;
}

vec3 bloom_gamma_power(sampler2D smp, vec2 uv, vec4 smpsize, vec2 bsize, float quality, float gamma, float power, float start_offset, float lod) {
    vec2 r = radius_blur_adapt(bsize) ;
    vec3 lookup;
    vec3 color  = vec3(0.0);
    vec3 vec3gamma = vec3(gamma);

    for( float d=start_offset; d<TAU; d+=TAU/bdirections) {
        for(float i=1.0/quality; i<=1.0; i+=1.0/quality) {
            lookup = textureLod(smp, uv + vec2( cos(d), sin(d)) * r * i, lod ).rgb ;
            color +=pow(lookup.rgb, vec3gamma);
        }
    }
    color /= (quality*bdirections);
    return color * power;
}

vec3 bloom_gamma(sampler2D smp, vec2 uv, vec4 smpsize, vec2 bsize, float quality, float gamma, float start_offset, float lod) {
    vec2 r = radius_blur_adapt(bsize);
    vec3 lookup;
    vec3 color  = vec3(0.0);
    vec3 vec3gamma = vec3(gamma);

    for( float d=start_offset; d<TAU; d+=TAU/bdirections) {
        for(float i=1.0/quality; i<=1.0; i+=1.0/quality) {
            lookup = textureLod(smp, uv + vec2( cos(d), sin(d)) * r * i, lod ).rgb ;
            color +=pow(lookup.rgb, vec3gamma);
        }
    }
    color /= (quality*bdirections);
    return color;
}

vec3 bloom(sampler2D smp, vec2 uv, vec4 smpsize, vec2 bsize, float quality, float start_offset, float lod) {
    vec2 r = radius_blur_adapt(bsize);
    vec3 lookup;
    vec3 color  = vec3(0.0);
    for( float d=start_offset; d<TAU; d+=TAU/bdirections) {
        for(float i=1.0/quality; i<=1.0; i+=1.0/quality) {
            lookup = textureLod(smp, uv + vec2( cos(d), sin(d)) * r * i, lod ).rgb ;
            color += lookup.rgb;
        }
    }
    color /= (quality*bdirections);
    return color;
}
