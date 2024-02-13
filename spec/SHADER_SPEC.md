# Vulkan GLSL RetroArch shader system

This document is a draft of RetroArch's new GPU shader system.
It will outline the features in the new shader subsystem and describe details for how it will work in practice.

In addition this document will contain various musings on why certain design choices are made and which compromised have been made to arrive at the conclusion. This is mostly for discussing and deliberation while the new system is under development.

## Introduction

### Target shader languages
 - Vulkan
 - GL 2.x (legacy desktop)
 - GL 3.x+ (modern desktop)
 - GLES2 (legacy mobile)
 - GLES3 (modern mobile)
 - (HLSL, potentially)
 - (Metal, potentially)

RetroArch is still expected to run on GLES2 and GL2 systems.
GL2 is mostly not relevant any longer, but GLES2 is certainly a very relevant platform still and having GLES2 compatibility makes GL2 very easy.
We therefore want to avoid speccing out a design which deliberately ruins GLES2 compatibility.

However, we also do not want to artificially limit ourselves to shader features which are only available in GLES2.
There are many shader builtins for example which only work in GLES3/GL3 and we should not hold back support in these cases.
When we want to consider GLES2 compat we should not spec out high level features which do not make much sense in the context of GLES2.

### Why a new spec?

The current shader subsystem in RetroArch is quite mature with a large body of shaders written for it.
While it has served us well, it is not forward-compatible.

The current state of writing high-level shading languages that work "everywhere" is very challenging.
There was no good ready-made solution for this.
Up until now, we have relied on nVidia Cg to serve as a basic foundation for shaders, but Cg has been discontinued for years and is closed source.
This is very problematic since Cg is not a forward compatible platform.
It has many warts which are heavily tied in to legacy APIs and systems.
For this reason, we cannot use Cg for newer APIs such as Vulkan and potentially D3D12 and Metal.

Cg cross compilation to GLSL is barely working and it is horribly unmaintainable with several unfixable issues.
The output is so horribly mangled and unoptimized that it is clearly not the approach we should be taking.
We also cannot do the Cg transform in runtime on mobile due to lack of open source Cg runtime, so there's that as well.

Another alternative is to write straight-up GLSL, but this too has some severe problems.
All the different GL versions and GLSL variants are different enough that it becomes painful to write portable GLSL code that works without modification.
Examples include:

 - varying/attribute vs in/out (legacy vs modern)
 - precision qualifiers (GLSL vs ESSL)
 - texture2D vs texture (legacy vs modern)
 - Lack of standard support for #include to reduce copy-pasta

The problem really is that GLSL shaders are dependent on the runtime GL version, which makes it very annoying and hard to test all shader variants.

We do not want to litter every shader with heaps of #ifdefs everywhere to combat this problem.
We also want to avoid having to write pseudo-GLSL with some text based replacement behind the scenes.

#### Vulkan GLSL as the portable solution

Fortunately, there is now a forward looking and promising solution to our problems.
Vulkan GLSL is a GLSL dialect designed for Vulkan and SPIR-V intermediate representation.
The good part is that we can use whatever GLSL version we want when writing shaders, as it is decoupled from the GL runtime.

In runtime, we can have a vendor-neutral mature compiler,
[https://github.com/KhronosGroup/glslang](glslang) which compiles our Vulkan GLSL to SPIR-V.
Using [https://github.com/KhronosGroup/SPIRV-Cross](SPIRV-Cross), we can then do reflection on the SPIR-V binary to deduce our filter chain layout.
We can also disassemble back to our desired GLSL dialect in the GL backend based on which GL version we're running,
which effectively means we can completely sidestep all our current problems with a pure GLSL based shading system.

Another upside of this is that we no longer have to deal with vendor-specific quirks in the GLSL frontend.
A common problem when people write for nVidia is that people mistakingly use float2/float3/float4 types from Cg/HLSL, which is supported
as an extension in their GLSL frontend.

##### Why not SPIR-V directly?

This was considered, but there are several convenience problems with having a shading spec around pure SPIR-V.
The first problem is metadata. In GLSL, we can quite easily extend with custom #pragmas or similar, but there is no trivial way to do this in SPIR-V
outside writing custom tools to emit special metadata as debug information or similar with OpSource.

We could also have this metadata outside in a separate file, but juggling more files means more churn, which we should try to avoid.
The other problem is convenience. If RetroArch only accepts SPIR-V, we would need an explicit build step outside RetroArch first before we could
test a shader. This gets very annoying during shader development,
so it is clear that we need to support GLSL anyways, making SPIR-V support kinda redundant.

The main argument for supporting SPIR-V would be to allow new shading languages to be used. This is a reasonable thing to consider, which is why
the goal is to not design ourselves into a corner where it's only Vulkan GLSL that can possibly work down the line. We are open to the idea that
new shading languages that target SPIR-V will emerge.

### Warts in old shader system

While the old shader system is functional it has some severe warts which have accumulated over time.
In hindsight, some of the early design decisions were misguided and need to be properly fixed.

#### Forced POT with padding

This is arguably the largest wart of them all. The original reason behind this design decision was caused by a misguided effort to combat FP precision issues with texture sampling. The idea at the time was to avoid cases where nearest neighbor sampling at texel edges would cause artifacts. This is a typical case when textures are scaled with non-integer factors. However, the problem to begin with is naive nearest neighbor and non-integer scaling factors, and not FP precision. It was pure luck that POT tended to give better results with broken shaders, but we should not make this mistake again. POT padding has some severe issues which are not just cleanliness related either.

Technically, GLES2 doesn't require non-POT support, but in practice, all GPUs support this.

##### No proper UV wrapping
Since the texture "ends" at UV coords < 1.0, we cannot properly
use sampler wrapping modes. We can only fake `CLAMP_TO_BORDER` by padding with black color, but this filtering mode is not available by default in GLES2 and even GLES3!
`CLAMP_TO_BORDER` isn't necessarily what we want either. `CLAMP_TO_EDGE` is usually a far more sane default.

##### Extra arguments for actual width vs. texture width

With normalized coordinates we need to think in both real resolution (e.g. 320x240) vs. POT padded resolutions (512x512) to deal with normalized UV coords. This complicates things massively and
we were passing an insane amount of attributes and varyings to deal with this because the ratios between the two needn't be the same for two different textures.

#### Arbitrary limits
The way the old shader system deals with limits is quite naive.
There is a hard limit of 8 when referencing other passes and older frames.
There is no reason why we should have arbitrary limits like these.
Part of the reason is C where dealing with dynamic memory is more painful than is should be so it was easier to take the lazy way out.

#### Tacked on format handling

In more complex shaders we need to consider more than just the plain `RGBA8_UNORM` format.
The old shader system tacked on these things after the fact by adding booleans for SRGB and FP support, but this obviously doesn't scale.
This point does get problematic since GLES2 has terrible support for render target formats, but we should allow complex shaders to use complex RT formats
and rather just allow some shader presets to drop GLES2 compat.

#### PASS vs PASSPREV

Ugly. We do not need two ways to access previous passes, the actual solution is to have aliases for passes instead and access by name.

#### Inconsistencies in parameter passing

MVP matrices are passed in with weird conventions in the Cg spec, and its casing is weird.
The source texture is passed with magic TEXUNIT0 semantic while other textures are passed via uniform struct members, etc.
This is the result of tacking on feature support slowly over time without proper forethought.

## High level Overview

The RetroArch shader format outlines a filter chain/graph, a series of shader passes which operate on previously generated data to produce a final result.
The goal is for every individual pass to access information from *all* previous shader passes, even across frames, easily.

 - The filter chain specifies a number of shader passes to be executed one after the other.
 - Each pass renders a full-screen quad to a texture of a certain resolution and format.
 - The resolution can be dependent on external information.
 - All filter chains begin at an input texture, which is created by a libretro core or similar.
 - All filter chains terminate by rendering to the "backbuffer".

The backbuffer is somewhat special since the resolution of it cannot be controlled by the shader.
It can also not be fed back into the filter chain later
because the frontend (here RetroArch) will render UI elements and such on top of the final pass output.

Let's first look at what we mean by filter chains and how far we can expand this idea.

### Simplest filter chain

The simplest filter chain we can specify is a single pass.

```
(Input) -> [ Shader Pass #0 ] -> (Backbuffer)
```

In this case there are no offscreen render targets necessary since our input is rendered directly to screen.

### Multiple passes

A trivial extension is to keep our straight line view of the world where each pass looks at the previous output.

```
(Input) -> [ Shader Pass #0 ] -> (Framebuffer) -> [ Shader Pass #1 ] -> (Backbuffer)
```

Framebuffer here might have a different resolution than both Input and Backbuffer.
A very common scenario for this is separable filters where we first scale horizontally, then vertically.

### Multiple passes and multiple inputs

There is no reason why we should restrict ourselves to a straight-line view.

```
     /------------------------------------------------\
    /                                                  v
(Input) -> [ Shader Pass #0 ] -> (Framebuffer #0) -> [ Shader Pass #1 ] -> (Backbuffer)
```

In this scenario, we have two inputs to shader pass #1, both the original, untouched input as well as the result of a pass in-between.
All the inputs to a pass can have different resolutions.
We have a way to query the resolution of individual textures to allow highly controlled sampling.

We are now at a point where we can express an arbitrarily complex filter graph, but we can do better.
For certain effects, time (or rather, results from earlier frames) can be an important factor.

### Multiple passes, multiple inputs, with history

We now extend our filter graph, where we also have access to information from earlier frames. Note that this is still a causal filter system.

```
Frame N:        (Input     N, Input N - 1, Input N - 2) -> [ Shader Pass #0 ] -> (Framebuffer     N, Framebuffer N - 1, Input N - 3) -> [ Shader Pass #1 ] -> (Backbuffer)
Frame N - 1:    (Input N - 1, Input N - 2, Input N - 3) -> [ Shader Pass #0 ] -> (Framebuffer N - 1, Framebuffer N - 2, Input N - 4) -> [ Shader Pass #1 ] -> (Backbuffer)
Frame N - 2:    (Input N - 2, Input N - 3, Input N - 4) -> [ Shader Pass #0 ] -> (Framebuffer N - 2, Framebuffer N - 3, Input N - 5) -> [ Shader Pass #1 ] -> (Backbuffer)
```

For framebuffers we can read the previous frame's framebuffer. We don't really need more than one frame of history since we have a feedback effect in place.
Just like IIR filters, the "response" of such a feedback in the filter graph gives us essentially "infinite" history back in time,
although it is mostly useful for long-lasting blurs and ghosting effects. Supporting more than one frame of feedback would also be extremely memory intensive since framebuffers tend to be
much higher resolution than their input counterparts. One frame is also a nice "clean" limit. Once we go beyond just 1, the floodgate opens to arbitrary numbers, which we would want to avoid.
It is also possible to fake as many feedback frames of history we want anyways,
since we can copy a feedback frame to a separate pass anyways which effectively creates a "shift register" of feedback framebuffers in memory.

Input textures can have arbitrary number of textures as history (just limited by memory).
They cannot feedback since the filter chain cannot render into it, so it effectively is finite response (FIR).

For the very first frames, frames with frame N < 0 are transparent black (all values 0).

### No POT padding

No texture in the filter chain is padded at any time. It is possible for resolutions in the filter chain to vary over time which is common with certain emulated systems.
In this scenarios, the textures and framebuffers are simply resized appropriately.
Older frames still keep their old resolution in the brief moment that the resolution is changing.

It is very important that shaders do not blindly sample with nearest filter with any scale factor. If naive nearest neighbor sampling is to be used, shaders must make sure that
the filter chain is configured with integer scaling factors so that ambiguous texel-edge sampling is avoided.

### Deduce shader inputs by reflection

We want to have as much useful information in the shader source as possible. We want to avoid having to explicitly write out metadata in shaders whereever we can.
The biggest hurdle to overcome is how we describe our pipeline layout. The pipeline layout contains information about how we access resources such as uniforms and textures.
There are three main types of inputs in this shader system.

 - Texture samplers (sampler2D)
 - Look-up textures for static input data
 - Uniform data describing dimensions of textures
 - Uniform ancillary data for render target dimensions, backbuffer target dimensions, frame count, etc
 - Uniform user-defined parameters
 - Uniform MVP for vertex shader

#### Deduction by name

There are two main approaches to deduce what a sampler2D uniform wants to sample from.
The first way is to explicitly state somewhere else what that particular sampler needs, e.g.

```
uniform sampler2D geeWhatAmI;

// Metadata somewhere else
SAMPLER geeWhatAmI = Input[-2]; // Input frame from 2 frames ago
```

The other approach is to have built-in identifiers which correspond to certain textures.

```
// Source here being defined as the texture from previous framebuffer pass or the input texture if this is the first pass in the chain.
uniform sampler2D Source;
```

In SPIR-V, we can use `OpName` to describe these names, so we do not require the original Vulkan GLSL source to perform this reflection.
We use this approach throughout the specification. An identifier is mapped to an internal meaning (semantic). The shader backend looks at these semantics and constructs
a filter chain based on all shaders in the chain.

Identifiers can also have user defined meaning, either as an alias to existing identifiers or mapping to user defined parameters.

### Combining vertex and fragment into a single shader file

One strength of Cg is its ability to contain multiple shader stages in the same .cg file.
This is very convenient since we always want to consider vertex and fragment together.
This is especially needed when trying to mix and match shaders in a GUI window for example.
We don't want to require users to load first a vertex shader, then fragment manually.

GLSL however does not support this out of the box. This means we need to define a light-weight system for preprocessing
one GLSL source file into multiple stages.

#### Should we make vertex optional?

In most cases, the vertex shader will remain the same.
This leaves us with the option to provide a "default" vertex stage if the shader stage is not defined.

### #include support

With complex filter chains there is a lot of oppurtunity to reuse code.
We therefore want light support for the #include directive.

### User parameter support

Since we already have a "preprocessor" of sorts, we can also trivially extend this idea with user parameters.
In the shader source we can specify which uniform inputs are user controlled, GUI visible name, their effective range, etc.

### Lookup textures

A handy feature to have is reading from lookup textures.
We can specify that some sampler inputs are loaded from a PNG file on disk as a plain RGBA8 texture.

#### Do we want to support complex reinterpretation?

There could be valid use cases for supporting other formats than plain `RGBA8_UNORM`.
`SRGB` and `UINT` might be valid cases as well and maybe even 2x16-bit, 1x32-bit integer formats.

#### Lookup buffers

Do we want to support lookup buffers as UBOs as well?
This wouldn't be doable in GLES2, but it could be useful as a more modern feature.
If the LUT is small enough, we could realize it via plain old uniforms as well perhaps.

This particular feature could be very interesting for generic polyphase lookup banks with different LUT files for different filters.

## Vulkan GLSL specification

This part of the spec considers how Vulkan GLSL shaders are written. The frontend uses the glslang frontend to compile GLSL sources.
This ensures that we do not end up with vendor-specific extensions.
The #version string should be as recent as possible, e.g. `#version 450` or `#version 310 es`.
It is recommended to use 310 es since it allows mediump which can help on mobile.
Note that after the Vulkan GLSL is turned into SPIR-V, the original #version string does not matter anymore.
Also note that SPIR-V cannot be generated from legacy shader versions such as #version 100 (ES 2.0) or #version 120 (GL 2.1).

The frontend will use reflection on the resulting SPIR-V file in order to deduce what each element in the UBO or what each texture means.
The main types of data passed to shaders are read-only and can be classified as:

 - `uniform sampler2D`: This is used for input textures, framebuffer results and lookup-textures.
 - `uniform Block { };`: This is used for any constant data which is passed to the shader.
 - `layout(push_constant) uniform Push {} name;`: This is used for any push constant data which is passed to the shader.

### Resource usage rules

Certain rules must be adhered to in order to make it easier for the frontend to dynamically set up bindings to resources.

 - All resources must be using descriptor set #0, or don't use layout(set = #N) at all.
 - layout(binding = #N) must be declared for all UBOs and sampler2Ds.
 - All resources must use different bindings.
 - There can be only one UBO.
 - There can be only use push constant block.
 - It is possible to have one regular UBO and one push constant UBO.
 - If a UBO is used in both vertex and fragment, their binding number must match.
 - If a UBO is used in both vertex and fragment, members with the same name must have the same offset/binary interface.
   This problem is easily avoided by having the same UBO visible to both vertex and fragment as "common" code.
 - If a push constant block is used in both vertex and fragment, members with the same name must have the same offset/binary interface.
 - sampler2D cannot be used in vertex, although the size parameters of samplers can be used in vertex.
 - Other resource types such as SSBOs, images, atomic counters, etc, etc, are not allowed.
 - Every member of the UBOs and push constant blocks as well as every texture must be meaningful
   to the frontend in some way, or an error is generated.

### Initial preprocess of slang files

The very first line of a `.slang` file must contain a `#version` statement.

The first process which takes place is dealing with `#include` statements.
A slang file is preprocessed by scanning through the slang and resolving all `#include` statements.
The include process does not consider any preprocessor defines or conditional expressions.
The include path must always be relative, and it will be relative to the file path of the current file.
Nested includes are allowed, but includes in a cycle are undefined as preprocessor guards are not considered.

E.g.:
```
#include "common.inc"
```

After includes have been resolved, the frontend scans through all lines of the shader and considers `#pragma` statements.
These pragmas build up ancillary reflection information and otherwise meaningful metadata.

#### `#pragma stage`
This pragma controls which part of a `.slang` file are visible to certain shader stages.
Currently, two variants of this pragma are supported:

 - `#pragma stage vertex`
 - `#pragma stage fragment`

If no `#pragma stage` has been encountered yet, lines of code in a shader belong to all shader stages.
If a `#pragma stage` statement has been encountered, that stage is considered active, and the following lines of shader code will only be used when building source for that particular shader stage. A new `#pragma stage` can override which stage is active.

#### `#pragma name`
This pragma lets a shader set its identifier. This identifier can be used to create simple aliases for other passes.

E.g.:
```
#pragma name HorizontalPass
```

#### `#pragma format`
This pragma controls the format of the framebuffer which this shader will render to.
The default render target format is `R8G8B8A8_UNORM`.

Supported render target formats are listed below. From a portability perspective,
please be aware that GLES2 has abysmal render target format support,
and GLES3/GL3 may have restricted floating point render target support.

If rendering to uint/int formats, make sure your fragment shader output target is uint/int.

#### 8-bit
 - `R8_UNORM`
 - `R8_UINT`
 - `R8_SINT`
 - `R8G8_UNORM`
 - `R8G8_UINT`
 - `R8G8_SINT`
 - `R8G8B8A8_UNORM`
 - `R8G8B8A8_UINT`
 - `R8G8B8A8_SINT`
 - `R8G8B8A8_SRGB`

#### 10-bit
 - `A2B10G10R10_UNORM_PACK32`
 - `A2B10G10R10_UINT_PACK32`

#### 16-bit
 - `R16_UINT`
 - `R16_SINT`
 - `R16_SFLOAT`
 - `R16G16_UINT`
 - `R16G16_SINT`
 - `R16G16_SFLOAT`
 - `R16G16B16A16_UINT`
 - `R16G16B16A16_SINT`
 - `R16G16B16A16_SFLOAT`

#### 32-bit
 - `R32_UINT`
 - `R32_SINT`
 - `R32_SFLOAT`
 - `R32G32_UINT`
 - `R32G32_SINT`
 - `R32G32_SFLOAT`
 - `R32G32B32A32_UINT`
 - `R32G32B32A32_SINT`
 - `R32G32B32A32_SFLOAT`

E.g.:
```
#pragma format R16_SFLOAT
```
#### `#pragma parameter`

Shader parameters allow shaders to take user-defined inputs as uniform values.
This makes shaders more configurable.

The format is:
```
#pragma parameter IDENTIFIER "DESCRIPTION" INITIAL MINIMUM MAXIMUM [STEP]
```
The step parameter is optional.
INITIAL, MINIMUM and MAXIMUM are floating point values.
IDENTIFIER is the meaningful string which is the name of the uniform which will be used in a UBO or push constant block.
DESCRIPTION is a string which is human readable representation of IDENTIFIER.

E.g:
```
layout(push_constant) uniform Push {
   float DummyVariable;
} registers;
#pragma parameter DummyVariable "This is a dummy variable" 1.0 0.2 2.0 0.1
```

### I/O interface variables

The slang shader spec specifies two vertex inputs and one fragment output.
Varyings between vertex and fragment shaders are user-defined.

#### Vertex inputs
Two attributes are provided and must be present in a shader.
It is only the layout(location = #N) which is actually significant.
The particular names of input and output variables are ignored, but should be consistent for readability.

##### `layout(location = 0) in vec4 Position;`
This attribute is a 2D position in the form `vec4(x, y, 0.0, 1.0);`.
Shaders should not try to extract meaning from the x, y.
`gl_Position` must be assigned as:

```
gl_Position = MVP * Position;
```
##### `layout(location = 1) in vec2 TexCoord;`
The texture coordinate is semantically such that (0.0, 0.0) is top-left and (1.0, 1.0) is bottom right.
If TexCoord is passed to a varying unmodified, the interpolated varying will be `uv = 0.5 / OutputSize` when rendering the upper left pixel as expected and `uv = 1.0 - 0.5 / OutputSize` when rendering the bottom-right pixel.

#### Vertex/Fragment interface
Vertex outputs and fragment inputs link by location, and not name.

E.g.:
```
// Vertex
layout(location = 0) out vec4 varying;
// Fragment
layout(location = 0) in vec4 some_other_name;
```
will still link fine, although using same names are encouraged for readability.

#### Fragment outputs

##### `layout(location = 0) out vec4 FragColor;`
Fragment shaders must have a single output to location = 0.
Multiple render targets are not allowed. The type of the output depends on the render target format.
int/uint type must be used if UINT/INT render target formats are used, otherwise float type.

### Builtin variables

#### Builtin texture variables
The input of textures get their meaning from their name.

 - Original: This accesses the input of the filter chain, accessible from any pass.
 - Source: This accesses the input from previous shader pass, or Original if accessed in the first pass of the filter chain.
 - OriginalHistory#: This accesses the input # frames back in time.
   There is no limit on #, except larger numbers will consume more VRAM.
   OriginalHistory0 is an alias for Original, OriginalHistory1 is the previous frame and so on.
 - PassOutput#: This accesses the output from pass # in this frame.
   PassOutput# must be causal, it is an error to access PassOutputN in pass M if N >= M.
   PassOutput# will typically be aliased to a more readable value.
 - PassFeedback#: This accesses PassOutput# from the previous frame.
   Any pass can read the feedback of any feedback, since it is causal.
   PassFeedback# will typically be aliased to a more readable value.
 - User#: This accesses look-up textures.
   However, the direct use of User# is discouraged and should always be accessed via aliases.

#### Builtin texture size uniform variables

If a member of a UBO or a push constant block is called ???Size# where ???# is the name of a texture variable,
that member must be a vec4, which will receive these values:
 - X: Horizontal size of texture
 - Y: Vertical size of texture
 - Z: 1.0 / (Horizontal size of texture)
 - W: 1.0 / (Vertical size of texture)

It is valid to use a size variable without declaring the texture itself. This is useful for vertex shading.
It is valid (although probably not useful) for a variable to be present in both a push constant block and a UBO block at the same time.

#### Builtin uniform variables

Other than uniforms related to textures, there are other special uniforms available.
These builtin variables may be part of a UBO block and/or a push constant block.

 - MVP: mat4 model view projection matrix.
 - OutputSize: a vec4(x, y, 1.0 / x, 1.0 / y) variable describing the render target size (x, y) for this pass.
 - FinalViewportSize: a vec4(x, y, 1.0 / x, 1.0 / y) variable describing the render target size for the final pass.
   Accessible from any pass.
 - FrameCount: a uint variable taking a value which increases by one every frame.
   This value could be pre-wrapped by modulo if specified in preset.
   This is useful for creating time-dependent effects.

#### Aliases
Aliases can give meaning to arbitrary names in a slang file.
This is mostly relevant for LUT textures, shader parameters and accessing other passes by name.

If a shader pass has a `#pragma name NAME` associated with it, meaning is given to the shader:
 - NAME, is a sampler2D.
 - NAMESize is a vec4 size uniform associated with NAME.
 - NAMEFeedback is a sampler2D for the previous frame.
 - NAMEFeedbackSize is a vec4 size uniform associated with NAMEFeedback.

#### Example slang shader

```
#version 450
// 450 or 310 es are recommended

layout(set = 0, binding = 0, std140) uniform UBO
{
   mat4 MVP;
   vec4 SourceSize; // Not used here, but doesn't hurt
   float ColorMod;
};

#pragma name StockShader
#pragma format R8G8B8A8_UNORM
#pragma parameter ColorMod "Color intensity" 1.0 0.1 2.0 0.1

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
void main()
{
   gl_Position = MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 1) uniform sampler2D Source;
void main()
{
   FragColor = texture(Source, vTexCoord) * ColorMod;
}
```

### Push constants vs uniform blocks
Push constants are fast-access uniform data which on some GPUs will improve performance over plain UBOs.
It is encouraged to use push constant data as much as possible.

```
layout(push_constant) uniform Push
{
   vec4 SourceSize;
   vec4 FinalViewportSize;
} registers;
```

However, be aware that there is a limit to how large push constant blocks can be used.
Vulkan puts a minimum required size of 128 bytes, which equals 8 vec4s.
It is an error to use more than 128 bytes.
If you're running out of space, you can move the MVP to a UBO instead, which frees up 64 bytes.
Always prioritize push constants for data used in fragment shaders as there are many more fragment threads than vertex.
Also note that like UBOs, the push constant space is shared across vertex and fragment.

If you need more than 8 vec4s, you can spill uniforms over to plain UBOs,
but more than 8 vec4s should be quite rare in practice.

E.g.:

```
layout(binding = 0, std140) uniform UBO
{
   mat4 MVP; // Only used in vertex
   vec4 SpilledUniform;
} global;

layout(push_constant) uniform Push
{
   vec4 SourceSize;
   vec4 BlurPassSize;
   // ...
} registers;
```

### Samplers
Which samplers are used for textures are specified by the preset format.
The sampler remains constant throughout the frame, there is currently no way to select samplers on a frame-by-frame basic.
This is mostly to make it possible to use the spec in GLES2 as GLES2 has no concept of separate samplers and images.

### sRGB
The input to the filter chain will not be of an sRGB format.
This is due to many reasons, the main one being that it is very difficult for the frontend to get "free" passthrough of sRGB. It is possible to have a first pass which linearizes the input to a proper sRGB render target. In this way, custom gammas can be used as well.

Similarly, the final pass will not be an sRGB backbuffer for similar reasons.

### Caveats

#### Frag Coord
TexCoord also replaces `gl_FragCoord`. Do not use `gl_FragCoord` as it doesn't consider the viewports correctly.
If you need `gl_FragCoord` use `vTexCoord * OutputSize.xy` instead.

#### Derivatives
Be careful with derivatives of vTexCoord. The screen might have been rotated by the vertex shader, which will also rotate the derivatives, especially in the final pass which hits the backbuffer.
However, derivatives are fortunately never really needed, since w = 1 (we render flat 2D quads),
which means derivatives of varyings are constant. You can do some trivial replacements which will be faster and more robust.

```
dFdx(vTexCoord) = vec2(OutputSize.z, 0.0);
dFdy(vTexCoord) = vec2(0.0, OutputSize.w);
fwidth(vTexCoord) = max(OutputSize.z, OutputSize.w);
```
To avoid issues with rotation or unexpected derivatives in case derivatives are really needed,
off-screen passes will not have rotation and
dFdx and dFdy will behave as expected.

#### Correctly sampling textures
A common mistake made by shaders is that they aren't careful enough about sampling textures correctly.
There are three major cases to consider

##### Bilinear sampling
If bilinear is used, it is always safe to sample a texture.

##### Nearest, with integer scale
If the OutputSize / InputSize is integer,
the interpolated vTexCoord will always fall inside the texel safely, so no special precautions have to be used.
For very particular shaders which rely on nearest neighbor sampling, using integer scale to a framebuffer and upscaling that
with more stable upscaling filters like bicubic for example is usually a great choice.

##### Nearest, with non-integer scale
Sometimes, it is necessary to upscale images to the backbuffer which have an arbitrary size.
Bilinear is not always good enough here, so we must deal with a complicated case.

If we interpolate vTexCoord over a frame with non-integer scale, it is possible that we end up just between two texels.
Nearest neighbor will have to find a texel which is nearest, but there is no clear "nearest" texel. In this scenario, we end up having lots of failure cases which are typically observed as weird glitches in the image which change based on the resolution.

To correctly sample nearest textures with non-integer scale, we must pre-quantize our texture coordinates.
Here's a snippet which lets us safely sample a nearest filtered texture and emulate bilinear filtering.

```
   vec2 uv = vTexCoord * global.SourceSize.xy - 0.5; // Shift by 0.5 since the texel sampling points are in the texel center.
   vec2 a = fract(uv);
   vec2 tex = (floor(uv) + 0.5) * global.SourceSize.zw; // Build a sampling point which is in the center of the texel.

   // Sample the bilinear footprint.
   vec4 t0 = textureLodOffset(Source, tex, 0.0, ivec2(0, 0));
   vec4 t1 = textureLodOffset(Source, tex, 0.0, ivec2(1, 0));
   vec4 t2 = textureLodOffset(Source, tex, 0.0, ivec2(0, 1));
   vec4 t3 = textureLodOffset(Source, tex, 0.0, ivec2(1, 1));

   // Bilinear filter.
   vec4 result = mix(mix(t0, t1, a.x), mix(t2, t3, a.x), a.y);
```

The concept of splitting up the integer texel along with the fractional texel helps us safely
do arbitrary non-integer scaling safely.
The uv variable could also be passed pre-computed from vertex to avoid the extra computation in fragment.

### Preset format (.slangp)

The present format is essentially unchanged from the old .cgp and .glslp, except the new preset format is called .slangp.

## Porting guide from legacy Cg spec

### Common functions
 - mul(mat, vec) -> mat * vec
 - lerp() -> mix()
 - ddx() -> dFdx()
 - ddy() -> dFdy()
 - tex2D() -> texture()
 - frac() -> fract()

### Types

 - floatN -> vecN
 - boolN -> bvecN
 - intN -> ivecN
 - uintN -> uvecN
 - float4x4 -> mat4

### Builtin uniforms and misc

 - modelViewProj -> MVP
 - IN.video\_size -> SourceSize.xy
 - IN.texture\_size -> SourceSize.xy (no POT shenanigans, so they are the same)
 - IN.output\_size -> OutputSize.xy
 - IN.frame\_count -> FrameCount (uint instead of float)
 - \*.tex\_coord -> TexCoord (no POT shenanigans, so they are all the same)
 - \*.lut\_tex\_coord -> TexCoord
 - ORIG -> `Original`
 - PASS# -> PassOutput#
 - PASSPREV# -> No direct analog, PassOutput(CurrentPass - #), but prefer aliases

### Cg semantics

 - POSITION -> gl\_Position
 - float2 texcoord : TEXCOORD0 -> layout(location = 1) in vec2 TexCoord;
 - float4 varying : TEXCOORD# -> layout(location = #) out vec4 varying;
 - uniform float4x4 modelViewProj -> uniform UBO { mat4 MVP; };

Output structs should be flattened into separate varyings.

E.g. instead of
```
struct VertexData
{
   float pos : POSITION;
   float4 tex0 : TEXCOORD0;
   float4 tex1 : TEXCOORD1;
};

void main_vertex(out VertexData vout)
{
   vout.pos = ...;
   vout.tex0 = ...;
   vout.tex1 = ...;
}

void main_fragment(in VertexData vout)
{
   ...
}
```

do this

```
#pragma stage vertex
layout(location = 0) out vec4 tex0;
layout(location = 1) out vec4 tex1;
void main()
{
   gl_Position = ...;
   tex0 = ...;
   tex1 = ...;
}

#pragma stage fragment
layout(location = 0) in vec4 tex0;
layout(location = 1) in vec4 tex1;
void main()
{
}
```

Instead of returning a float4 from main\_fragment, have an output in fragment:

```
layout(location = 0) out vec4 FragColor;
```
