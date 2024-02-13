#ifndef SUBPX_MASKS
#define SUBPX_MASKS

#pragma parameter mask_strength "Phosphor Mask Strength" 0.5 0.0 1.0 0.01
#pragma parameter mask_picker "Phosphor Mask Layout" 0.0 0.0 19.0 1.0

int mask = int(mask_picker);

#include "../subpixel_masks.h"

#endif
