
# Broken Shader Presets

The following shaders are known to be broken due to various issues. 

This list is updated as of [slang-shaders@`33876b3`](https://github.com/libretro/slang-shaders/commit/33876b3578baac8302b6189ac7acbb052013919e)

## Broken due to parsing errors
librashader's preset parser is somewhat stricter than RetroArch in what it accepts. All shaders and textures in a preset must 
resolve to a fully canonical path to properly parse. The following shaders have broken paths.

* No known broken presets.

librashader's parser is fuzzed with slang-shaders and will accept invalid keys like `mipmap1` or `filter_texture = linear` 
to account for shader presets that use these invalid constructs. No known shader presets fail to parse due to syntax errors 
that haven't already been accounted for.

## Broken due to preprocessing errors

The preprocessor resolves `#include` pragmas in each `.slang` shader and recursively flattens files into a single compute unit.

* `bezel/Mega_Bezel/shaders/hyllian/crt-super-xbr/crt-super-xbr.slangp`: GLSL error: `ERROR: common-functions-bezel.inc:63: 'HSM_GetInverseScaledCoord' : no matching overloaded function found `
