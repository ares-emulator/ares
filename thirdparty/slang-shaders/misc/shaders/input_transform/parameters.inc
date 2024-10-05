// clang-format off
#pragma parameter SCALING_SETTINGS "= Scaling parameters =" 0.0 0.0 1.0 1.0
#pragma parameter FORCE_ASPECT_RATIO "Force aspect ratio" 1.0 0.0 1.0 1.0
#pragma parameter ASPECT_H "Horizontal aspect ratio before crop (0 = unchanged)" 0.0 0.0 256.0 1.0
#pragma parameter ASPECT_V "Vertical aspect ratio before crop (0 = unchanged)" 0.0 0.0 256.0 1.0
#pragma parameter FORCE_INTEGER_SCALING_H "Force integer scaling horizontally" 0.0 0.0 1.0 1.0
#pragma parameter FORCE_INTEGER_SCALING_V "Force integer scaling vertically" 0.0 0.0 1.0 1.0

#pragma parameter OVERSCALE "Overscale (0 = full image, 1 = full screen)" 0.0 0.0 1.0 0.01

#pragma parameter CROPPING_SETTINGS "= Cropping parameters =" 0.0 0.0 1.0 1.0
#pragma parameter OS_CROP_TOP "Overscan crop top" 0.0 0.0 1024.0 1.0
#pragma parameter OS_CROP_BOTTOM "Overscan crop bottom" 0.0 0.0 1024.0 1.0
#pragma parameter OS_CROP_LEFT "Overscan crop left" 0.0 0.0 1024.0 1.0
#pragma parameter OS_CROP_RIGHT "Overscan crop right" 0.0 0.0 1024.0 1.0

#pragma parameter MOVING_SETTINGS "= Moving parameters =" 0.0 0.0 1.0 1.0
#pragma parameter SHIFT_H "Horizontal shift" 0.0 -2048.0 2048.0 1.0
#pragma parameter SHIFT_V "Vertical shift" 0.0 -2048.0 2048.0 1.0
#pragma parameter CENTER_AFTER_CROPPING "Center cropped area" 1.0 0.0 1.0 1.0
// clang-format on
