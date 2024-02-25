# handheld-nebula border shader

These shader presets will allow you to apply a 280x224* border around an input image from one of the following handheld cores:

* 160x144 : Nintendo - Game Boy
* 160x144 : Nintendo - Game Boy Color
* 240x160 : Nintendo - Game Boy Advance
* 160x144 : Sega - Game Gear

I've posted images of the shaders in use to [this imgur album](https://imgur.com/a/nqzDnYa).

*The Game Gear version of this shader uses a 248x224 image with a 160x144 hole cut in it, this is then stretched to "280x224" via aspect ratio settings to ensure ideal sampling.*

**Set your video aspect ration to 4:3 when using these shaders.** RetroArch's "core provided" scaling function will not automatically set a correct aspect ratio for the output image, as it will only take the handheld core's reported resolution into account and not the output image's 280x224 resolution.

Please note that RetroArch's integer scaling function will not automatically set a correct integer scale for the output image, as it will only take the handheld core's reported resolution into account, and not the output image's 280x224 resolution.

One option to display it integer scaled is to set a custom viewport size, either with the Custom Ratio menu option or defining custom_viewport_width and custom_viewport_height in the config file, setting Aspect Ratio Index to "Custom", and enabling Integer Scaling to center the image automatically.

The other option is to use one of the auto-box shaders in the last pass of the shader chain to integer scale or center the output of the border shader within the viewport. You may need to disable  RetroArch's Integer Scaling and set the Aspect Ratio equal to your screen aspect ratio to see the whole image.

A template image is included, you can swap it out with another 280x224 image in png format with the center XY pixels transparent per the relevant handheld. The default image is [“Cosmic Cliffs” in the Carina Nebula](https://webbtelescope.org/contents/media/images/2022/031/01G77PKB8NKR7S8Z6HBXMYATGJ) taken by the James Webb Space Telescope. It has been cropped to a 4:3 aspect ratio, then downsampled to 256x224.

Here are [Copyright Details](https://webbtelescope.org/copyright) for the work, and an excerpt in case that URL goes down.

> Unless otherwise stated, all material on the site was produced by NASA and the Space Telescope Science Institute (STScI). Material on this site produced by STScI was created, authored, and/or prepared for NASA under Contract NAS5-03127. Unless otherwise specifically stated, no claim to copyright is being asserted by STScI and material on this site may be freely used as in the public domain in accordance with NASA's contract. However, it is requested that in any subsequent use of this work NASA and STScI be given appropriate acknowledgement.