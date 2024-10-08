# librashader CLI

``` 
Helpers and utilities to reflect and debug 'slang' shaders and presets

Usage: librashader-cli <COMMAND>

Commands:
  render      Render a shader preset against an image
  compare     Compare two runtimes and get a similarity score between the two runtimes rendering the same frame
  parse       Parse a preset and get a JSON representation of the data
  pack        Create a serialized preset pack from a shader preset
  preprocess  Get the raw GLSL output of a preprocessed shader
  transpile   Transpile a shader in a given preset to the given format
  reflect     Reflect the shader relative to a preset, giving information about semantics used in a slang shader
  help        Print this message or the help of the given subcommand(s)
    
Options:
  -h, --help     Print help
  -V, --version  Print version

```

`librashader-cli` provides a CLI interface to much of librashader's functionality to reflect and debug 'slang' shaders
and presets.

## Installation
If cargo is available, `librashader-cli` can be installed from crates.io.

```
$ cargo install librashader-cli
```

## Applying a shader preset to an image

``` 
Render a shader preset against an image

Usage: librashader-cli render [OPTIONS] --preset <PRESET> --image <IMAGE> --out <OUT> --runtime <RUNTIME>

Options:
  -p, --preset <PRESET>
          The path to the shader preset to load

  -w, --wildcards <WILDCARDS>...
          Additional wildcard options, comma separated with equals signs. The PRESET and PRESET_DIR wildcards are always added to the preset parsing context.

          For example, CONTENT-DIR=MyVerticalGames,GAME=mspacman

  -f, --frame <FRAME>
          The frame to render.

          The renderer will run up to the number of frames specified here to ensure feedback and history.

          [default: 0]
          
  -d, --dimensions <DIMENSIONS>
          The dimensions of the image.

          This is given in either explicit dimensions `WIDTHxHEIGHT`, or a percentage of the input image in `SCALE%`.

      --params <PARAMS>...
          Parameters to pass to the shader preset, comma separated with equals signs.

          For example, crt_gamma=2.5,halation_weight=0.001

      --passes-enabled <PASSES_ENABLED>
          Set the number of passes enabled for the preset

  -i, --image <IMAGE>
          The path to the input image

      --frame-direction <FRAME_DIRECTION>
          The direction of rendering. -1 indicates that the frames are played in reverse order

          [default: 1]

      --rotation <ROTATION>
          The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 3 = 270deg

          [default: 0]

      --total-subframes <TOTAL_SUBFRAMES>
          The total number of subframes ran. Default is 1

          [default: 1]

      --current-subframe <CURRENT_SUBFRAME>
          The current sub frame. Default is 1

          [default: 1]

  -o, --out <OUT>
          The path to the output image

          If `-`, writes the image in PNG format to stdout.

  -r, --runtime <RUNTIME>
          The runtime to use to render the shader preset

          [possible values: opengl3, opengl4, vulkan, wgpu, d3d9, d3d11, d3d12, metal]

  -h, --help
          Print help (see a summary with '-h')


```

The `render` command can be used to apply a shader preset to an image. The available runtimes will depend on the platform
that `librashader-cli` was built for. 

For example, to apply `crt-royale.slangp` to `image.png` using the OpenGL 3.3 runtime

``` 
$  librashader-cli render -i image.png -p crt-royale.slangp -r opengl3 -o out.png
```

Some presets have animations that rely on a frame counter. The `--frame`/`-f` argument can be used to select which frame to render. 
``` 
$  librashader-cli render -i image.png -p MBZ__0__SMOOTH-ADV.slangp -f 120 -r opengl3 -o out.png
```

## Comparing the similarities of two runtimes

``` 
Compare two runtimes and get a similarity score between the two runtimes rendering the same frame

Usage: librashader-cli compare [OPTIONS] --preset <PRESET> --image <IMAGE> --left <LEFT> --right <RIGHT>

Options:
  -p, --preset <PRESET>
          The path to the shader preset to load

  -w, --wildcards <WILDCARDS>...
          Additional wildcard options, comma separated with equals signs. The PRESET and PRESET_DIR wildcards are always added to the preset parsing context.

          For example, CONTENT-DIR=MyVerticalGames,GAME=mspacman

  -f, --frame <FRAME>
          The frame to render.

          The renderer will run up to the number of frames specified here to ensure feedback and history.

          [default: 0]
          
  -d, --dimensions <DIMENSIONS>
          The dimensions of the image.

          This is given in either explicit dimensions `WIDTHxHEIGHT`, or a percentage of the input image in `SCALE%`.

      --params <PARAMS>...
          Parameters to pass to the shader preset, comma separated with equals signs.

          For example, crt_gamma=2.5,halation_weight=0.001

      --passes-enabled <PASSES_ENABLED>
          Set the number of passes enabled for the preset

  -i, --image <IMAGE>
          The path to the input image

      --frame-direction <FRAME_DIRECTION>
          The direction of rendering. -1 indicates that the frames are played in reverse order

          [default: 1]

      --rotation <ROTATION>
          The rotation of the output. 0 = 0deg, 1 = 90deg, 2 = 180deg, 3 = 270deg

          [default: 0]

      --total-subframes <TOTAL_SUBFRAMES>
          The total number of subframes ran. Default is 1

          [default: 1]

      --current-subframe <CURRENT_SUBFRAME>
          The current sub frame. Default is 1

          [default: 1]

  -l, --left <LEFT>
          The runtime to compare against

          [possible values: opengl3, opengl4, vulkan, wgpu, d3d9, d3d11, d3d12, metal]

  -r, --right <RIGHT>
          The runtime to compare to

          [possible values: opengl3, opengl4, vulkan, wgpu, d3d9, d3d11, d3d12, metal]

  -o, --out <OUT>
          The path to write the similarity image.

          If `-`, writes the image to stdout.

  -h, --help
          Print help (see a summary with '-h')

```

The `compare` command can be used to get the similarity of two different runtimes, returning a similarity score and similarity image. 
The available runtimes will depend on the platform that `librashader-cli` was built for. This is mainly used for debug and testing purposes; two runtimes should output
highly identical (> 0.99 similarity) with a near black similarity image.

## Parsing a shader preset 

``` 
Parse a preset and get a JSON representation of the data

Usage: librashader-cli parse [OPTIONS] --preset <PRESET>

Options:
  -p, --preset <PRESET>
          The path to the shader preset to load

  -w, --wildcards <WILDCARDS>...
          Additional wildcard options, comma separated with equals signs. The PRESET and PRESET_DIR wildcards are always added to the preset parsing context.

          For example, CONTENT-DIR=MyVerticalGames,GAME=mspacman

  -h, --help
          Print help (see a summary with '-h')
```


The `parse` command can be used to parse a shader preset and get a JSON represenation of its data. Wildcards can be specified
with the `--wildcards` / `-w` argument. All paths will be resolved relative to the preset.

<details>
<summary>
Getting preset information for CRT Royale
</summary>

The following command
``` 
$  librashader-cli parse -p crt-geom.slangp
```

will output the following JSON
```json 
{
  "shader_count": 12,
  "shaders": [
    {
      "id": 0,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-first-pass-linearize-crt-gamma-bob-fields.slang",
      "alias": "ORIG_LINEARIZED",
      "filter": "Nearest",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 1,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-scanlines-vertical-interlacing.slang",
      "alias": "VERTICAL_SCANLINES",
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 2,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-bloom-approx.slang",
      "alias": "BLOOM_APPROX",
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Absolute",
          "factor": {
            "Absolute": 320
          }
        },
        "y": {
          "scale_type": "Absolute",
          "factor": {
            "Absolute": 240
          }
        }
      }
    },
    {
      "id": 3,
      "path": "/tmp/shaders_slang/blurs/shaders/royale/blur9fast-vertical.slang",
      "alias": null,
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 4,
      "path": "/tmp/shaders_slang/blurs/shaders/royale/blur9fast-horizontal.slang",
      "alias": "HALATION_BLUR",
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 5,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-mask-resize-vertical.slang",
      "alias": null,
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": false,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Absolute",
          "factor": {
            "Absolute": 64
          }
        },
        "y": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 0.0625
          }
        }
      }
    },
    {
      "id": 6,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-mask-resize-horizontal.slang",
      "alias": "MASK_RESIZE",
      "filter": "Nearest",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": false,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 0.0625
          }
        },
        "y": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 7,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-scanlines-horizontal-apply-mask.slang",
      "alias": "MASKED_SCANLINES",
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 8,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-brightpass.slang",
      "alias": "BRIGHTPASS",
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 9,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-bloom-vertical.slang",
      "alias": null,
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 10,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-bloom-horizontal-reconstitute.slang",
      "alias": null,
      "filter": "Linear",
      "wrap_mode": "ClampToBorder",
      "frame_count_mod": 0,
      "srgb_framebuffer": true,
      "float_framebuffer": false,
      "mipmap_input": false,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Input",
          "factor": {
            "Float": 1.0
          }
        }
      }
    },
    {
      "id": 11,
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/src/crt-royale-geometry-aa-last-pass.slang",
      "alias": null,
      "filter": "Linear",
      "wrap_mode": "ClampToEdge",
      "frame_count_mod": 0,
      "srgb_framebuffer": false,
      "float_framebuffer": false,
      "mipmap_input": true,
      "scaling": {
        "valid": true,
        "x": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        },
        "y": {
          "scale_type": "Viewport",
          "factor": {
            "Float": 1.0
          }
        }
      }
    }
  ],
  "textures": [
    {
      "name": "mask_grille_texture_small",
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/TileableLinearApertureGrille15Wide8And5d5SpacingResizeTo64.png",
      "wrap_mode": "Repeat",
      "filter_mode": "Linear",
      "mipmap": false
    },
    {
      "name": "mask_grille_texture_large",
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/TileableLinearApertureGrille15Wide8And5d5Spacing.png",
      "wrap_mode": "Repeat",
      "filter_mode": "Linear",
      "mipmap": true
    },
    {
      "name": "mask_slot_texture_small",
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/TileableLinearSlotMaskTall15Wide9And4d5Horizontal9d14VerticalSpacingResizeTo64.png",
      "wrap_mode": "Repeat",
      "filter_mode": "Linear",
      "mipmap": false
    },
    {
      "name": "mask_slot_texture_large",
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/TileableLinearSlotMaskTall15Wide9And4d5Horizontal9d14VerticalSpacing.png",
      "wrap_mode": "Repeat",
      "filter_mode": "Linear",
      "mipmap": true
    },
    {
      "name": "mask_shadow_texture_small",
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/TileableLinearShadowMaskEDPResizeTo64.png",
      "wrap_mode": "Repeat",
      "filter_mode": "Linear",
      "mipmap": false
    },
    {
      "name": "mask_shadow_texture_large",
      "path": "/tmp/shaders_slang/crt/shaders/crt-royale/TileableLinearShadowMaskEDP.png",
      "wrap_mode": "Repeat",
      "filter_mode": "Linear",
      "mipmap": true
    }
  ],
  "parameters": []
}

```
</details>

## Getting the preprocessed GLSL source of a `.slang` shader

``` 
Get the raw GLSL output of a preprocessed shader

Usage: librashader-cli preprocess --shader <SHADER> --output <OUTPUT>

Options:
  -s, --shader <SHADER>
          The path to the slang shader

  -o, --output <OUTPUT>
          The item to output.

          `json` will print a JSON representation of the preprocessed shader.

          [possible values: fragment, vertex, params, passformat, json]

  -h, --help
          Print help (see a summary with '-h')
```

The `preprocess` command can be used to invoke the preprocessor on `.slang` 
shader source files and output the raw GLSL source that gets passed to SPIRV-Cross
during shader compilation. 

For example, to get the preprocessed fragment shader source code
```
$ librashader-cli preprocess -s crt-geom.slang -o fragment 
```

`preprocess` can also be used to get information about the pass format, 
parameters, or meta information inferred from `#pragma` declarations in the
source code.

For example (using `jq` to truncate the output)
```
$ librashader-cli preprocess -s crt-geom.slang -o params | jq 'first(.[])'
```
will return the following JSON
```json
{
  "CRTgamma": {
    "id": "CRTgamma",
    "description": "CRTGeom Target Gamma",
    "initial": 2.4,
    "minimum": 0.1,
    "maximum": 5.0,
    "step": 0.1
  }
}
```

## Convert a `.slang` to a target shader format
``` 
Transpile a shader in a given preset to the given format

Usage: librashader-cli transpile --shader <SHADER> --stage <STAGE> --format <FORMAT>

Options:
  -s, --shader <SHADER>
          The path to the slang shader

  -o, --stage <STAGE>
          The shader stage to output

          [possible values: fragment, vertex]

  -f, --format <FORMAT>
          The output format

          [possible values: glsl, hlsl, wgsl, msl, spirv]

  -v, --version <VERSION>
          The version of the output format to parse as, if applicable

          For GLSL, this should be an string corresponding to a GLSL version (e.g. '330', or '300es', or '300 es').

          For HLSL, this is a shader model version as an integer (50), or a version in the format MAJ_MIN (5_0), or MAJ.MIN (5.0).

          For MSL, this is the shader language version as an integer in format <MMmmpp>(30100), or a version in the format MAJ_MIN (3_1), or MAJ.MIN (3.1).

  -h, --help
          Print help (see a summary with '-h')
```

The `transpile` command can be used to convert a `.slang` shader source file 
to a target format used by a runtime. GLSL (under OpenGL semantics), HLSL, 
WGSL, MSL, and SPIR-V disassembly is supported as output formats.

For example, to get the pixel shader in Shader Model 6.0 HLSL for `crt-geom.slang`

```
$ librashader-cli transpile -s crt-geom.slang -o fragment -f hlsl -v 60
```

## Getting detailed reflection information for a shader

```
Reflect the shader relative to a preset, giving information about semantics used in a slang shader.

Usage: librashader-cli reflect [OPTIONS] --preset <PRESET> --index <INDEX>

Options:
  -p, --preset <PRESET>
          The path to the shader preset to load

  -w, --wildcards <WILDCARDS>...
          Additional wildcard options, comma separated with equals signs. The PRESET and PRESET_DIR wildcards are always added to the preset parsing context.

          For example, CONTENT-DIR=MyVerticalGames,GAME=mspacman

  -i, --index <INDEX>
          The pass index to use

  -b, --backend <BACKEND>
          [default: cross]
          [possible values: cross, naga]

  -h, --help
          Print help (see a summary with '-h') 
```

The `reflect` command can be used to get reflection detailed information regarding
uniform and texture bindings for a shader pass, relative to a preset. As semantics for LUT images are defined by the preset definition, reflection information is only valid for a shader source file relative to its shader preset.

The default backend to do reflection is with SPIRV-Cross. Reflections via Naga (used in the wgpu runtime) are also available if desired, and may have different results than SPIRV-Cross.

<details>
<summary>
Getting reflection information for CRT Geom
</summary>

`crt-geom.slangp` only has a single pass, but we still need to specify the pass in relation to its preset.

```
$ librashader-cli reflect -p crt-geom.slangp -i 0 
```

The above command will output the following JSON

```json
{
  "ubo": {
    "binding": 0,
    "size": 96,
    "stage_mask": "VERTEX | FRAGMENT"
  },
  "push_constant": {
    "binding": null,
    "size": 96,
    "stage_mask": "VERTEX | FRAGMENT"
  },
  "meta": {
    "param": {
      "ysize": {
        "offset": {
          "ubo": null,
          "push": 80
        },
        "size": 1,
        "id": "ysize"
      },
      "xsize": {
        "offset": {
          "ubo": null,
          "push": 76
        },
        "size": 1,
        "id": "xsize"
      },
      "invert_aspect": {
        "offset": {
          "ubo": null,
          "push": 68
        },
        "size": 1,
        "id": "invert_aspect"
      },
      "x_tilt": {
        "offset": {
          "ubo": null,
          "push": 28
        },
        "size": 1,
        "id": "x_tilt"
      },
      "y_tilt": {
        "offset": {
          "ubo": null,
          "push": 32
        },
        "size": 1,
        "id": "y_tilt"
      },
      "R": {
        "offset": {
          "ubo": null,
          "push": 16
        },
        "size": 1,
        "id": "R"
      },
      "d": {
        "offset": {
          "ubo": null,
          "push": 12
        },
        "size": 1,
        "id": "d"
      },
      "vertical_scanlines": {
        "offset": {
          "ubo": null,
          "push": 72
        },
        "size": 1,
        "id": "vertical_scanlines"
      },
      "SHARPER": {
        "offset": {
          "ubo": null,
          "push": 48
        },
        "size": 1,
        "id": "SHARPER"
      },
      "interlace_detect": {
        "offset": {
          "ubo": null,
          "push": 60
        },
        "size": 1,
        "id": "interlace_detect"
      },
      "CURVATURE": {
        "offset": {
          "ubo": null,
          "push": 56
        },
        "size": 1,
        "id": "CURVATURE"
      },
      "overscan_x": {
        "offset": {
          "ubo": null,
          "push": 36
        },
        "size": 1,
        "id": "overscan_x"
      },
      "overscan_y": {
        "offset": {
          "ubo": null,
          "push": 40
        },
        "size": 1,
        "id": "overscan_y"
      },
      "cornersize": {
        "offset": {
          "ubo": null,
          "push": 20
        },
        "size": 1,
        "id": "cornersize"
      },
      "cornersmooth": {
        "offset": {
          "ubo": null,
          "push": 24
        },
        "size": 1,
        "id": "cornersmooth"
      },
      "CRTgamma": {
        "offset": {
          "ubo": null,
          "push": 4
        },
        "size": 1,
        "id": "CRTgamma"
      },
      "scanline_weight": {
        "offset": {
          "ubo": null,
          "push": 52
        },
        "size": 1,
        "id": "scanline_weight"
      },
      "lum": {
        "offset": {
          "ubo": null,
          "push": 64
        },
        "size": 1,
        "id": "lum"
      },
      "DOTMASK": {
        "offset": {
          "ubo": null,
          "push": 44
        },
        "size": 1,
        "id": "DOTMASK"
      },
      "monitorgamma": {
        "offset": {
          "ubo": null,
          "push": 8
        },
        "size": 1,
        "id": "monitorgamma"
      }
    },
    "unique": {
      "MVP": {
        "offset": {
          "ubo": 0,
          "push": null
        },
        "size": 16,
        "id": "MVP"
      },
      "Output": {
        "offset": {
          "ubo": 64,
          "push": null
        },
        "size": 4,
        "id": "OutputSize"
      },
      "FrameCount": {
        "offset": {
          "ubo": null,
          "push": 0
        },
        "size": 1,
        "id": "FrameCount"
      }
    },
    "texture": {
      "Source": {
        "binding": 2
      }
    },
    "texture_size": {
      "Source": {
        "offset": {
          "ubo": 80,
          "push": null
        },
        "stage_mask": "VERTEX | FRAGMENT",
        "id": "SourceSize"
      }
    }
  }
}
```
</details>

## Serializing a preset pack to a single file 

```
Create a serialized preset pack from a shader preset

Usage: librashader-cli pack [OPTIONS] --preset <PRESET> --out <OUT> --format <FORMAT>

Options:
  -p, --preset <PRESET>
          The path to the shader preset to load

  -w, --wildcards <WILDCARDS>...
          Additional wildcard options, comma separated with equals signs. The PRESET and PRESET_DIR wildcards are always added to the preset parsing context.

          For example, CONTENT-DIR=MyVerticalGames,GAME=mspacman

  -o, --out <OUT>
          The path to write the output

          If `-`, writes the output to stdout

  -f, --format <FORMAT>
          The file format to output

          [possible values: json, msgpack]

  -h, --help
          Print help (see a summary with '-h')

```

The `pack` command can be used to create a "preset pack", a format to store an entire shader preset, including files and preprocessed source code, 
into a JSON or MessagePack representation where it can be loaded into memory without filesystem access. 

This file format is experimental, and may be used in the future as a way to cache shader presets, or for usages in environments without a filesystem, 
such as on the web. Note that packs are only supported by the librashader Rust API, and are not portable across other implementations of "slang" shaders.

It is unlikely that the librashader C API will ever support loading shader packs.