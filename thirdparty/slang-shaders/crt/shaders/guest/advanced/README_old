   ____ ____ _____      ____                 _        ____     __     __                         
  / ___|  _ \_   _|    / ___|_   _  ___  ___| |_     |  _ \ _ _\ \   / /__ _ __   ___  _ __ ___  
 | |   | |_) || |_____| |  _| | | |/ _ \/ __| __|____| | | | '__\ \ / / _ \ '_ \ / _ \| '_ ` _ \ 
 | |___|  _ < | |_____| |_| | |_| |  __/\__ \ ||_____| |_| | |_  \ V /  __/ | | | (_) | | | | | |
  \____|_| \_\|_|      \____|\__,_|\___||___/\__|    |____/|_(_)  \_/ \___|_| |_|\___/|_| |_| |_|

   CRT - Guest - Dr.Venom
   
   Copyright (C) 2018-2019 guest(r) - guest.r@gmail.com

   Incorporates many good ideas and suggestions from Dr.Venom.
   
   
This guide: Rev 1, April 28th 2019
   
 
----------------
# Introduction #
----------------

This shader mimics the look of Cathode Ray Tubes (CRTs) on modern LCD monitors.
Its main goal is to be accurate out of the box, and keep plenty of customization options for the CRT purists to tinker with. That's you if you're reading this :)

Since the range of shader parameters can be a bit overwhelming we've created this little readme to explain some of the options. 

If you have questions, please don't hesitate to reach out to us! 

Most of the stuff in this guide has been discussed on English Amiga Board here: http://eab.abime.net/showthread.php?t=95969&page=2



--------------
# Afterglow # 
--------------

The RGB phosphors in CRTs have a decay time when going from fully lit to off. When going from bright lit status to off the luminance level instantly falls to about 10%, but then tend to linger at that level for a bit. This is what we call "afterglow". This afterglow of phosphors causes slightly visible trails when bright objects are moving fast on a dark background.

For an example of this "CRT phosphor afterglowing" see this video on youtube: https://youtu.be/N72uiXFgrh0

If you look at the UFO flying by from second 13 to 18 you can clearly see the trail in motion. In the video it looks slightly exaggerated because of the way cameras work.

The shader allows to control the strength of afterglow per Red, Green, Blue "phospors". Generally the blue phosphor has the least afterglow, so you may want to have red and green a bit stronger than blue. 

Afterglow switch ON/OFF                 // Turn afterglow feature ON/OFF
Afterglow Red (more is more)            // controls the initial brightness of the afterglow on the red phosphor. Higher values makes red more visible in the afterglow. Default is 0.07
Persistence Red (more is less)          // Controls the decay time, i.e. if red should fade away quickly from its initial brightness or slowly. A higher value makes the red phosphor fade away more quickly. Default value is 0.05. 
Afterglow Green   
Persistence Green 
Afterglow Blue 
Persistence Blue  
Afterglow saturation                    // Determines the saturation of the RGB afterglow. Generally afterglow has very low saturation so it defaults to a low value of 0.1. Higher values give more saturation.
   
-------------
# TATE mode #
-------------

Yes we do in weird lingo :) In short TATE means "vertical" orientation for arcade monitors. The term ”Tate” is apparently a shortened form of the Japanese verb “tateru,” which means “to stand.” Pronounced “tah-teh,” though the common mispronunciation of “tayte” has gained semi-acceptance. Also commonly spelled with all capital letters (“TATE”), though it is not an acronym. 

In the shader when you set TATE to 1 both the scanlines and mask orientation will rotate by 90 degrees to accommodate to Arcade games that are in vertical orientation, like 1942 shooters and the like. Mostly useful for when running MAME vertical games.

-------------------------
# Smart Integer Scaling #
-------------------------

When the video scaling is set to full screen, it may happen that the scanlines appear slightly uneven. To remedy this you can enable this option. It will scale vertically to the nearest suitable integer scale factor. The "smart" part is that it will take into account "overscan" as it appeared on TVs, so it may scale the image slightly larger than the screen size to keep things full screen while also enabling integer scale factor. When it makes use of this overscan up to about 10% of the image may fall outside of the visible area. If the nearest integer larger scale factor would go over this this bound, it chooses the nearest lower integer scale, which will come at the cost of some black bars around the image. Just try and learn :).

Values are:
0 = off
1 = smart integer scale vertically
2 = smart integer scale vertically and keep aspect ratio 

----------------
# Raster Bloom #
----------------

Raster bloom is a feature that occurs on some CRTs, where the image will slightly expand on brighter images. This may give some extra "pop" to short flashing bright images, like big explosions and such. Almost all CRTs are experiencing raster bloom to some extent, but how much and whether it's really visible depends largely on the CRT model and how much it has aged / been maintained.

The following two youtube videos show examples of raster blooming.

The first movie shows metal slug on a CRT TV. If you go to second 75 and look at the bottom right where it says "Credit 04", you'll see how this text gets pushed slightly to the outside of the bezel when the screen gets bright and it coming back in on darker screens. This is sort of the default case where bloom sizes up the image by 1 to 2%.

https://youtu.be/_K-kTSUaekk

The next one shows an older TV where the voltage regulation has clearly been diminished. From 1:15 in this youtube:

https://youtu.be/zbGYwPwf-zA

The guy is turning the brightness of the monitor up and down. You can see how the white raster expands quite a bit when the brightness on the monitor is turned up and then shrinks back when it's lowered. This is what we call the 10% blooming case. I.e. it has a defect and needs servicing, opposed to the 1 - 2% normal bloom on properly calibrated sets.

The following parameters control Raster Bloom in the shader:
  
Raster bloom %          // This determines the raster bloom scale factor. A value of 2 to 3% will result in realistic raster bloom. Larger values will exaggerate the effect, or you're really into mimicking a faulty CRT :).
R. Bloom Overscan Mode  // This setting determines whether a fully bright image may push the image outside of the screen / bezel or not. Possible settings:

(This assumes that automatic full screen scaling is enabled in Retroarch video options.)
0 = Raster bloom is always within the boundaries of the visible screen
1 = holds the middle between option 0 and 2 :)
2 = Raster bloom pushes part of the image outside of the visible area on bright screens.

Setting 1 and 2 more or less mimic the behavior as seen in the metal slug x video mentioned above.

-------------------------
# Saturation adjustment #
-------------------------

This controls how saturated the image is. Higher values give more saturation. This may be useful with emulated systems like e.g. SNES, which tend to have a more saturated image than default. 

--------------------------
# Gamma In and Gamma Out #
--------------------------

Setting Gamma In ("input gamma") to 2.4 affects the following:

- horizontal interpolation is done in gamma space, where brighter colors spread a bit more over darker ones. Should match interpolation of CRT's.
- scanlines are applied differently
- masks are better distributed over the spectrum

Generally it's best to keep this value at the default of 2.4 as it mostly concerns a shader internal conversion step where this default value results in desired behavior


Gamma Out:

Of course we have to switch back to the normal color space, so there is the Gamma Out out functionality. In some CRT shaders it's about 10% lower compared with the input gamma. Different input/output gamma values affect saturation and brightness.

Since there is an option to use neutral input/output gamma (1.0), you can observe the difference within the shader.

I think most importantly is that this part of the shader functions as intended and can be tweaked to personal preference.

I think the most catchy part here is that CRT's have this 2.2-2.25 gamma and I defaulted 2.4. It roots in sRGB a bit and I got used to it. You can set output gamma (often referred to as CRT gamma) to 2.25 np.

From the CRT color research (at the end of this section) it was found that the researched CRTs had a Gamma of 2.25. So to have the shader color profiles displayed properly one has to set the Gamma Out to 2.25.  


----------------
# Bright boost #
----------------

This setting makes the image brighter. Making the image brighter is useful / necessary when scanlines are enabled, as scanline simulation reduces brightness of the image.

Good values for "bright boost" are between 1.1 and 1.3 depending on preference. Note that there's a tradeoff, higher values for brightboost make the image brighter, but can cause clipping of colors, making the image become more "flat". On lower color systems, like 8-bit, this clipping may be seen as soon as you go over 1.2. It's best to experiment a bit to see what suits one's own taste.   

-------------
# Scanlines #
-------------

One of the most distinguishable features of CRTs when run in progressive mode are visible scanlines. I.e. the image is characterized by distinguishable brighter lines (the "scanlines"), and darker/black in-between lines. Sometime people refer to these darker lines when they say scanlines, but the bottom line is the same ;-)

Since good scanline simulation is one of the most determining aspects of good CRT simulation, there are no less than 5 parameters that control this feature:
 
Scanline Type         // 0 = normal scanlines; 1 = more intense scanline type ; 2 = more aggressive / accentuated scanline type. Default = 0
Scanline beamshape    // The scanline beamshape "low" and "high" parameters define the look of the scanlines. With these two settings the scanlines can be made thinner or thicker, less or more rounded at the edges and degrees between them. In particular the scanline beamshape low value defines the scanline shape near the middle and the beamshape high value defines the scanline shape near the edges. For example a setting of 5.0,15.0 creates a stronger but more flat like looking scanline. The default values work very well, but you may want to experiment depending on your screen resolution and preference.
Scanline dark     		// On a real CRT darker scanlines are thinner than bright scanlines. This setting determines by how much. Raising the value makes them thinner. Default is 1.35. 
Scanline bright   		// On a real CRT bright scanlines are thicker than darker scanlines. This setting determines how much thicker. Lowering the values will make them thicker. Default is 1.10. 
Increased bright scanline beam  // This accentuates the brighter parts or pixels within a single scanline. On a real CRT bright pixels within a scanline will appear thicker. This setting controls by how much. Default is 0.65. 

---------------------------
# Sharpness and smoothing #
---------------------------

The following parameters determine the smoothness versus sharpness balance of the image.

A real CRT has the peculiar but very nice characteristic that single pixels are smooth, while the overall image is sharp. This opposed to modern LCDs, where both individual pixels and the total image are very sharp. To recreate the soft "pillow shape" phosphor dot characteristics while keeping overall image sharp there 4 parameters that control this balance. 

Horizontal sharpness     //  This setting determines the overall image sharpness mostly. Higher values create a sharper image. Default is 5.25. 
Substractive sharpness   //  This is a  nice "hack" that may be used in combination with "horizontal sharpness". Higher values give more sharpness to pixels and mask. Default is 0.05. 
Horizontal Smoothing    //   This is some candy that blends pixels more that are close in color tint to each other. Gives a nice touch to especially high color systems that have many grades of color (like playstation).
Smart Smoothing Threshold // This works in cooperation with "Horizontal Smoothing". It sets the threshold for how far apart "same" color tints must be for horizontal smoothing to apply.  

-------------
# Curvature #
-------------

The physical properties of (earlier) shadow mask CRTs make them to have a slightly curved screen. For some of these CRTs the image may appear slightly curved as well, although that largely depends on make and model and how well the set is calibrated. The following two parameters allow for the curvature to be configured in the vertical and horizontal direction.

CurvatureX        // default is OFF. In case you like curvature, a recommended value is 0.03.
CurvatureY        // Default is OFF. In case you like curvature, a recommended value is 0.04. 

--------
# Glow #
--------

Phosphors typically create a small surrounding glow on objects. This is especially noticeable when bright objects are shown on a dark background, and even more so when watching a CRT in a dim environment. Whether this glow exists because the light emission gets scattered slightly in the front glass of the tube or something else we would like to know :). 

In the shader the glow strength, radius and grade (fall-off) can be configured.

Glow Strength           // Determines the overall strength of the glow . Default is 0.02. 
H. Glow Radius          // Determines the radius of the glow in horizontal direction. Higher values create a bigger radius. Default is 4.0.  
Horizontal Glow Grade   // Determines the grade/ fall-off / fade of the glow. Higher values make the glow fade out more quickly. 
V. Glow Radius          // Same as horizontal parameter but for vertical.
Vertical Glow Grade     // Same as horizontal parameter but for vertical.  

--------
# Mask #
--------

Together with scanlines the second most distinguishable feature of CRTs is the very subtle pattern, also called "mask", apparent in images displayed on a CRT. The type of pattern largely depends on the technology used, shadow mask (dotmask and slotmask) versus aperture grille (trinitron). All these types of masks can be simulated with the shader. 

CRT Mask: 0:CGWG, 1-4:Lottes, 5-6:'Trinitron', 7: slotmask (see below)    // The shader allows to set 7 different types of masks

0 = CGCW - a very light generic mask pattern
1 - 4 = Lottes masks, 4 different types of masks that simulate a shadow mask more or less.
5 - 6 = Trinitron. These are very effective mask types, that closely resemble the appearance of Trinitron's aperture grill. "5" is a finer version, mostly for use on 1080p, "6" is a more coarse version, mostly for use on 4K resolution.
7 = slotmask, to be used in conjunction with the additional slotmask parameters.

Mask types 1 to 6 strength can be set with these two parameters:

Lottes maskDark       // lower values generally make the mask appear more strongly
Lottes maskLight      // higher values generally make the mask appear more strongly 

When using "7" slotmask, below parameters need to be set:

CRT Mask Size (2.0 is nice in 4k)   // set to 1 for 1080p, 2 for 4K resolution
Slot Mask Strength                  // Overall strength of the slot mask. Good values are around 0.5 
Slot Mask Width                     // Determines the horizontal size of the slotmask pattern. Use 2 or 3 for 1080p, 4-6 for 4K resolution.
Slot Mask Height: 2x1 or 4x1"       // Determines the vertical size of the slotmask pattern. Use 1 for 1080p, or 2 for 4K resolution. 

Finally there's a parameter that influences the look of mask 5 - 7.
Mask 5&6 cutoff                     // This determines how soon black appears between pixels for colors that are close to the R, G, or B primaries. This is suitable for both Trinitron and Slotmask. Default is 0.2. Higher values create a quicker cutoff to black between these pixels. 

-----------------------
# Color Temperature % #
-----------------------

Each CRT monitor has a "whitepoint" setting, which influences how warm or cold the colors will look. The whitepoint setting differed quite a bit between different CRT models. Some had a 5000K (D50) whitepoint, i.e. "warm" colors, and some had a 9300K whitepoint,i.e. very "cold" colors.  Most will sit somewhere in between, with today's standard being 6500K (D65). Note that a warmer setting tends the greys noticeably to a more yellowish brown, while the colder setting moves it to a more blueish grey. 

At setting 0 the "color temperature %" is equivalent to the D65 whitepoint, let's say "neutral". Lower it to move towards the more warm 5000K point, or raise it to move it more towards the 9300K cold point.    

-------------------
# PVM Like Colors #
-------------------

 PVM Like Colors        // This is a bit of candy that tries to simulate some of the color aberrations that appear with Sony PVM and BVM monitors. 
 
 -------------------
# LUT Colors #
-------------------

The shader has two ways to simulating "CRT Colors". LUT colors and CRT Color Profiles. LUT colors use lookup tables to transform the sRGB color profile to something more close to CRT colors. These LUTs come from varying sources, and the accuracy regarding CRT color simulation is a bit of an unknown, but at the least they provide some nice alternative color schemes which you may like.
 
 
---------------------     ---------------
# CRT Color Profile #  &  # Color Space #
---------------------     ---------------

"CRT colors" changes the default colors to something more close to what the RGB Phosphors in CRTs produce.

These profiles are about subtle changes, but if you were used to the display of CRTs you may remember the display having more vivid greens, softer yellows and red, etc. It all depends on the type of CRT you were looking at, but admittedly CRT colors are different from the default sRGB color gamut that is prevalent in today's LCD screens.

The CRT colors are based on research of the CIE chromaticity coordinates of the most common phosphors used back in the day. Therefore we ended up with 3 "specs" which are EBU standard phosphors, P22-RGB phosphors and the SMPTE-C standard. These three profiles can be selected under "CRT colors" as number 1, 2 and 3.

Then there are two additional "calibrated" profiles that actually quite closely match a Philips based CRT monitor and a Trinitron monitor. They are profile number 4 and 5.  

Some more information on these profiles can be read below.

The main drawback currently is that Phosphor color primaries are partly outside the sRGB spectrum, such that for these profiles a "Wide Color Gamut" monitor is needed / recommended. This is what the "Color space" option is for. If you happen to own a monitor that is able to display DCI-P3 color gamut, then set this option to "1". Option "2" is for AdobeRGB and "3" for Rec. 2020. DCI-P3 is verified to be quite accurate.
 
 
In conclusion:
 
The good news is that we've got "CRT colors" largely covered now with the correct specs and two "quite accurate" calibrated profiles. The bad news is that with a default sRGB monitor the profiles will be more or less clipped and look wrong, depending on the content. Then we have some good news, as it seems that DCI-P3 or some other wide color gamut will be part of the HDR-500+ spec (see here: https://displayhdr.org/wp-content/uploads/2019/02/DisplayHDR_SpecChart_Rows_190219.jpg ) So within a few years wide color gamut should become mainstream in monitors.  The question remains whether for HDR-500+ certified monitors this wide color gamut can be enabled by the user or emulator, or whether it will be only available in HDR content encoded mode. Time will tell.

Lastly, let's not forget we are talking about subtle differences from the default sRGB colors here. Let's just say you have to be slightly OCD on CRT tech to really appreciate the difference :-)

For those interested below is some additional information on each of the profiles and the specific chromaticity values used. This is purely additional information, there is no need (or possibility) to do anything with these in the shader settings. 


"CRT Colors" for CRT-Guest-Dr-Venom  -- Additional information.

3 x spec
2 x calibration

Specifications for three standards

1. EBU Standard Phosphors
// Amongst others used in Sony BVMs and Higher-end PVMs
// Tolerances are described in the E.B.U. standard reference document "E.B.U. standard for chromaticity tolerances for studio monitors" tech-3213-E.
// PVM-1440QM service manual quotes 0.01 as tolerance on the RGB CIE coordinates. 

xb0 = "150.000000"
xg0 = "290.000000"
xr0 = "640.000000"
yb0 = "60.000000"
yg0 = "600.000000"
yr0 = "330.000000"

Whitepoint is D65

X 95,04  --> Xw0 950.4
Y 100    --> Yw0 1000
Z 108,88 --> Zw0 1088.8

2. P22 Phosphors 
// These phosphors are often quoted as the "default" phosphors used in CRTs 
// Also used in lower-end PVMs, see Sony PVM-20M4E 20M2E Colour Video Monitor.pdf
// These can still be bought :-)  https://www.phosphor-technology.com/crt-phosphors/  -- includes CIE coordinates.

xb0 = "148.000000"
xg0 = "310.000000"
xr0 = "647.000000"
yb0 = "62.000000"
yg0 = "594.000000"
yr0 = "343.000000"

3. SMPTE-C
// Spec for most of America.
// I have forgone on the 1953 NTSC standard, as apart from very few early color TV's the 1953 NTSC standard was never actually used. Instead less saturated primaries were used to achieve brighter screens.
// Taken from the WIKI on NTSC (https://en.wikipedia.org/wiki/NTSC) In 1968-69 the Conrac Corp., working with RCA, defined a set of controlled phosphors for use in broadcast color picture video monitors. This specification survives today as the SMPTE "C" phosphor specification: 

xb0 = "155.000000"
xg0 = "310.000000"
xr0 = "630.000000"
yb0 = "70.000000"
yg0 = "595.000000"
yr0 = "340.000000"

Whitepoint is D65

X 95,04  --> Xw0 950.4
Y 100    --> Yw0 1000
Z 108,88 --> Zw0 1088.8


4. Calibrated profile for Philips CRT monitors. Of course an approximation, but I'm pleased with the "quite accurate" results.
// Manually calibrated and compared to real Philips based CRT monitors, running side by side with the shader on a 10-bit DCI-P3 gamut panel. This calibrated CRT profile covers amongst others Philips CM8533, Philips VS-0080, and Commodore 1084.
// Note the whitepoint is significantly different from D65. It's closer to 6100K, but clearly not on the blackbody curve. Possibly an ISO-line target, given the slight hue on the whitepoint. Other than that it could be aging / whitepoint drift. I compared four CRT monitors, one of them in very mint condition, and they all have this slight hue on the whitepoint, so I would guess this is how they came out of the factory. But then again since these things are now getting close to 30 years old, who knows? Either way the profile should be good: factory out or true to life aged CRTs... :D.   
// It's important this specific whitepoint is used in the shader or the colors will not be accurate.
// Also it's important to note that this profile should be used with "Gamma Out" at 2.25 or the colors will be less accurate.

xb0 = "154.000000"
xg0 = "300.000000"
xr0 = "635.000000"
yb0 = "60.000000"
yg0 = "620.000000"
yr0 = "339.000000"

Whitepoint:
Xw0 = "910.000000"
Yw0 = "1000.000000"
Zw0 = "960.000000"


5. Calibrated profile for Sony Trinitron Monitor. 
// In a similar fashion as the Philips CRT based profile, this is a manually calibrated profile for a Sony Trinitron monitor, model KX-14CP1.
// This monitor uses a Whitepoint that is close to 9300K. The Z value in the calibration process has been raised to the point where the "blue-ishness" of the white matches. To achieve further 9300K white, I guess one has to raise the hardware whitepoint of the host PC monitor...
// It's important this specific whitepoint is used in the shader or the colors will not be accurate.
// Also it's important to note that this profile should be used with "Gamma Out" at 2.25 or the colors will be less accurate.

xb0 = "152.000000"
xg0 = "279.000000"
xr0 = "647.000000"
yb0 = "60.000000"
yg0 = "635.000000"
yr0 = "335.000000"

Whitepoint:
Xw0 = "903.000000"
Yw0 = "1000.000000"
Zw0 = "1185.000000"

End :)





