
# Broken Shader Presets

The following shaders are known to be broken due to various issues. 

This list is updated as of [slang-shaders@`356678e`](https://github.com/libretro/slang-shaders/commit/356678ec53ca940a53fa509eff0b65bb63a403bb)

## Broken due to parsing errors
librashader's preset parser is somewhat stricter than RetroArch in what it accepts. All shaders and textures in a preset must 
resolve to a fully canonical path to properly parse. The following shaders have broken paths.

* `bezel/Mega_Bezel/shaders/hyllian/crt-super-xbr/crt-super-xbr.slangp`: Missing `bezel/Mega_Bezel/shaders/hyllian/crt-super-xbr/shaders/linearize.slang`
* `crt/crt-maximus-royale-fast-mode.slangp`: Missing `crt/shaders/crt-maximus-royale/FrameTextures/16_9/TV_decor_1.png`
* `crt/crt-maximus-royale-half-res-mode.slangp`: Missing `crt/shaders/crt-maximus-royale/FrameTextures/16_9/TV_decor_1.png`
* `crt/crt-maximus-royale.slangp`: Missing `crt/shaders/crt-maximus-royale/FrameTextures/16_9/TV_decor_1.png`
* `crt/mame_hlsl.slangp`: Missing `crt/shaders/mame_hlsl/shaders/lut.slang`
* `denoisers/fast-bilateral-super-2xbr-3d-3p.slangp`: Missing `xbr/shaders/super-xbr/super-2xbr-3d-pass0.slang`
* `presets/tvout/tvout+ntsc-256px-composite.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-3phase.slang`
* `presets/tvout/tvout+ntsc-256px-svideo.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-3phase.slang`
* `presets/tvout/tvout+ntsc-2phase-composite.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-2phase.slang`
* `presets/tvout/tvout+ntsc-2phase-svideo.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-2phase.slang`
* `presets/tvout/tvout+ntsc-320px-composite.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-2phase.slang`
* `presets/tvout/tvout+ntsc-320px-svideo.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-2phase.slang`
* `presets/tvout/tvout+ntsc-3phase-composite.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-3phase.slang`
* `presets/tvout/tvout+ntsc-3phase-svideo.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-3phase.slang`
* `presets/tvout/tvout+ntsc-nes.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-3phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-256px-composite+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-3phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-256px-svideo+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-3phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-2phase-composite+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-2phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-2phase-svideo+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-2phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-320px-composite+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-2phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-320px-svideo+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-2phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-3phase-composite+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-3phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-3phase-svideo+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-svideo-3phase.slang`
* `presets/tvout+interlacing/tvout+ntsc-nes+interlacing.slangp`: Missing `ntsc/shaders/ntsc-pass1-composite-3phase.slang`
* `scalefx/shaders/old/scalefx-9x.slangp`: Missing `../stock.slang`
* `scalefx/shaders/old/scalefx.slangp`: Missing `../stock.slang`

librashader's parser is fuzzed with slang-shaders and will accept invalid keys like `mipmap1` or `filter_texture = linear` 
to account for shader presets that use these invalid constructs. No known shader presets fail to parse due to syntax errors 
that haven't already been accounted for.

## Broken due to preprocessing errors

The preprocessor resolves `#include` pragmas in each `.slang` shader and recursively flattens files into a single compute unit.

* `misc/shaders/glass.slang`: Missing `misc/include/img/param_floats.h`.
  * Looks like this was moved too deep, it references `../include/img/param_floats.h`, but the shader lives in the `misc/shaders/` folder.