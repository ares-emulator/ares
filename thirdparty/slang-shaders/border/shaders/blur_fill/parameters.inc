// See compose.slang for copyright and other information.

// clang-format off
#pragma parameter BLUR_FILL_SETTINGS "=== Blur fill v1.10 settings ===" 0.0 0.0 1.0 1.0

#include "../../../misc/shaders/input_transform/parameters.inc"

#pragma parameter OTHER_SETTINGS "= Other parameters =" 0.0 0.0 1.0 1.0
#pragma parameter EXTEND_H "Extend the fill horizontally" 0.0 0.0 1.0 1.0
#pragma parameter EXTEND_V "Extend the fill vertically" 0.0 0.0 1.0 1.0

#pragma parameter MIRROR_BLUR "Mirror the blur" 0.0 0.0 1.0 1.0

#pragma parameter FILL_GAMMA "Background fill gamma adjustment" 1.4 0.5 2.0 0.1

#pragma parameter SAMPLE_SIZE "No. of lines for rendering the blur" 16.0 1.0 1024.0 1.0
// clang-format on
