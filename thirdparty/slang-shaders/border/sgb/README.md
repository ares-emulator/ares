# Super Game Boy border shader

These shader presets will allow you to apply a 256x224 Super Game Boy border around a 160x144 input image from a Game Boy core.

Please note that RetroArch's integer scaling function will not automatically set a correct integer scale for the output image, as it will only take the Game Boy core's reported resolution of 160x144 into account, and not the output image's 256x224 resolution.

One option to display it integer scaled is to set a custom viewport size, either with the Custom Ratio menu option or defining custom_viewport_width and custom_viewport_height in the config file, setting Aspect Ratio Index to "Custom", and enabling Integer Scaling to center the image automatically.

The other option is to use one of the auto-box shaders in the last pass of the shader chain to integer scale or center the output of the border shader within the viewport. You may need to disable  RetroArch's Integer Scaling and set the Aspect Ratio equal to your screen aspect ratio to see the whole image.

An example border is included, you can swap it out with another 256x224 Super Game Boy border in png format with the center 160x144 transparent. Borders derived from an emulator screenshot may be shifted up one pixel and off center, so you would need to shift it down one pixel to center it. The "Super Game Boy Color" borders were created by BLUEamnesiac.