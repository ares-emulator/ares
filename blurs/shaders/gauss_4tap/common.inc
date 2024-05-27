/*
    Gauss-4tap v1.0 by fishku
    Copyright (C) 2023
    Public domain license (CC0)

    Simple two-pass Gaussian blur filter which does two taps per pass.
    Idea based on:
    https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

    Changelog:
    v1.0: Initial release.
*/

// Finds the offset so that two samples drawn with linear filtering at that
// offset from a central pixel, multiplied with 1/2 each, sum up to a 3-sample
// approximation of the Gaussian sampled at pixel centers.
float get_offset(float sigma) {
    // Weight at x = 0 evaluates to 1 for all values of sigma.
    const float w = exp(-1.0 / (sigma * sigma));
    return 2.0 * w / (2.0 * w + 1.0);
}
