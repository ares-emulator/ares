/*
    Dual Filter Blur & Bloom v1.1 by fishku
    Copyright (C) 2023
    Public domain license (CC0)

    The dual filter blur implementation follows the notes of the SIGGRAPH 2015 talk here:
    https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf
    Dual filtering is a fast large-radius blur that approximates a Gaussian blur. It is closely
    related to the popular blur filter by Kawase, but runs faster at equal quality.

    How it works: Any number of downsampling passes are chained with the same number of upsampling
    passes in an hourglass configuration. Both types of resampling passes exploit bilinear
    interpolation with carefully chosen coordinates and weights to produce a smooth output.
    There are just 5 + 8 = 13 texture samples per combined down- and upsampling pass.
    The effective blur radius increases with the number of passes.

    This implementation adds a configurable blur strength which can diminish or accentuate the
    effect compared to the reference implementation, equivalent to strength 1.0.
    A blur strength above 3.0 may lead to artifacts, especially on presets with fewer passes.

    The bloom filter applies a thresholding operation, then blurs the input to varying degrees.
    The scene luminance is estimated using a feedback pass with variable update speed.
    The final pass screen blends a tonemapped bloom value with the original input, with the bloom
    intensity controlled by the scene luminance (a.k.a. eye adaption).

    Changelog:
    v1.1: Added bloom functionality.
    v1.0: Initial release.
*/

vec3 downsample(sampler2D tex, vec2 coord, vec2 offset) {
    // The offset should be 1 source pixel size which equals 0.5 output pixel sizes in the default
    // configuration.
    return (texture(tex, coord - offset).rgb +                     //
            texture(tex, coord + vec2(offset.x, -offset.y)).rgb +  //
            texture(tex, coord).rgb * 4.0 +                        //
            texture(tex, coord + offset).rgb +                     //
            texture(tex, coord - vec2(offset.x, -offset.y)).rgb) *
           0.125;
}

vec3 upsample(sampler2D tex, vec2 coord, vec2 offset) {
    // The offset should be 0.5 source pixel sizes which equals 1 output pixel size in the default
    // configuration.
    return (texture(tex, coord + vec2(0.0, -offset.y * 2.0)).rgb +
            (texture(tex, coord + vec2(-offset.x, -offset.y)).rgb +
             texture(tex, coord + vec2(offset.x, -offset.y)).rgb) *
                2.0 +
            texture(tex, coord + vec2(-offset.x * 2.0, 0.0)).rgb +
            texture(tex, coord + vec2(offset.x * 2.0, 0.0)).rgb +
            (texture(tex, coord + vec2(-offset.x, offset.y)).rgb +
             texture(tex, coord + vec2(offset.x, offset.y)).rgb) *
                2.0 +
            texture(tex, coord + vec2(0.0, offset.y * 2.0)).rgb) /
           12.0;
}

