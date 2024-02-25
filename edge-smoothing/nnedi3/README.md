NNEDI3 is a neural network based image doubler. This is a port of [MPV's GLSL implementation](https://github.com/bjin/mpv-prescalers/tree/master) for use with the emulator front-end Retroarch.

[Some examples of NNEDI3 scaling to 4x using 32 neurons in RGB](http://screenshotcomparison.com/comparison/204717/)

# Usage

Place the files in a folder in Retroarch\shaders\shaders_slang\. Make sure you are using the vulkan renderer in the Retroarch config.

The slangp files are only examples. You can configure them hundreds of different ways to try and find the right balance between image quality and performance. 

NNEDI3 is an image doubler and can only scale by powers of two. It also has 5 different quality settings (16, 32, 64, 128, and 256 neurons). Each increase in neurons doubles the amount of processing that needs to be done. There isn't a massive difference between the quality settings, but 16 neurons should look the worst and 256 neurons should look the best.

Since NNEDI3 is rather slow, it can be useful to do a RGB to YUV conversion and scale the luma using NNEDI3 and the chroma using a different algorithm like Jinc. You can also scale the chroma with NNEDI3 at a different number of neurons, but that requires modifying a few of the shaders to use different passes as their sources.

# Filenames

What various suffixes in the filenames mean:

* -luma: Triggered on luma channel only.
* -rgb: Triggered on the red, green, and blue channels (or y, u, and v if a conversion has been done).
* -#x: The amount of upscaling being done.
* -nns###: The amount of neurons being used for scaling (16, 32, 64, 128, or 256).
* -cshift: Used to correct the 0.5 pixel center shift introduced by NNEDI3.
* -win8x4: Uses a local sampling window size of 8x4.
* -pass#: NNEDI3 requires two passes to double an image. Pass 1 performs vertical scaling, and pass 2 does horizontal scaling.

For example:
* 'nnedi3-nns32-2x-rgb-nns32-4x-luma.slangp': Scale from 1x to 2x using NNEDI3 on all channels with 32 neurons. Then scale from 2x to 4x using NNEDI3 with 32 neurons only on the luma channel. The chroma channels are scaled from 2x to 4x with another algorithm.
* 'nnedi3-nns64-2x-nns32-4x-nns16-8x-rgb.slangp':  Scale from 1x to 2x using NNEDI3 on all channels with 64 neurons. Then scale from 2x to 4x using NNEDI3 on all channels with 32 neurons. Then scale from 4x to 8x using NNEDI3 on all channels with 16 neurons.
* 'nnedi3-nns64-4x-luma': Scale from 1x to 2x using NNEDI3 with 64 neurons on the luma channel. Then scale from 2x to 4x using NNEDI3 with 64 neurons on the luma channel. The chroma channels are scaled with another algorithm.

# Notes

* Shaders with larger numbers of neurons will be slower to compile since all the neural network's floating point weights are baked into the code.
* NNEDI3 causes a slight 0.5 pixel center shift in the image that needs to be corrected after every doubling. jinc2-cshift-luma.slang and jinc2-cshift-rgb.slang correct this slight shift.
* Clamp_to_edge is needed while scaling in YUV or there will be annoying border artifacts after scaling.
* I didn't port the 8x6 windowed versions of NNEDI3 since they don't seem to offer any real quality increase.