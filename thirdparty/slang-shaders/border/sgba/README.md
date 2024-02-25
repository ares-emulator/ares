# Super Game Boy Advance border shader

These shader presets will allow you to apply a 320x240 Super Game Boy Advance border around a 240x160 input image from a Game Boy Advance core.

Please note that RetroArch's integer scaling function will not automatically set a correct integer scale for the output image, as it will only take the Game Boy Advance core's reported resolution of 240x160 into account, and not the output image's 320x240 resolution.

One option to display it integer scaled is to set a custom viewport size, either with the Custom Ratio menu option or defining custom_viewport_width and custom_viewport_height in the config file, setting Aspect Ratio Index to "Custom", and enabling Integer Scaling to center the image automatically.

The other option is to use one of the auto-box shaders in the last pass of the shader chain to integer scale or center the output of the border shader within the viewport. You may need to disable RetroArch's Integer Scaling and set the Aspect Ratio equal to your screen aspect ratio to see the whole image.

An example border is included, you can swap it out with another 320x240 Super Game Boy Advance border in png format with the center 240x160 transparent. The borders included with this shader were created by EndUser based on a concept by BLUEamnesiac.