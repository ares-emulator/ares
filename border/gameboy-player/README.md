# Game Boy Player border shader

These shader presets will allow you to apply a 608x448 Game Boy Player border around a 240x160 input image from a Game Boy Advance core, with the input being scaled 2x to fit the border.

Please note that RetroArch's integer scaling function will not automatically set a correct integer scale for the output image, as it will only take the Game Boy Advance core's reported resolution of 240x160 into account, and not the output image's 608x448 resolution.

One option to display it integer scaled is to set a custom viewport size, either with the Custom Ratio menu option or defining custom_viewport_width and custom_viewport_height in the config file, setting Aspect Ratio Index to "Custom", and enabling Integer Scaling to center the image automatically.

The other option is to use one of the auto-box shaders in the last pass of the shader chain to integer scale or center the output of the border shader within the viewport. You may need to disable RetroArch's Integer Scaling and set the Aspect Ratio equal to your screen aspect ratio to see the whole image.

An example border is included, you can swap it out with another 608x448 Game Boy Player border in png format with the center 240x160 transparent. The borders included with this shader were ripped from a Game Boy Player disc by a kind anonymous user and were edited to appear correct with a GBA image fully shown.

Using CRT-Royale with this border shader may result in flickering. This is due to it displaying the border shader as 480i, due to the output image being greater than 400px vertically. Disable interlacing detection in CRT Royale's user-settings.h if want 480p instead.