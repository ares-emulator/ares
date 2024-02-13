xBR Shaders
=======
These shaders uses the xBR algorithm to scale. xBR (scale by rules) works by detecting edges and interpolating pixels along them. It has some empyrical rules developed to detect the edges in many directions. There are many versions implemented and they differs basically by the number of directions considered and the way interpolations are done.


xbr-lvl2.cg
--------------

It can detect four edge directions, which shows as better rounded objects on screen. The interpolation blends pixel colors, so that the final image is smoother than the noblend versions. It works well in any integer scale factor.

Recommended sampling: `Point`
Recommended scale factors: `Integer multiples`

xbr-lvl2-sharp.cg
--------------

It can detect four edge directions, which shows as better rounded objects on screen. The interpolation blends pixel colors, so that the final image is smooth. This is a sharper version than the standard. It works well in any integer scale factor.

Recommended sampling: `Point`
Recommended scale factors: `Integer multiples`


xbr-lvl3.cg
--------------

It can detect six edge directions, which shows as even better rounded objects on screen than the lvl2. The interpolation blends pixel colors, so that the final image is smooth. It works well in any integer scale factor.

Recommended sampling: `Point`
Recommended scale factors: `Integer multiples`

xbr-lvl3-sharp.cg
--------------

It can detect six edge directions, which shows as even better rounded objects on screen than the lvl2. The interpolation blends pixel colors, so that the final image is smooth. This is a sharper version than the standard. It works well in any integer scale factor.

Recommended sampling: `Point`
Recommended scale factors: `Integer multiples`

super-xbr.cg
--------------

Super-xBR uses parts of the xBR algorithm and combines with other linear interpolation ones. It is intended for games with smooth color gradients. Compared to the hybrid ones, Super-xBR provides a much better image, without discontinuities.

Recommended sampling: `Point`
Recommended scale factors: `Integer multiples`

super-xbr-fast.cg
--------------

It's a faster version of Super-xBR.

Recommended sampling: `Point`
Recommended scale factors: `Integer multiples`

Other Presets folder
--------------

This folder contains other presets for historical purposes.
