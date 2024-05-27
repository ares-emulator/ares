------------------------------------------------------------------------------------------------------------
Mega Bezel Shader Readme
------------------------------------------------------------------------------------------------------------
![Mega Bezel Logo](MegaBezelLogo.png)

**Version V1.17.2_2024-05-18**
----------------------------------------
----------------------------------------

This file is best viewed in a markdown editor/viewer. You can also view it here https://github.com/HyperspaceMadness/Mega_Bezel with correct formatting

If you are wondering **"What is this thing?"** here's a little overview sharing our ideas around the project and some of the Mega Bezel features:
[RetroArch – Introducing the Mega Bezel Reflection Shader – Libretro](https://www.libretro.com/index.php/retroarch-introducing-the-mega-bezel/)

Find more conversation about the shader here on the Libretro forum:

https://forums.libretro.com/t/hsm-mega-bezel-reflection-shader-feedback-and-updates

----------------------------------------
----------------------------------------
----------------------------------------
**!!!IMPORTANT!!! - Please be sure to read the INSTALLATION and RETROARCH SETUP instructions Below**
----------------------------------------

----------------------------------------
----------------------------------------
----------------------------------------
**Licensing**
----------------------------------------

The base Mega_Bezel package and included components licensing are GPLv3

Community Collections/Packs are licensing is independent of this. Please consult each package details.

----------------------------------------
----------------------------------------
----------------------------------------
**Latest Releases**
----------------------------------------

[Shader Package Approx. 12 MB](https://github.com/HyperspaceMadness/Mega_Bezel/releases)

[Extra Examples Package]( https://github.com/HyperspaceMadness/HSM_Mega_Bezel_Examples/releases)

**Github Repo**
----------------------------------------
https://github.com/HyperspaceMadness/Mega_Bezel

**Intro**
----------------------------------------

This shader package is meant for you to experiment with and make your own creations if you like. Try adjusting the shader parameters to change the look, as most things are adjustable for personal taste. The base package is now part of the Libretro repo but there will also be updates coming periodically which may get a pre-release here before they go into the main repo.

----------------------------------------
----------------------------------------
----------------------------------------
**Installation - VERY IMPORTANT, PLEASE READ**
----------------------------------------

***
**INSTALLING THE BASE MEGA BEZEL PACKAGE**
* **You MUST use Retroarch Version 1.9.8 or Later** (It will fail to load on earlier versions)
* **If you want to use the default Mega Bezel included with Retroarch:**
  * Use **Online Updater -> Update Slang Shaders** to update the shaders, the shaders will be added here `Retroarch/shaders/shaders_slang/bezel/Mega_Bezel`
* **If you want to install from a zipped release from the link above:**
  * If the `shaders/shaders_slang/bezel/Mega_Bezel` folder exists delete it
  * Inside the .zip is a **Mega_Bezel** folder. Copy the Mega_Bezel folder into your `Retroarch/shaders/shaders_slang/bezel` folder (If the bezel folder isn't there you can create it)
  * The final path to the Mega bezel should be `Retroarch/shaders/shaders_slang/bezel/Mega_Bezel`

***
**RETROARCH SETUP**
  * Set video driver to **Vulkan** or GLCore if Vulkan is not available
  * It will run in **GLCore** but seems faster in **Vulkan**
  * **D3D IS NOT RECOMMENDED**. If it loads in D3D it has a VERY slow load time
  * Restart Retroarch after changing the video driver
* Open the **Settings** Menu and Set:
  * **User Interface / Show Advanced Settings** to **ON**
  * **Video / Scaling / Aspect Ratio** to **Full**
    * This will match your monitor aspect aspect ratio
  * **Video / Scaling / Integer Scale** to **OFF**
  * **Video / Output / Video Rotation** to **Normal**
  * **Core / Allow Rotation** to **OFF** -- **Important for FB Neo**
  * For **FB Neo**
    * Turn **vertical mode OFF** in **Quick Menu > Core Options** if it was previously turned on
  * **Do all of this before loading content**
* For **FB Neo**
  * **If your game is vertical** set the Rotate CRT Tube parameter to 1. If it is now upside down set Flip Core Image Vertical and Horizontal to 1

***
**LOADING AND SAVING PRESETS**
  * **Load a preset** in the shaders menu.
      * The Mega Bezel shader presets are found in: `Retroarch/shaders/shaders_slang/bezel/Mega_Bezel/Presets`
  * **IMPORTANT!!** When saving a preset make sure you have the **Simple Presets** feature set to **ON**
    * This will save a preset with a reference to the preset you loaded plus whatever parameter changes you made
    * This will keep your presets loading properly when the shader updates in the future

***
**EDITING PRESETS**
  * You can then open this Simple Preset file and add other parameter adjustments or set textures as you see fit. E.G. add the following lines to change the background image:
    * `BackgroundImage = "MyImage.jpg"`
  * Or change the path indicated on the `#reference` line to point at a different base preset

***
**INSTALLING ADDITIONAL PRESET/GRAPHIC COLLECTIONS**
  * Create a folder named `Mega_Bezel_Packs` in the root of the shaders folder, the final path of this should be `Retroarch/shaders/Mega_Bezel_Packs
  * Place any of the additional collections inside this folder
  * For example the final path to the examples pack should be `Retroarch/shaders/Mega_Bezel_Packs/HSM_Mega_Bezel_Examples`

----------------------------------------
----------------------------------------
----------------------------------------
**Community Collections / Packs**
----------------------------------------

@Duimon: Awesome graphics and presets for the different historical consoles & computers::
* [Releases · Duimon/Duimon-Mega-Bezel (github.com)](https://github.com/Duimon/Duimon-Mega-Bezel/releases/)
* https://forums.libretro.com/t/duimon-reflection-shader-graphics-feedback-and-updates

@TheNamec: Really amazing graphics for the Commodore & Amiga systems and PVMs:
  * [Releases · TheNamec/megabezel-commodore-pack (github.com)](https://github.com/TheNamec/megabezel-commodore-pack/releases)
  * https://forums.libretro.com/t/thenamec-mega-bezel-commodore-pack-announcement

----------------------------------------
----------------------------------------
----------------------------------------
The Mega Bezel is coded and maintained by HyperspaceMadness, but leverages some amazing work from the community.
HyperspaceMadness@outlook.com

----------------------------------------
----------------------------------------
----------------------------------------
**Acknowledgements**
----------------------------------------
***
**THANKS TO THE SHADER WRITERS!**

The Mega Bezel sits atop the shoulders of giants and uses a number of amazing shaders written by shader writers from the community:
  * Guest.r (Guest-Advanced crt shader)
  * EasyMode
  * CGCW (LCD Grid)
  * DariusG (GDV Mini)
  * Dogway (Grade Color Correction)
  * Hyllian (SGENDPT)
  * Aliaspider (GTU)
  * Sp00kyFox (MDAPT & ScaleFX)
  * Trogglemonkey (Royale 3D Curvature)
  * Flyguy (Text Shadertoy)
  * Special thanks to Hunterk for porting a number of these and helping me with his expertise along the way!


***
**THANKS TO THE MEGA BEZEL GRAPHICS ARTISTS PUSHING THE FEATURES DURING DEVELOPMENT**

The Mega Bezel would also not have gotten to this level of sophistication without some intense feedback from graphics experts pushing the features and finding the holes.

  * @Duimon
  * @TheNamec
  * @Soqueroeu

***
**THANKS TO THE GRAPHICS ARTISTS WHO HAVE INSPIRED US**

And of course I probably would never have started this without seeing the great overlays created previously
  * @OrionsAngel
  * @exodus123456

----------------------------------------
----------------------------------------
----------------------------------------
**What does it do?**
----------------------------------------

  * Adds an auto-generated bezel around the screen with reflection
  * Enable easier use of bezels and more "natural" presentation
  * Ease of use for screen scaling and automatic aspect ratio with existing shaders
  * Provide a consistent set of enhanced features wrapped around the core crt shaders
  * Layering images to add artwork and visual effects

----------------------------------------
**How does it work?**
----------------------------------------

  * In general there is a background image which fills the screen, then the scaled down game screen with an automatically generated bezel image is drawn on top.
  * The bezel and frame you see around the screen is auto generated and not part of the background image
  * Additional Images can be layered on top to augment the look
  * Most things can be changed to your taste with adjustment of the parameters, so try them out!

----------------------------------------
----------------------------------------
**Choosing a Preset**
----------------------------------------

  * Presets are named/sorted by performance
  * The most flexible and most resource hungry start with index 0.
  * As the name's index number increases the performance of the preset improves but but flexibility decreases.
  * The presets with the lowest performance requirements are:
    * MBZ__3__STD__GDV-MINI - This has a Dynamic Bezel and Reflections
    * MBZ__4__STD-NO-REFLECT__GDV-MINI - This Has a Dynamic Bezel but No Reflections
    * MBZ__5__POTATO__GDV-MINI - This has no Dynamic Bezel and no Reflections
----------------------------------------

**Presets in Mega_Bezel / Presets**
----------------------------------------

- All in the root of the Presets folder use @guest.r's awesome Guest-Advanced CRT shader which is considered the default CRT shader for the Mega Bezel, the only exception to this is the POTATO preset which uses GDV-MINI for performance reasons. The following table is sorted by GPU performance and GPU RAM requirements

|Category                        |Reflection  |Image Layers |Tube Fx  |Pre-CRT Chain   |Smooth Upscale
|--------------------------------|------------|-------------|---------|----------------|---------------
| MBZ__0__SMOOTH-ADV             | ✔          | ✔          | ✔       | ADV + ScaleFx  | 3X
| MBZ__0__SMOOTH-ADV-NO-REFLECT  |            | ✔          | ✔       | ADV + ScaleFx  | 3X
| MBZ__0__SMOOTH-ADV-SCREEN-ONLY |            |             | ✔       | ADV + ScaleFx | 3X
| MBZ__1__ADV-SUPER-XBR          | ✔          | ✔          | ✔       | ADV + XBR     | 2X
| MBZ__1__ADV                    | ✔          | ✔          | ✔       | ADV           |
| MBZ__2__ADV-GLASS-SUPER-XBR    | ✔          |            | ✔        | ADV + XBR     | 2X
| MBZ__2__ADV-GLASS              | ✔          |            | ✔        | ADV           |
| MBZ__2__ADV-NO-REFLECT         |            | ✔          | ✔        | ADV           |
| MBZ__2__ADV-SCREEN-ONLY        |            |             | ✔       | ADV           |
| MBZ__3__STD-SUPER-XBR          | ✔          | ✔          | ✔       | STD + XBR     | 2X
| MBZ__3__STD                    | ✔          | ✔          | ✔       | STD           |
| MBZ__3__STD-GLASS-SUPER-XBR    | ✔          |            | ✔        | STD + XBR     | 2X
| MBZ__3__STD-GLASS              | ✔          |            | ✔        | STD           |
| MBZ__4__STD-NO-REFLECT         |            | ✔          | ✔        | STD           |
| MBZ__4__STD-SCREEN-ONLY        |            |             | ✔       | STD           |
| MBZ__5__POTATO-SUPER-XBR       |            | BG ONLY     |         | POTATO + XBR   | 2X
| MBZ__5__POTATO                 |            | BG ONLY     |         | POTATO         |
|

----
**Pre CRT Shader Chains**
----------------------------------------
----
| Shader Behavior                        |SMOOTH-ADV   |ADV   |STD   |POTATO
|----------------------------------------|-------------|------|------|-------
| Reducing Core Resolution               | ✔           | ✔   | ✔   | ✔
| Resolution Text                        | ✔           | ✔   | ✔   |
| Intro Animation                        | ✔           | ✔   | ✔   |
| De-Dithering                           | ✔           | ✔   |      |
| Image Sharpening                       | ✔           | ✔   | ✔   |
| Uprezed Edge Contour Smoothing         | ✔           |     |      |
| Bandwidth Horizontal Blurring (GTU)    | ✔           | ✔   |     |
| NTSC Signal Processing (NTSC Adaptive) | ✔           | ✔   | ✔   | ✔
| Afterglow                              | ✔           | ✔   | ✔   | ✔
| Color Signal Processing (Grade)        | ✔           | ✔   | ✔   | ✔
| Interlacing & Downsample Blur          | ✔           | ✔   | ✔   | ✔
| Sinden Lightgun Border                 | ✔           | ✔   | ✔   | ✔

NTSC Processing is only included in NTSC Presets, and GTU Horizontal blurring isincluded in non-NTSC presets

**Descriptions:**

  * **POTATO Pre-CRT shader chain**
    * Fewest passes, but still Includes Grade Color Correction
  * **STD Pre-CRT shader chain**
    * Includes Some Basic Processing before the CRT shader
  * **ADV Pre-CRT shader chain**
    * Includes STD chain and adds DeDithering & GTU
  * **ADV Pre-CRT shader chain + ScaleFx Upres**
    * Includes ADV Pre-CRT shader chain and ScaleFX
    * Resolution is tripled in the middle of the chain for ScaleFX
    * This requires increased GPU processing
  * **SCREEN-ONLY**
    * Includes whatever the category's Pre-CRT shader chain but removes the bezel, images and reflection
  * **Glass**
    * Presets which show a blurry reflection in the area around the screen
  * **Image Layering**
    * Layering of multiple images for background, CRT housing, LEDs etc
    * Includes the Automatically Generated Bezel & Frame
  * **Tube Effects**
    * Tube Static Reflection Highlight
    * Tube Diffuse Image & Shadow
    * Tube Colored Gel
    * All presets include Tube Effects except the Potato

**Preset Folders in Mega_Bezel / Presets**

  * **Base_CRT_Presets**
    - Includes presets using different crt shaders for the screen
    - Look here for the LCD preset

  * **Base_CRT_Presets_DREZ**
    * Presets which set the resolution at the beginning of the shader chain
    * Good for reducing the resolution from the core to native res to use with a crt shader
    * Helps working with cores which are outputting at increased internal resolution, e.g. 2x, 4k. When a large image comes from the core and is downscaled to ntive res this creates antialiasing, helping smooth out the jaggies


**HSM Examples Package (Separate additional Package from Mega Bezel)**

  * **Community_CRT_Variations**
    - Presets with crt settings created by community members
    - Thanks to some of our community members who's settings appear here including @BendBombBoom, @NesGuy, @Sonkun & @Cyberlab

  * **Variations**
    * These presets are simple presets referencing one of the presets in the Mega_Bezel folder
    * They reference the original preset then have adjusted parameters or texture paths

  * **Experimental**
    * **Use at your own risk!**
    * These presets are work in progress and are likely to be moved, renamed, dissappear or change behavior at any future release


----------------------------------------
----------------------------------------
----------------------------------------
**Troubleshooting**
----------------------------------------
---

 * **If you have difficulties loading the shader** try loading it with the imageviewer core
    * **Steps**
        * Add the imageviewer core to Retroarch with the online updater
        * Open an image with this core
        * Load the shader
        * If the shader loads correctly then the shader is working.

     * If Retroarch crashes this is usually the core resolution overwhelming the graphics card's resources. This more often happens when you are using a SMOOTH-ADV preset. Try a STD, or STD-DREZ preset to reduce the resolution used within the shader chain
     * When the shader works in imageviewer, but doesn’t work when using a core, it is probably related to the core
     * If you still have difficulties loading the shader with a specific core, try updating the core
     * If you still have difficulties download a new separate version of Retroarch and try it there. Sometimes problems lurk in a random config file which is very hard to track down
 * **To see errors** coming from Retroarch you need to set up your logging settings:
    * **Logging - Logging Verbosity - ON**
      * **Frontend Logging - 0(Debug)**
      * **Log to File - ON**
      * **Restart Retroarch**
      * **Load your shader**
      * **Close Retroarch**
      * These last steps allow you to have a short log for us to look at
      * If you want to see the errors as they happen you can set Log to File to OFF and an additional console window will open when retroarch opens and show errors here


---
**If the Screen is changing size unexpectedly**

  * If the screen changes size when loading a game or switching between different parts of the game e.g. gameplay vs cinematic, this is because of the interaction between the different resolutions the core is outputting on different screens and the shader's integer scale or automatic aspect ratio settings.

* **How to fix**

  * **Make sure Integer Scale is OFF in the RetroArch Video Settings**
  * **If BOTH the HEIGHT and WIDTH of the screen size are changing size**

    * Set the Integer scale mode to OFF (0)
  * **If ONLY the WIDTH of the screen is changing size** (the HEIGHT stays constant)
    * Set the Aspect Ratio Type to Explicit (1) This will use the explicit aspect ratio number instead of guessing

    * If this solves your issue please consider posting on the thread at the top of this document the issue you had so that we can improve the auto aspect ratio in the future

---
**If the Screen is shown in a vertical aspect for a horizontal game:**

  * Set The **[ Aspect Ratio ] / Orientation** to **Horizontal**

---
**If you see artifacts on the game image like circles or interference patterns**
  * These artifacts which look like round swirls or circles like tree trunk rings are called a Moiré patterns which happen when a high frequency pattern is sampled at a lower frequency - https://en.wikipedia.org/wiki/Moiré_pattern

    * The base cause of the Moiré pattern is usually the curvature in combination with visible scanlines

* **How to fix**
    * Set **CRT Curvature Scale Multiplier** to 0, This will remove curvature from the game image but leave everything else the same
    * Set **Integer Scale Mode** to 1 or 2
    * Make the game screen larger with either **Viewport Zoom** or **Non-Integer Scale %**
    * Use a higher resolution monitor if available

----------------------------------------
----------------------------------------
----------------------------------------
**Bug Reporting**
----------------------------------------

* When reporting a bug, it is VERY IMPORTANT to post images of the issue. This helps communicate the issue better & quicker, even if the issue seems simple.
*  Please make sure you are using the latest version of the shader
* Please include info about your setup
  * Preset, Core, Core Internal Res 1x, 2x etc?, Monitor resolution, GPU
*  If you are having any issues with the shader not loading or crashing please include a log. See the readme for how to get a log
  * If loading the shader crashes Retroarch your core internal res is probably too high. Try native resolution or one of the DREZ presets which reduce the resolution in the first pass.

-----------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
----
**Parameter Descriptions**
-----------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------
**[ --- HSM MEGA BEZEL X.X.XXX 20XX-XX-XX --- ]:** - Title, Version, Date

  * **Show Resolution Info** --- Show Resolution info from different aspects of the shader chain with onscreen text

-----------------------------------------------------------------------------------------------
**[ CRT BRIGHTNESS & GAMMA ]:**
  * **Gamma In (Game Embedded Gamma - Gamma Space to Linear) Def 2.4** - The gamma adjustment applied to take the core image and bring it into linear color space. Gamma to Linear Space Decode
  * **Gamma Out (Electron Gun Gamma - Linear to Gamma Space) Def 2.4** - The gamma adjustment applied to the CRT shader's linear color output to bring it back into a gamma corrected space. Also known as Linear to Gamma Space Encode
  * **Post CRT Brightness** - Brightness adjustment on the CRT color output (Applied in Linear Color Space)
  * **Post CRT Brightness Affects Grade Black Level** - As this value is reduced the brightness adjustment amount will be reduced on the black level. So if you set it to a low value there will be almost no increase in brightness on the areas which were black before the black level adjustment.

-----------------------------------------------------------------------------------------------
**[ GRAPHICS GLOBAL BRIGHTNESS ]:**
  * **Graphics Brightness** - Brightness of all graphics and images which are not the CRT Shader
  * **Graphics Gamma Adjust** - Gamma adjustment on all graphics and images which are not the CRT Shader to adjust brightness with a gamma curve

-----------------------------------------------------------------------------------------------
**[ AMBIENT LIGHTING ]:** - Usually used to apply night lighting on all graphics
- **Ambient 1st Image Opacity**
  - How much of the ambient lighting darkening effect is applied when using the first image
- **Ambient 2nd Image Opacity**
  - How much of the ambient lighting darkening effect is applied when using the second image
* **Which Images to Use**
  * **0 - BOTH** - Normal Mode, Use Both Images
  * **1 - 1 ONLY** - Use the first image for both slots
  * **2 - 2 ONLY** - Use the second image for both slots
  * **3 - Swap** - Swap the Images


-----------------------------------------------------------------------------------------------
**[ SINDEN LIGHTGUN BORDER ]:**
  * **Show Border** - Show the white border around the inner edge of the tube
  * **Brightness** - How bright is the border
  * **Thickness** - How Thick is the border
  * **Compensate Empty Tube**
    * **0 - Don't adjust the tube size and puts the white border on top of the CRT image**
    * **1 - Adjust the tube to be wider so the white border is outside the CRT image**


-----------------------------------------------------------------------------------------------
**[ VIEWPORT ZOOM ]:**
* **Viewport Zoom** --- Zoom in or out on everything
* **Zoom CRT Mask** --- When this is on and we zoom in the crt phosphor mask will also scale in integer steps
* **Viewport Position X**
* **Viewport Position Y**

-----------------------------------------------------------------------------------------------
**[ FLIP & ROTATE ]:**

* **Flip Viewport Vertical** --- Some cores flip the viewport (full monitor area), this flips it back
* **Flip Viewport Horizontal**
* **Flip Core Image Vertical** --- Some cores flip the core image, this flips it back
* **Flip Core Image Horizontal**
* **Rotate CRT Tube** --- Turns the tube 90 degrees counter counter-clockwise

-----------------------------------------------------------------------------------------------
**[ ASPECT RATIO ]:**

* **Orientation** --- Should the aspect be tall or wide? This does not rotate the image.
  * **0 - Auto** - Chooses vertical vs horizontal based on the core resolution
  * **1 - Horizontal** - uses horizontal aspect
  * **2 - Vertical** - uses vertical aspect

* **Type**
  * **0 - Auto** - Choose aspect ratio based on an educated guess about the core's resolution
  * **1 - Explicit** - Use the aspect ratio from the **Explicit Aspect Ratio** parameter
  * **2 - 4:3** - Almost all arcade games are 4:3
  * **3 - 3:2**
  * **4 - 16:9**
  * **5 - PAR** - The aspect ratio of the core's pixel resolution
  * **6 - Full** - The screen will scale to the full viewport

  * **Explicit Aspect Ratio** - Ratio used when [Aspect Ratio] Type is set to Explicit or if Auto is chosen and no matching resolution can be found

-----------------------------------------------------------------------------------------------
**[ CRT SCREEN SCALING GENERAL ]:**

- **Integer Scale Mode**
  - **0 - OFF** Use Non-Integer Scale
  - **1 - ShortAxis Integer Scale On** - for the viewport (monitor) in landscape mode this is the vertical axis, If the screen/tube aspect ratio is vertical then integer scale is used for both horizontal and vertical axes
  - **2 - Integer Scale on both axes**

- **Preset is for Monitor Portrait Mode (Smaller CRT Screen)**
  - Turn on if this preset is to be used on monitor in Portrait mode, E.G. if your physical monitor is turned vertical

**[ INTEGER SCALE ]:**

- **Base Integer Scale Max Height %**
  - The maximum screen height of the default integer scale when integer scale is on

- **Integer Scale Multiple Offset**
  - Adjusts the size of the screen by increasing the multiple of the core resolution (on both axes) when using integer scale, to make the screen larger or smaller

- **Integer Scale Multiple Offset Long Axis**
  - Adds an additional multiple offset but for only the long axis, with a horizontal aspect ratio this is the horizontal axis

**[ NON-INTEGER SCALE PERCENT  ]:**

- **Non-Integer Scale %**
  - If integer scale isn't used, this sets the vertical size of the vertical percentage of the full viewport. The default is 82.97 which corresponds to an exact integer scale of 224p content

-----------------------------------------------------------------------------------------------
**[ NON INTEGER - PHYSICAL SIZES ]:**

- **Use Physical Monitor and Tube Sizes for Non-Integer**
  - Use these physical sizes instead of the integer scale percentage

- **Your Monitor's Aspect Ratio**
  - The aspect ratio of your physical monitor, most monitors are 1.77 which equates to 16:9

- **Your Monitor's Size (Diagonal)**
  - The size of your physical monitor

- **Simulated Tube Size (Diagonal)**
  - How big you want the crt screen to be on your monitor


-----------------------------------------------------------------------------------------------
**[ NON-INTEGER - AUTOMATIC SCREEN SCALE & PLACEMENT ]:**

- **Use Image For Automatic Placement (Scale and Y Pos)**
  - When on the placement image is inspected to find where to place and scale the screen image
- **Auto Place Horizontal (X Pos)**
  - 0 - OFF  Screen placed in the center
  - 1 - ON Tries to place the screen in the center of the hole in the placement image
- **Placement Image Mode: TRANSPARENCY : WHITE ON BLACK**
  - What channel of the texture to look at to find the hole in the image, either the transparent part, or a white rectangle on top of a black background

-----------------------------------------------------------------------------------------------
**[ NON-INTEGER SCALE OFFSET ]:**

- **Non-Integer Scale Offset**
  - Additional scale offset added on top of the non-integer scale, including image placement scale


-----------------------------------------------------------------------------------------------
**[ SNAP NON-INTEGER TO INTEGER SCALE ]:**

- **Snap to Closest Integer Scale**
  - Takes the current Non-Integer scale and snaps to the closest integer scale within a tolerance
- **Snap To Closest Integer Scale Tolerance**
  - Tolerance of how far away from the current integer scale we will snap to an integer scale


-----------------------------------------------------------------------------------------------
**[ POSITION OFFSET ]:**

- **Position X** - Moves the entire screen and frame left and right
- **Position Y** - Moves the entire screen and frame up and down


-----------------------------------------------------------------------------------------------
**[ CROPPING CORE IMAGE ]:** ---
Cropping removes parts of the game image at the edges of the screen which were never meant to be seen. Negative values can add more black area at the edges of the screen

- **Crop Mode**
  - **0 - OFF** No Cropping applied
  - **1 - Crop Black Only** Only apply the cropping amount within the black areas of the core image
  - **2 - Crop Any** Apply full crop amount
- **Crop Zoom %** Add Cropping on all sides at once
- **Crop Overscan Top**
- **Crop Overscan Bottom**
- **Crop Overscan Left**
- **Crop Overscan Right**
- **Black Threshold for 'CROP BLACK ONLY'** - The brightness threshold of the black area to be cropped

-----------------------------------------------------------------------------------------------
**[ DREZ DOWNSAMPLE FILTER - HYLLIAN - DREZ PRESETS ONLY ]:**
- **DREZ Filter** - Filter to use in the DREZ downsampling pass
  - 0 - B-Spline
  - 1 - Bicubic
  - 2 - Catmull-Rom
  - 3 - Bicubic H

-----------------------------------------------------------------------------------------------
**[ SCANLINE DIRECTION ]:**

- **Scanline Direction** - Direction of the scanlines
  - 0 - Auto --- Chooses horizontal or vertical scanline direction based on aspect ratio
  - 1 - Horizontal
  - 2 - Vertical
- **No-Scanline Mode**
  - Guest Advanced Only, keeps scanline dynamics withoug the darkness between lines
  - 0 - OFF
  - 1 - ON

-----------------------------------------------------------------------------------------------
**[ CORE RES SAMPLING ]:**

**Adjusting core res sampling changes how the CRT perceives the core resolution**
  e.g. If you use a core with 4X internal resolution you can set core res sampling to 0.25 it be sampled as if it was at 1x resolution

- **Scanline Direction Multiplier (X-Prescale for H Scanline)**
  - Adjust the sampling in direction of the scanlines
  - E.G. if the scanlines are horizontal this adjusts sampling along the horizontal axis
  - 0 uses the upscaling ratio, so for a SMOOTH-ADV preset this ratio would be 300%, If there is no upscaling this ratio is 100%
- **Scanline Dir Downsample Blur**
  - Add blur along the scanline direction
- **Opposite Direction Multiplier (Y Downsample for H Scanline)**
  - Adjust the sampling in direction opposite of the scanlines
  - E.G. if the scanlines are horizontal this adjusts sampling along the vertical axis
  - 0 uses the upscaling ratio, so for a SMOOTH-ADV preset this ratio would be 300%, If there is no upscaling this ratio is 100%
- **Opposite Dir Downsample Blur**
  - Add blur along the opposite direction of the scanlines

-----------------------------------------------------------------------------------------------
**[ FAST SHARPEN - GUEST.R ]:**

- **Sharpen Strength**
- **Amount of Sharpening**
- **Details Sharpened**

-----------------------------------------------------------------------------------------------
**[ INTERLACING From Guest.r :) ]:**

- **Interlace and Fake Scanlines Trigger Res**
  - Resolution where the shader should switch into its interlace or high res content mode.
- **Interlacing Mode**
  * Default is Mode 4 which gives a result with no scanlines and bilinear blending
  - **-1 - No Scanlines**
  - **0 - No Interlacing**
  - **1-3 - Normal Interlacing Modes**
  - **4-5 - Interpolation Interlacing Modes**
- **Interlacing Effect Smoothness**
- **Interlacing Scanline Effect**
- **Interlacing (Scanline) Saturation**

-----------------------------------------------------------------------------------------------
**[ FAKE SCANLINES ]:**

- **Show Fake Scanlines - OFF | ON | USE TRIGGER RES**
- **Opacity**
  - Opacity of scanlines added on top of the crt image.
  - These scanlines are not tied to the core image resolution
- **Scan Resolution**
- **Scan Resolution Mode: AUTO (CORE RES) : EXPLICIT**
- **Explicit Scan Resolution**
- **Int Scale Scanlines**
- **Rolling Scanlines**
- **Scanline Curvature**

-----------------------------------------------------------------------------------------------
**[ CURVATURE ]:**
    Applies tube curvature

- **CURVATURE MODE**
  - **0 - Turn Curvature Off**
  - **1 - 2D** - Simplest and fastest curvature
  - **2 - 2D-CYL** - Simplest and fastest curvature but for a cylindrical tube like a Trinitron
  - **3 - 3D Sphere**  -  Geometric projection from the surface of a sphere to the viewport - Same as CRT-Royale
  - **4 - 3D Sphere with adjusted corner mapping** --- Very similar to #1
  - **5 - 3D Cylindrical Mapping** --- Vertically flat curvature like a Trinitron, e.g. PVM or BVM
- **2D Curvature on Long Axis** - Curvature multiple on long axis, for a horizontal aspect ratio this is the horizontal axis
- **2D Curvature on Short Axis** - Curvature multiple on short axis, for a horizontal aspect ratio this is the vertical axis
- **3D Radius** - Radius for the sphere the 3D projection is done on, values from 1-4 then to be useful
- **3D View Distance** - This is the distance of the virtual camera from the Sphere
- **3D Tilt Angle Y** - Vertical Tilt, with split screen this will tilt both screens towards or away from the center
- **CRT Curvature Scale Multiplier** - This allows reducing the curvature of the crt image to reduce Moire artifacts or just give a flat look while the bezel and black edge stays the same

-----------------------------------------------------------------------------------------------
**[ ANTI-FLICKER ]:**

Blend parts of the image which flicker on/off repeatedly between frames often used for Character's Shadow, giving a blended result.

- **Anti-Flicker ON** --- Turn the effect ON / OFF
- **Luma Difference Threshold**
  - Brightness difference required before the colors will be blended

-----------------------------------------------------------------------------------------------
**[ GRAPHICS CACHE ]:**

  * **Cache Graphics**
    * **0: OFF** - Graphics Layering and Bezel Generation are regenerated every frame
    * **1: ON** - Graphics & Bezel are generated once and cached for subsequent frames. The cache auto updates when changes in parameters are detected
  * **Cache Update Indicator**
    * **0: OFF** - Never show the red dot indicator on screen when the cache updates
    * **1: ON** - Appears whenever the graphics are regenerated and cache is updated, when caching is off or if the cache is auto-updated
    * **2: ONLY WHEN CACHE OFF** - Indicator does not appear on auto-update, It only appears when caching is off


-----------------------------------------------------------------------------------------------
**[ A/B SPLITSCREEN COMPARE ]:**

- **Show:  CRT SHADER | ORIGINAL**
  - Switch between showing the raw game image or the complete CRT image
- **Compare Area:  LEFT | RIGHT | TOP | BOTTOM**
  - Which part of the screen should we show the comparison image
- **Splitscreen Position**
  - Shift the split in the middle of the screen towards the one side or the other
- **Freeze CRT Tube (Freeze Left, New changes on Right)**
  - Freeze an image of the CRT tube at the current time on one side of the screen, while the area on the other side keeps updating to user changes
- **Freeze Graphics (Freeze Left, New changes on Right)**
  - Freeze an image of the Graphics at the current time ion one side of the screen, while the area on the other side keeps updating to user changes


- **[ SHOW PASS (For Debugging) ]:**

Shows the results at different stages of the shader chain

- **Show Pass Index**
    * **0: END** - Result of the final pass
    * **1: REFLECTION** - Shows te Reflection pass (includes final image of the tube)
    * **2: TUBE** - Shows the pass with the tube effects applied, E.G. vignette, diffuse, static reflection
    * **3: CRT** - The CRT effect only
    * **4: INTERLACE** - The stage before the CRT which has interlacing effects added
    * **5: COLOR CORRECT & UPSCALE** - Color Correct, and upscaling like ScaleFx or SuperXBR are applied here
    * **6: DEDITHER** - ADV passes only, at this point dithering should be blended together
    * **7: DREZ** - DREZ passes only, shows result after the downsampling pass
    * **8: CORE** - This is the initial input into the shader pipeline
    * **9: LAYERS TOP** - Just the pass which composites all the layers which are in front of the CRT tube
    * **10: LAYERS BOTTOM** - Just the pass which composites all the layers which are behind the CRT tube
- **Apply Screen Scale & Curvature to Unscaled Passes**
    * **0: Show exact output of the chosen pass**
    * **1: Apply scale and curvature to the output of the chosen pass so it is easier to directly compare between earlier and later passes**
- **Show Alpha Channel**
    Show the alpha channel of the pass. E.G. the alpha channel of the Layers Bottom pass is the reflection mask

-----------------------------------------------------------------------------------------------
**[ SCREEN VIGNETTE ]:**

- **Use Vignette**
  - Fade out the game screen as we move away from the center of the screen
- **Amount (Strength)** - Overall Darkness
- **Corner Amount (Power)** - Darkeness towards the edges
- **Show Vignette in Reflection** - Darken the reflection or not


-----------------------------------------------------------------------------------------------
**[ MONOCHROME ]:** --- Have the screen act as if it is a monochrome CRT

- **Monochrome Color:**
  - 0: OFF
  - 1: BLACK & WHITE
  - 2: AMBER
  - 3: GREEN
- **Monochrome Gamma**
- **Monochrome Hue Offset**
- **Monochrome Saturation**


-----------------------------------------------------------------------------------------------
**[ TUBE ASPECT & EMPTY SPACE ]:**

- **Tube Aspect** - Default is 2: Explicit
  - 0 - EVEN - If there is empty tube space added it will be added evenly on all sides. If there is no empty space then the aspect ratio will match the screen aspect.
  - 1 - SCREEN - Have the tube keep the same aspect of the game screen as empty space is added
  - 2 - EXPLICIT - Sets the explicit aspect ratio of the tube
- **Explicit Tube Aspect**
  - Aspect used when Tube Aspect is set to Explicit
- **Empty Tube Thickness**
  - Amount of Empty tube around the game image
- **Empty Tube Thicknes X Scale**
  - Amount of Empty tube around the game image horizontally
- **Screen (Game Image) Corner Radius Scale**
  - How round are the corners of the CRT image on top of empty area of the tube


-----------------------------------------------------------------------------------------------
**[ TUBE DIFFUSE IMAGE ]:**

The color/texture of the tube which appears behind the CRT image
- **Tube Diffuse Color**
  - 0: Black - Game image is shown over 100% black
  - 1: Image - Shows an grayish image of crt tube with lighting
  - 2: Transparent - See through the CRT tube to the background
- **Colorize On** - Colorize the image
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
  - A multiplier on the amount of global ambient lighting applied
- **Scale**
- **Scale X**

-----------------------------------------------------------------------------------------------
**[ TUBE SHADOW IMAGE ]:**

Adds a shadow on top of the tube diffuse image and colored gel
- **Tube Shadow Image - OFF | ON**
  - Apply a shadow on the top of the tube diffuse coloring
- **Opacity**
  - Opacity of the shadow, and how dark the shadow is
- **Position X**
- **Position Y**
- **Scale X**
- **Scale Y** - Scales shadow from the top of the tube
- **Curvature Scale** - How much curvature is applied to the shadow, more curvature has the effect of making it look like the light is higher relative to the tube/bezel


-----------------------------------------------------------------------------------------------
**[ CRT ON TUBE DIFFUSE BLENDING ]:**

- **Tube Opacity**
  - Opacity of the tube, with opacity of 0 you will see through to the background, good for things like Tron's Deadly Discs
- **CRT On Tube Diffuse Blend Mode** - How to apply the CRT (Game Image) on top of the tube
  - 0: OFF - Don't apply the game image
  - 1: ADD - Apply the game image additively this is the normal behavior
  - 2: Multiply - Darken the tube diffuse image with the game image
- **CRT On Tube Diffuse Blend Amount** - Opacity or how much of the image to apply to the tube

-----------------------------------------------------------------------------------------------
**[ TUBE COLORED GEL IMAGE ]:**
    Colored effect added on top of the CRT image to tint it

- **Use Tube Colored Gel** - Apply the image or not
- **Dual Screen Visibility** - Which screens to show the colored Gel
  - 0: Both Screens
  - 1: Only the First Screen
  - 2: Only the Second Screen
- **Multiply Blend Amount** - Image applied like a colored gel in photography
  - Used to make vector games which output black and white colored, E.G Battlezone
- **Additive Blend Amount** - Image applied as a brightening of the tube area
  - Used to add color to the screen as if it was the gel being diffusely lit from outside the monitor
- **Normal Blend Amount** - Used for the more opaque parts of a gel image like for the Vectrex
- **Normal Blend Transparency Threshold** - Adjusts at what transparency of the image the area should be appear fully transparent
- **Normal Multiply by Tube Diffuse Shading** - Darken the gel with the tube diffuse image, allows you to add shading to the gel image
- **Normal Blend Brightness** - Brightness for these more opaque parts
- **Ambient Lighting Multiplier** - How much of the global ambient lighting to apply
- **Ambient 2nd Image Lighting Multiplier** - How much of the global 2nd ambient lighting to apply
- **Scale** - Scale the colored gel image
- **Flip Horizontal** - Flip the colored gel image Horizontally
- **Flip Vertical** - Flip the colored gel image Vertically
- **Show CRT on Top of Colored Gel Normal** - Put the CRT image on top of the gel image so it is not color shifted or obscured.

-----------------------------------------------------------------------------------------------
**[ TUBE STATIC REFLECTION IMAGE  ]:**
- **Use Tube Static Reflection Image - OFF | ON** --- Apply the effect or not
- **Opacity** --- This is the shine on the tube which imitates reflection from the environment
- **Dual Screen Visibility** --- Which screen the static reflection is shown
  - 0: Both Screens**
  - 1: Only the First Screen
  - 2: Only the Second Screen
- **Ambient Lighting Multiplier** --- How much of the global ambient lighting image to apply, default is 100
- **Ambient 2nd Image Lighting Multiplier** --- How much of the global 2nd ambient lighting image to apply, default is 0
- **Scale** --- Scales the tube reflection image from the center of the tube
- **Shadow Opacity** --- How much of the shadow should appear on the static reflection image


-----------------------------------------------------------------------------------------------
**[ SCREEN BLACK EDGE ]:**

- **Global Corner Radius** --- Global radius of all corners before their own multipliers are applied
- **Black Edge Corner Radius Scale** --- the roundness of the corner of the screen area
- **Black Edge Sharpness** --- Blends the edge of the game screen image to black, lower values will fade the edge
- **Black Edge Curvature Scale Multiplier** --- How much the black edge will follow the screen curvature
- **Black Edge Thickness** --- How thick the black edge is on the edge of the tube
  - Set this to 0 or less to remove the black edge
- **Black Edge Thickness X Scale** --- Scale the thickness on the left and right edge


-----------------------------------------------------------------------------------------------
**[ DUAL SCREEN ]:**

- **Dual Screen Mode**
  - 0 - OFF - Single Screen
  - 1 - VERTICAL - Split into 2 screens one on the top and one on the bottom
  - 2 - HORIZONTAL - Split into 2 screens one on the left and one on the right

- **Core Image Split Mode**
  - 0 - AUTO - Split in the same direction as the dual screen mode
  - 1 - VERTICAL
  - 2 - HORIZONTAL
- **Core Image Split Offset**
  - Adjusts where we split the core image into two
  - This is an offset in pixels from the center
   the screen
  - Value in Pixels
- **Viewport Split Offset**
  - Sets where the viewport split placed. The split defines the area where one screen or the other appears
  - Value is a percentage from the center of the screen
- **Scale Screens from Center of Split**
  - 0 - OFF - The screens will scale their center
  - 1 - ON - The screens will scale from the split position rather than from their own centers
- **Position Offset Between Screens**
  - Positive values move screens apart
  - Negative values move screens closer to each other
- **2nd Screen Aspect Ratio Mode**
  - 0 - Use the same Aspect ratio as the first Screen
  - 1 - PAR (Uses the square pixel aspect of the bottom screen's resolution)
- **2nd Screen Use Independent Scale** - Don't affect the second screen with the scale of the first
- **2nd Screen Scale Offset** - Increase or Decrease scale of second screen
- **2nd Screen Pos X** - Move the second screen Horizontally
- **2nd Screen Pos Y** - Move the second screen Vertically
- **2nd Screen Crop Zoom %** - Crop on all sides at once
- **2nd Screen Crop Overscan Top**
- **2nd Screen Crop Overscan Bottom**
- **2nd Screen Crop Overscan Left**
- **2nd Screen Crop Overscan Right**


-----------------------------------------------------------------------------------------------
**[ REFLECTION POSITION & SCALE ]:**

- **Screen Reflection Scale**
  - Scales the reflection from the center
  - With a larger scale, the image from the screen will appear without mirroring, like the Big Blur preset
- **Screen Reflection Pos X**
  - Shift the reflection left or right
- **Screen Reflection Pos Y**
  - Shift the reflection up or down


-----------------------------------------------------------------------------------------------
**[ AMBIENT LIGHTING IMAGE 1 ]:**

- **Hue**
  - Shift the hue of the color of the image
- **Saturation**
  - How saturated the ambient lighting is
- **Value**
  - How dark or bright the ambient lighting is
- **Contrast**
  - Contrast in the ambient lighting
- **Scale Aspect**
  - **MATCH VIEWPORT** - Stretch the width of the image to the full viewport
  - **USE TEXURE ASPECT** - The base aspect of the image will match the image file aspect
- **Scale With Zoom**
  - **OFF** Don't scale the ambient lighting with the viewport Zoom
  - **ON** Scale with the viewport Zoom
- **Scale Offset**
  - Scale the lighting image in both directions
- **Scale Offset X**
  - Scale the lighting image in the horizontal direction
- **Rotate**
  - Rotate the lighting image
- **Mirror Horizontal**
  - Flip the ambient lighting left to right
- **Position X**
  - Move the image left and right
- **Position Y**
  - Move the image up and down


-----------------------------------------------------------------------------------------------
**[ AMBIENT LIGHTING IMAGE 2 ]:** - Has the same parameters as Ambient Image 1


-----------------------------------------------------------------------------------------------
**[ BEZEL INDEPENDENT SCALE & CURVATURE ]:**

- **Use Independent Scale & Curvature**
  - Scale the bezel independent of the screen
- **Independent Scale**
  - Scale of the bezel when scale from image is not used
  - Base scale for the bezel default is 82.97 which is the same as the default screen size
- **Use Independent Curvature** --- Define curvature separately from the screen
- **Independent Curvature X** --- Horizontal curvature for the bezel when independent
- **Independent Curvature Y** --- Vertical curvature for the bezel when independent


-----------------------------------------------------------------------------------------------
**[ BEZEL GENERAL ]:**

- **Opacity**
  - At 100 the bezel is fully visible

- **Blend Mode**
  - **0 - Off**
  - **1 - Normal Blending**
  - **2 - Additive Blending** - Added on as added with a projector
  - **3 - Multiply Blending** - Image is applied by darkening the under layer

- **Width**
  - Thickness of the bezel on the sides of the tube, default is 125

- **Height**
  - Thickness of the bezel on the top and bottom of the tube, default is 100

- **Scale Offset**
  - Scale offset of the Bezel & Frame from its default position

- **Inner Curvature Scale Multiplier**
  - How much the bezel's curvature follows the tube curvature

- **Inner Corner Radius Scale** - Def 50
  - Roundness of the inner corner of the bezel, it is a multiplier of the roundness of the screen corner
  - 100 gives you the same roundness as the screen corner

- **Inner Edge Thickness**
  - Thickness of edge of inner, default 100

- **Inner Edge Sharpness** - Def 90

- **Outer Corner Radius Scale** - Def 100
  - Roundness of the inner corner of the bezel, it is a multiplier of the roundness of the screen corner
  - 100 gives you the same roundness as the screen corner

- **Outer Curvature Scale**
  - Amount of curvature on the outside of the bezel it is a multiplier of the roundness of the screen corner
  - Default is 0 which gives a straight edge of the outside of the bezel

- **Outer Edge Position Y**
  - This moves the outer edge of the bezel and the frame up and down

- **Noise**
  - Noise or speckles in the color, default is 30

- **Opacity of Shadow from Bezel on Tube**
  - How much of a darkness from the bezel onto the illuminated screen
  - Only visible when the black ring around the screen is reduced so that the bezel is almost on top of the screen


-----------------------------------------------------------------------------------------------
**[ BEZEL BRIGHTNESS ]:**

- **Base Brightness**
  - Brightness of the bezel, the default is 30 so only 30% brightness

- **Top Multiplier**
  - An adjustment over the base brightness for the top

- **Bottom Multiplier**
  - An adjustment over the base brightness for the bottom

- **Sides Multiplier**
  - An adjustment over the base brightness for both sides

- **Left Side Multiplier**
  - An adjustment over the base brightness for the left side

- **Right Side Multiplier**
  - An adjustment over the base brightness for the right side

- **Highlight**
  - The highlight or shininess in the middle of the bezel



-----------------------------------------------------------------------------------------------
**[ BEZEL COLOR ]:**

- **Hue**
  - The hue or "color" of the bezel E.G. Blue vs Orange
- **Saturation**
  - How saturated or strong the color is
- **Value/Brightness**
  - The brightness of the color, default is 10 which is 10%


-----------------------------------------------------------------------------------------------
**[ FRAME COLOR ]:**

- **Use Inependent Frame Color**
  - 0 by default, when turned on it uses a different color than the bezel color
- **Hue**
  - The hue or "color" of the bezel and frame E.G. Blue vs Orange
- **Saturation**
  - How saturated or strong the color is
- **Value/Brightness**
  - The brightness of the color, default is 10 which is 10%


-----------------------------------------------------------------------------------------------
**[ FRAME GENERAL ]:**

- **Opacity**
  - Opacity of the frame default is 100 which means it is fully visible
- **Texture Overlay Opacity (Highlight)**
  - Opacity of the texture applied on top of base color of the frame
  - The default texture is a white highlight and so adds a highlight effect to the frame giving it a bit more dimension
- **Texture Overlay Blend Mode** - Default is 2 so it is additive
  - **0 - Off**
  - **1 - Normal Blending**
  - **2 - Additive Blending** - Added on as added with a projector
  - **3 - Multiply Blending** - Applied by darkening what is underneath
- **Noise**
  - Noise or speckles in the color, default is 30
- **Inner Edge Thickness**
  - Thickness of the inner edge of the frame
- **Inner Corner Radius Scale**
  - Roundness of the inner corner,
- **Frame Thickness**
  - Base thickness of the frame
- **Frame Thickness Scale X**
  - Adjusts the frame thickness of frame at the left and right
- **Frame Outer Pos Y**
  - Shift the outside of the frame up and down which can make the top of the frame larger than the bottom or vice versa
- **Frame Outer Curvature Scale**
  - Curvature of the outside of the frame, at 100 it will match the curvature of the inside of the frame
- **Outer Corner Radius**
  - Roundness of the frame outer corner
- **Outer Edge Thickness**
  - Thickness of the shading on the outer edge
- **Outer Edge Shading**
  - Controls the darkness of the shading on the outer edge
- **Shadow Opacity**
  - Controls the darkness of the shadow under and around the frame
- **Shadow Width**
  - Controls how wide the shadow is around the frame

-----------------------------------------------------------------------------------------------
**[ REFLECTION ]:**

- **Global Amount**
  - Overall multiplier on the amount of reflection shown
- **Global Gamma Adjust**
  - Gamma adjustment on the reflection, allows you to reduce the amount of reflection in dark areas, or reduce contrast in the reflections
- **Direct Reflection**
  - Amount of the most detailed reflection shown
- **Diffused Reflection**
  - Amount of a very blurry and diffused reflection shown, helps blend between the main reflection to make a more natural effect
- **FullScreen Glow**
  - Amount of a very diffused reflection shown which mimics lighting from the overall brightness of the screen
- **FullScreen Glow Gamma**
  - Adjust the gamma of the full screen glow, this has the effect of controlling how bright the screen needs to be to see the fullscreen glow effect
- **Bezel Inner Edge Amount**
  - How much reflection on the small inner edge right at the outside of the tube
- **Bezel Inner Edge Fullscreen Glow**
  - Same as above but a non-directional glow from all over the screen
- **Frame Inner Edge Amount**
- **Frame Inner Edge Sharpness**
  - How soft or sharp the reflection is at the inner edge of the frame

-----------------------------------------------------------------------------------------------
**[ REFLECTION FADE ]:**

- **Fade Amount**
  - At 100 the reflection fades out as it comes away from the screen, at 0 the reflection does not fade and is full strength everywhere
- **Radial Fade Width**
  - The distance away from the sides of the screen where the reflection to completely fades out
- **Radial Fade Height**
  - The distance away from the top and bottom of the screen where the reflection to completely fades out
- **Lateral Outer Fade Position**
  - When the reflection fades out towards the corners, for example on the bottom bezel the reflection fades out towards the left and right. The position where the fade starts.
- **Lateral Outer Fade Distance**
  - For the lateral fade the distance for it to fade out


-----------------------------------------------------------------------------------------------
**[ REFLECTION CORNER ]:**

- **Corner Fade**
  - How much should the corner fade out
- **Corner Fade Distance**
  - The distance from the corner where the reflection fully fades out
- **Corner Inner Spread**
  - How much the inner corner reflection spreads out
- **Corner Outer Spread**
  - How much the outer corner reflection spreads out
- **Corner Rotation Offset Top**
  - Adjust the rotation of the highlight in the top corners
- **Corner Rotation Offset Bottom**
  - Adjust the rotation of the highlight in the bottom corners
- **Corner Spread Falloff**
  - Controls the profile of the falloff, small values make falloff faster near the center.

-----------------------------------------------------------------------------------------------
**[ REFLECTION BLUR ]:**

- **Blur Samples - 0 for OFF**
  - Default is 12
- **Min Blur**
  - What is the least amount of blur in the reflection this is nearest the screen
- **Max Blur**
  - The highest amount of blur in the reflection, this is the farther away from the screen

-----------------------------------------------------------------------------------------------
**[ REFLECTION NOISE ]:**

- **Noise Amount**
  - How much noise seen in the reflection, gives the effect of the scattered reflection of a slightly textured surface
- **Noise Samples (0 for OFF)**
  - How many samples taken for the effect, more samples the smoother the effect, fewer samples makes the surface look more like it has little bumps in it
- **Sample Distance**
  - What is the farthest distance away from the point being drawn where the scattered sample come from



-----------------------------------------------------------------------------------------------
**[ REFLECTION MASK IMAGE - Only in Image Layer Presets ]:**

- **Reflection Image Mask Amount**
  - How much the image darkens the reflection
- **Follow Layer**
  - Which layer should the image mask match, Default is 4, Following the Device
  - **0 - FULL**
  - **1 - TUBE**
  - **2 - BEZEL**
  - **3 - BG**
  - **4 - DEVICE**
  - **5 - DECAL**
  - **6 - CAB**
  - **7 - TOP**
- **Follow Mode**
  - Should the image follow exactly
  - **0 - FOLLOW SCALE & POS** - Follows only position and orientation
  - **1 - FOLLOW EXACTLY** - Follows curvature and split mode
- **Mask Brightness** - Brighten's the mask which brightens the reflection
- **Mask Black Level** - Increases the black level so that the darker areas of the maskbecome  darker, can help increase contrast
- **Mipmapping Blend Bias (Affects Sharpness)** - Makes the effect of the mask blurrier or sharper

-----------------------------------------------------------------------------------------------
**[ REFLECTION GLASS ]:**

- **Glass Border ON (For Glass Preset)**
  - Changes the appearance of the reflection to look like the glass effect, this is here for technical reasons, not very useful to change interactively
- **Glass Reflection Vignette**
  - Adds a vignette over the entire viewport to darken the areas as it goes towards the edges used to darken the reflection in the glass preset
- **Glass Reflection Vignette Size**



-----------------------------------------------------------------------------------------------
## **POTATO Presets Only**

-----------------------------------------------------------------------------------------------
**[ POTATO BACKGROUND IMAGE LAYER ]:**

- **Background Blend Mode** - Default is Additive

  - **OFF** - Image is not applied
  - **NORMAL**
  - **ADD** - Adds the image as if it is being projected on top
  - **MULTIPLY** - Image is applied as if it was a colored plastic film

- **Opacity**

- **Brightness**

- **Show Background**

  - **0 - UNDER SCREEN** - Background is applied under the screen and Since the default blend mode is additive this gives a backdrop effect. E.G. Tron's Deadly Discs
  - **1 - OVER SCREEN** - Background is applied on top of the screen



-----------------------------------------------------------------------------------------------
## **GLASS Presets Only**

-----------------------------------------------------------------------------------------------
**[ GLASS BACKGROUND IMAGE ]:**

- **Background Image Opacity**

- **Background Blend Mode** - Default is Additive

  - **Off** - Image is not applied

  - **Normal**

  - **Additive** adds the image as if it is being projected on top

  - **Multiply** applies as if it was a colored plastic film


-----------------------------------------------------------------------------------------------
**[ LAYER ORDER ]:**

Layer order adjusts the order in which the layers are composited or "layered" on top of each other, the index 0 is the bottom or base layer. If two layers are given the same index they fall back to being composited in the order seen here.

- **Background Image**
- **Viewport Vignette**
- **LED Image**
- **Device Image**
- **Device LED Image**
- **CRT Screen**
- **Decal Image**
- **Cabinet Glass Image**
- **Top Image**

-----------------------------------------------------------------------------------------------
**[ CUTOUT ]:**

Used to cut a rectangular area from the layers, for example cutting out the hole in the bezel art

- **Scale Mode**
  - Controls if this layer's scaling follows another layer
  - **0 - Full** - Scale to the viewport
  - **1 - Tube** - Follow the Tube Scaling
  - **2 - Bezel** - Follow the Bezel Scaling
  - **3 - Background** - Follow the Background Image Scaling
  - **4 - Bezel Image** - Follow the Bezel Image Scaling
  - **5 - Decal Image** - Follow the Decal Image Scaling
- **Scale**
  - Scales cutout in both directions
- **Scale X**
  - Scales cutout horizontally
- **Position Y**
  - Moves the cutout vertically
- **Corner Radius - Def 0**
  - Rounds the corner of the cutout

-----------------------------------------------------------------------------------------------
**[ MASK DEBUG ]:**

- **Mask** - Show the mask as a semi transparent color for the:

  - **-1 - Cut Out**
  - **0 - ALL** - Whole viewport
  - **1 - Screen** - Illuminated area of the tube
  - **2 - Tube**
  - **2 - Bezel and Inward** - Bezel and inward
  - **3 - Bezel**
  - **4 - Bezel +** - Bezel and outward
  - **5 - Frame**
  - **6 - Frame +** - Frame and outward
  - **7 - Background**  - Outside the frame



## ***Common Layer Parameters***

	*Many parameters which repeated from layer to layer, their description is shown here*

- ***Opacity***
  - *Opacity multiplier of the layer being applied. 0 means we will not see the layer because it is fully transparent*

- ***Colorize On*** - Turns on the Colorization
- ***Hue*** - The hue adjustement, eg more blue vs red
- ***Saturation***
- ***Brightness***
  - *Adjust Brightness of the Layer, 100 is no change*
- ***Gamma***

- ***Ambient Lighting Multiplier***
  - How much of the global ambient lighting to add to the layer

- ***Apply Ambient Lighting in ADD Blend Mode***
    - By default when a layer is in ADD mode ambient lighting is not applied to the image, this works well for things like LEDs
    - When **Apply Ambient Lighting in ADD Blend Mode** is set to 1 the ambient lighting will be applied when in ADD blend mode

- ***Blend Mode*** *- How the image is applied to the layer underneath Default is 1: Normal Blending*

  - ***0 - Off*** *- The layer is not shown*
  - ***1 - Normal Blending***
  - ***2 - Additive Blending*** *- Applied additively to brighten what's underneath*
  - ***3 - Multiply Blending*** *- Applied to as a darkening of what is underneath*

- ***Source Matte Color***
  - *Controls how the image transparency is interpreted based on the matte color used when the image was stored (what color the image is blended with in the transparent area).*
  - *Used to remove white fringing on the edges around transparent areas.*
  - ***0 - Black***
    - *The color in the transparent area was black*. Technically this is called Premultiplied alpha.
  - ***1 - White***
    - *The color in the transparent area was white, Use this if you see white fringing on the edges of the transparency where there should be none.*
  - ***2 - None***
    - *The image was not blended with any matte color, the only transparency info is in the alpha channel*

- **Mask** - Mask the layer with the area inside the:
  - ***0 - ALL*** *- Whole viewport*
  - ***1 - Screen*** *- Illuminated area of the tube*
  - ***2 - Tube*** *- Inside the tube
  - ***2 - Bezel and Inward*** *- Bezel and inward*
  - ***3 - Bezel*** *- Bezel
  - ***4 - Bezel +*** *- Bezel and outward*
  - ***5 - Frame*** *- Frame
  - ***6 - Frame +*** *- Frame and outward*
  - ***7 - Background***  *- Outside the frame*

- ***Cutout Mask***
  - ***0 - OFF*** *- Don't cut out any area of the layer*
  - ***1 - ON*** *- Make the area of the layer INSIDE the cutout mask transparent*
  - ***2 - Invert*** *- Make the area of the layer OUTSIDE the cutout mask transparent*

- ***Dual Screen Visibility*** - Where to show this image
  - ***0 - Show on Both Screens***
  - ***1 - Show only on Screen 1***
  - ***2 - Show only on Screen 2***

- ***Follow Layer***
  - *Controls if this layer follows another layer's scaling*
  - ***Full / Fullscreen*** *- Scale to the viewport*
  - ***Tube*** *- Follow the Tube Scaling*
  - ***Bezel*** *- Follow the Bezel Scaling*
  - ***Background*** *- Follow the Background Image Scaling*
  - ***Bezel Image*** *- Follow the Bezel Image Scaling*
  - ***Decal Image*** *- Follow the Decal Image Scaling*
  - ***Top Extra Image*** *- Follow the Top Extra Image Scaling*

- ***Follow Mode***
  - *Controls if this layer follows another layer's scaling*
  - ***Follow Scale and Pos*** *- Follow Scale and Position, ignore split*
  - ***Follow Exactly*** *- Match the layer exactly including split*

- ***Follow Full also follows Zoom***
  - When the layer Follow Layer is in **FULL** mode this controls if the layer should scale with the global zoom or not, this defaults to ON

- ***Image Fill Mode***
  - **0 - USE TEXURE ASPECT** --- Keeps the aspect of the teture
  - **1 - SPLIT HORIZONTAL** --- Splits the image in the center and slide it out towards the sides to match the required aspect
  - **2 - STRETCH** --- Stretch the image across the whole area to match the required aspect

- ***Split Mode Preserve Center %*** --- Preserves a part of the center of the graphic when split is used
  - One usage is to have a logo in the center of the bottom of the monitor graphic and reserve space for this

- ***Split Mode Repeat Width %*** --- Width of repeating texture in exposed area
  - When this is 0 repeat is off

- **Scale** --- *Scales image layer equally in both directions*
- **Scale X** --- *Scales image layer horizontally*
- **Position X** --- *Moves the image layer horizontally*
- **Position Y** --- *Moves the image layer vertically*
- **Mipmapping Blend Bias** --- *Adjusts the sharpness of the image*



-----------------------------------------------------------------------------------------------
**[ BACKGROUND LAYER ]:**

- **Opacity**
- **Colorize On**
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
- **Ambient Lighting in ADD Mode**
- **Blend Mode**
- **Source Matte Color**
- **Mask**
- **Cutout Mask**
- **Dual Screen Visibility**

**[ BACKGROUND SCALE & FOLLOW ]:**

- **Follow Layer**
  - **0 - FullScreen**
  - **1 - Tube**
  - **2 - Bezel**
- **Follow Layer**
- **Follow Full also follows Zoom**
- **Image Fill Mode**
- **Split Mode Preserve Center %**
- **Scale**
- **Scale X**
- **Position X**
- **Position Y**
- **Mirror Wrap**
  - When ON the image is wrapped when we draw out of the texture bounds
- **Mipmapping Blend Bias**

-----------------------------------------------------------------------------------------------
**[ VIEWPORT VIGNETTE LAYER ]:**

- **Opacity**
- **Mask**
- **Cutout Mask**
- **Follow Layer**
  - **0 - Full**
  - **1 - Background**
  - **2 - Tube**
  - **3 - Bezel**
- **Scale**
- **Scale X**
- **Position Y**

-----------------------------------------------------------------------------------------------
**[ LED LAYER ]:**

- **Opacity**
- **Colorize On**
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
- **Ambient Lighting in ADD Mode**
- **Blend Mode**
- **Source Matte Color**
- **Mask**
- **Cutout Mask**
- **Dual Screen Visibility**

**[ LED SCALE & FOLLOW ]:**

- **Follow Layer**
  - **0 - FullScreen**
  - **1 - Tube**
  - **2 - Bezel**
  - **3 - BG**
  - **4 - Device**
- **Follow Mode**
- **Follow Full also follows Zoom**
- **Image Fill Mode**
- **Split Mode Preserve Center %**
- **Scale**
- **Scale X**
- **Position X**
- **Position Y**
- **Mipmapping Blend Bias**

-----------------------------------------------------------------------------------------------
**[ DEVICE IMAGE LAYER ]:**

- **Opacity**
- **Colorize On**
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
- **Ambient Lighting in ADD Mode**
- **Blend Mode**
- **Source Matte Color**
- **Mask**
- **Cutout Mask**
- **Dual Screen Visibility**

**[ DEVICE SCALE & FOLLOW ]:**

- **Follow Layer**
  - **0 - FullScreen**
  - **1 - Tube**
  - **2 - Bezel**
  - **3 - BG**
- **Follow Mode**
- **Follow Full also follows Zoom**
- **Image Fill Mode**
- **Split Mode Preserve Center %**
- **Scale**
- **Scale X**
- **Position X**
- **Position Y**
- **Mipmapping Blend Bias**

-----------------------------------------------------------------------------------------------
**[ DECAL LAYER ]:**

- **Opacity**
- **Colorize On**
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
- **Ambient Lighting in ADD Mode**
- **Blend Mode**
- **Source Matte Color**
- **Mask**
- **Cutout Mask**
- **Dual Screen Visibility**

**[ DECAL SCALE & FOLLOW ]:**

- **Follow Layer**
  - **0 - FullScreen**
  - **1 - Tube**
  - **2 - Bezel**
  - **3 - BG**
  - **4 - Device**
- **Follow Mode**
- **Follow Full also follows Zoom**
- **Image Fill Mode**
- **Split Mode Preserve Center %**
- **Scale**
- **Scale X**
- **Position X**
- **Position Y**
- **Mipmapping Blend Bias**

-----------------------------------------------------------------------------------------------
**[ CABINET GLASS LAYER ]:**

- **Opacity**
- **Colorize On**
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
- **Ambient Lighting in ADD Mode**
- **Blend Mode**
- **Source Matte Color**
- **Mask**
- **Cutout Mask**
- **Dual Screen Visibility**

**[ CABINET GLASS SCALE & FOLLOW ]:**

- **Follow Layer**
  - **0 - FullScreen**
  - **1 - Tube**
  - **2 - Bezel**
  - **3 - BG**
  - **4 - Device**
  - **5 - Decal**
- **Follow Mode**
- **Follow Full also follows Zoom**
- **Image Fill Mode**
- **Split Mode Preserve Center %**
- **Scale**
- **Scale X**
- **Position X**
- **Position Y**
- **Mipmapping Blend Bias**



-----------------------------------------------------------------------------------------------
**[ TOP EXTRA LAYER ]:**

- **Opacity**
- **Colorize On**
- **Hue Offset**
- **Saturation**
- **Brightness**
- **Gamma Adjust**
- **Ambient Lighting Multiplier**
- **Ambient Lighting in ADD Mode**
- **Blend Mode**
- **Source Matte Color**
- **Mask**
- **Cutout Mask**
- **Dual Screen Visibility**

**[ TOP SCALE & FOLLOW ]:**

- **Follow Layer**
  - **0 - FullScreen**
  - **1 - Tube**
  - **2 - Bezel**
  - **3 - BG**
  - **4 - Device**
  - **5 - Decal**
- **Follow Mode**
- **Follow Full also follows Zoom**
- **Image Fill Mode**
- **Split Mode Preserve Center %**
- **Scale**
- **Scale X**
- **Position X**
- **Position Y**
- **Mirror Wrap**
  - When drawing past the edges of the texture use mirror wrapping
- **Mipmapping Blend Bias**


-----------------------------------------------------------------------------------------------
**[ INTRO SEQUENCE ]:**

	Animation sequence which plays when the content starts up, animation times are in frames. The frame rate for most games 60 fps

- **Show Intro**
  - 0 - OFF
  - 1 - When Content Loads
  - 2 - Repeat
- **Speed**
  - Overall speed of the entire intro. 1 is full speed

-----------------------------------------------------------------------------------------------
**[ INTRO LOGO ]:**

- **Logo Blend Mode**
  - 0 - Off
  - 1 - Normal Blending
  - 2 - Additive Blending - Added on as added with a projector
  - 3 - Multiply Blending - Image is applied by darkening the under layer
- **Logo Over Solid Color**
  - 0 - Off - The Logo is layered under the solid color
  - 1 - ON - The Logo is layered over the solid color
- **Logo Height (0 for exact resolution)**
- **Logo Res X**
  - X Resolution of the logo image
- **Logo Res Y**
  - Y Resolution of the logo image
- **Logo Placement **
  - 0 - Middle
  - 1 - Top Left
  - 2 - Top Right
  - 3 - Bottom Left
  - 4 - Bottom Right
- **Logo Wait Before Start Frames**
  - How many frames before it starts to fade in
- **Logo Fade In Frames**
  - How many frames to fade in
- **Logo Hold Frames**
  - How many frames to hold the image at full opacity before the fade out starts
- **Logo Fade Out Frames**
  - How many frames to fade out

-----------------------------------------------------------------------------------------------
**[ INTRO SOLID COLOR ]:**

- **Solid Color Blend Mode**
  - 0 - Off
  - 1 - Normal Blending
  - 2 - Additive Blending - Added on as added with a projector
  - 3 - Multiply Blending - Image is applied by darkening the under layer
- **Solid Color Hue**
- **Solid Color Saturation**
- **Solid Color Value**
- **Solid Color Hold Frames**
  - How many frames to hold the solid color
- **Solid Color Fade Out Frames**
  - How many frames to fade out

  **[ INTRO STATIC NOISE ]:**

- **Static Noise Blend Mode**
  - **0 - Off**
  - **1 - Normal Blending**
  - **2 - Additive Blending** - Added on as added with a projector
  - **3 - Multiply Blending** - Image is applied by darkening the under layer
- **Static Noise Hold Frames**
  - How many frames to hold the static noise
- **Static Noise Fade Out Frames**
  - How many frames to fade out

-----------------------------------------------------------------------------------------------
**[ INTRO SOLID BLACK ]:**

- **Solid Black Hold Frames**
  - How many frames to hold the solid black before the fade out starts
- **Solid Black Fade Out Frames**
  - How many frames to fade out

-----------------------------------------------------------------------------------------------
**[ --- SMOOTHING - SCALEFX ---- ]:**

**ScaleFX ON** applies a shape smoothing on the core image and creates a higher resolution smoothed image
  - After you turn this on you must increase **Core Res Sampling**, or **Downsample Blur** in the next section to see a difference
  - ScaleFX only works well when the it's input is the native res from the core
  To see the contour smoothing coming from **ScaleFX** either **Core Res Sampling**, or **Downsample Blur** must be increased from their default value.
