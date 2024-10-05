// See compose.slang for copyright and other information.

// clang-format off
#pragma parameter AVERAGE_FILL_SETTINGS "=== Average fill v1.9 settings ===" 0.0 0.0 1.0 1.0

#include "../../../misc/shaders/input_transform/parameters.inc"

#pragma parameter OTHER_SETTINGS "= Other parameters =" 0.0 0.0 1.0 1.0
#pragma parameter EXTEND_H "Extend the fill horizontally" 1.0 0.0 1.0 1.0
#pragma parameter EXTEND_V "Extend the fill vertically" 1.0 0.0 1.0 1.0

#pragma parameter CORNER_BLEND_MODE "Cropped corner blend mode" 0.0 0.0 3.0 1.0

#pragma parameter FILL_GAMMA "Background fill gamma adjustment" 1.0 0.5 2.0 0.1

#pragma parameter SAMPLE_SIZE "No. of lines for calculating the average" 8.0 1.0 64.0 1.0
// clang-format on
