Unfortunately d3d10, d3d11 and d3d12 output drivera in Retroarch has some bugs preventing
koko-aio from running properly or running at all.

If you absolutely need to use one of those output drivers (xbox user?),
then you NEED to open the file located in config\config-static.inc
with a text editor and turn the line:

// #define D3D_WORKAROUND
into:
#define D3D_WORKAROUND

So, removing the leading "//", and save it.

That way some workarounds will be activated and you should be able to run
koko-aio on your xbox or with a direct3d output driver.

As of today doing that will disable:
	* Performance optimizations when drawing anything outside the game content area
	* Delta render
	* Luminosity tied zoom
	* Leds scene change detection
	

