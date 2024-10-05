use crate::buffer::{D3D12Buffer, RawD3D12Buffer};
use crate::descriptor_heap::{CpuStagingHeap, RenderTargetHeap, ResourceWorkHeap};
use crate::draw_quad::DrawQuad;
use crate::error::FilterChainError;
use crate::filter_pass::FilterPass;
use crate::framebuffer::OwnedImage;
use crate::graphics_pipeline::{D3D12GraphicsPipeline, D3D12RootSignature};
use crate::luts::LutTexture;
use crate::mipmap::D3D12MipmapGen;
use crate::options::{FilterChainOptionsD3D12, FrameOptionsD3D12};
use crate::samplers::SamplerSet;
use crate::texture::{D3D12InputImage, D3D12OutputView, InputTexture, OutputDescriptor};
use crate::{error, util};
use d3d12_descriptor_heap::{
    D3D12DescriptorHeap, D3D12DescriptorHeapSlot, D3D12PartitionableHeap, D3D12PartitionedHeap,
};
use gpu_allocator::d3d12::{Allocator, AllocatorCreateDesc, ID3D12DeviceVersion};
use librashader_common::map::FastHashMap;
use librashader_common::{ImageFormat, Size, Viewport};
use librashader_presets::ShaderPreset;
use librashader_reflect::back::targets::{DXIL, HLSL};
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::semantics::{ShaderSemantics, MAX_BINDINGS_COUNT};
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::{BindingUtil, TextureInput};
use librashader_runtime::image::{ImageError, LoadedTexture, UVDirection};
use librashader_runtime::quad::QuadType;
use librashader_runtime::uniforms::UniformStorage;
use parking_lot::Mutex;
use std::collections::VecDeque;
use std::mem::ManuallyDrop;
use std::path::Path;
use std::sync::Arc;
use windows::core::Interface;
use windows::Win32::Foundation::CloseHandle;
use windows::Win32::Graphics::Direct3D::Dxc::{
    CLSID_DxcCompiler, CLSID_DxcLibrary, CLSID_DxcValidator, DxcCreateInstance, IDxcCompiler,
    IDxcUtils, IDxcValidator,
};
use windows::Win32::Graphics::Direct3D12::{
    ID3D12CommandAllocator, ID3D12CommandQueue, ID3D12DescriptorHeap, ID3D12Device, ID3D12Fence,
    ID3D12GraphicsCommandList, ID3D12Resource, D3D12_COMMAND_LIST_TYPE_DIRECT,
    D3D12_COMMAND_QUEUE_DESC, D3D12_COMMAND_QUEUE_FLAG_NONE, D3D12_FENCE_FLAG_NONE,
    D3D12_RESOURCE_BARRIER, D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
    D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_RENDER_TARGET,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT_UNKNOWN;
use windows::Win32::System::Threading::{CreateEventA, WaitForSingleObject, INFINITE};

use librashader_cache::CachedCompilation;
use librashader_presets::context::VideoDriver;
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use rayon::prelude::*;

const MIPMAP_RESERVED_WORKHEAP_DESCRIPTORS: usize = 4096;

/// A Direct3D 12 filter chain.
pub struct FilterChainD3D12 {
    pub(crate) common: FilterCommon,
    pub(crate) passes: Vec<FilterPass>,
    pub(crate) output_framebuffers: Box<[OwnedImage]>,
    pub(crate) feedback_framebuffers: Box<[OwnedImage]>,
    pub(crate) history_framebuffers: VecDeque<OwnedImage>,
    pub(crate) staging_heap: D3D12DescriptorHeap<CpuStagingHeap>,
    pub(crate) rtv_heap: D3D12DescriptorHeap<RenderTargetHeap>,

    work_heap: ID3D12DescriptorHeap,
    sampler_heap: ID3D12DescriptorHeap,

    residuals: FrameResiduals,
    mipmap_heap: D3D12DescriptorHeap<ResourceWorkHeap>,

    disable_mipmaps: bool,

    default_options: FrameOptionsD3D12,
    draw_last_pass_feedback: bool,
}

pub(crate) struct FilterCommon {
    pub(crate) d3d12: ID3D12Device,
    pub samplers: SamplerSet,
    pub output_textures: Box<[Option<InputTexture>]>,
    pub feedback_textures: Box<[Option<InputTexture>]>,
    pub history_textures: Box<[Option<InputTexture>]>,
    pub config: RuntimeParameters,
    // pub disable_mipmaps: bool,
    pub luts: FastHashMap<usize, LutTexture>,
    pub mipmap_gen: D3D12MipmapGen,
    pub root_signature: D3D12RootSignature,
    pub draw_quad: DrawQuad,
    allocator: Arc<Mutex<Allocator>>,
}

pub(crate) struct FrameResiduals {
    outputs: Vec<OutputDescriptor>,
    mipmaps: Vec<D3D12DescriptorHeapSlot<ResourceWorkHeap>>,
    mipmap_luts: Vec<D3D12MipmapGen>,
    resources: Vec<ManuallyDrop<Option<ID3D12Resource>>>,
    resource_barriers: Vec<D3D12_RESOURCE_BARRIER>,
}

impl FrameResiduals {
    pub fn new() -> Self {
        Self {
            outputs: Vec::new(),
            mipmaps: Vec::new(),
            mipmap_luts: Vec::new(),
            resources: Vec::new(),
            resource_barriers: Vec::new(),
        }
    }

    pub fn dispose_mipmap_gen(&mut self, mipmap: D3D12MipmapGen) {
        self.mipmap_luts.push(mipmap)
    }

    pub fn dispose_output(&mut self, descriptor: OutputDescriptor) {
        self.outputs.push(descriptor)
    }

    pub fn dispose_mipmap_handles(
        &mut self,
        handles: Vec<D3D12DescriptorHeapSlot<ResourceWorkHeap>>,
    ) {
        self.mipmaps.extend(handles)
    }

    pub unsafe fn dispose_resource(&mut self, resource: ManuallyDrop<Option<ID3D12Resource>>) {
        self.resources.push(resource)
    }

    /// Disposition only handles transition barriers.
    ///
    /// **Safety:** It is only safe to dispose a barrier created with resource strategy IncrementRef.
    ///
    pub unsafe fn dispose_barriers(
        &mut self,
        barrier: impl IntoIterator<Item = D3D12_RESOURCE_BARRIER>,
    ) {
        self.resource_barriers.extend(barrier);
    }

    pub fn dispose(&mut self) {
        self.outputs.clear();
        self.mipmaps.clear();
        for resource in self.resources.drain(..) {
            drop(ManuallyDrop::into_inner(resource))
        }
        for barrier in self.resource_barriers.drain(..) {
            if barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION {
                if let Some(resource) = unsafe { barrier.Anonymous.Transition }.pResource.take() {
                    drop(resource)
                }
            } else if barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_UAV {
                if let Some(resource) = unsafe { barrier.Anonymous.UAV }.pResource.take() {
                    drop(resource)
                }
            }
            // other barrier types should be handled manually
        }
    }
}

impl Drop for FrameResiduals {
    fn drop(&mut self) {
        self.dispose();
    }
}

mod compile {
    use super::*;
    use librashader_pack::PassResource;

    #[cfg(not(feature = "stable"))]
    pub type DxilShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<DXIL, SpirvCompilation, SpirvCross> + Send>;

    #[cfg(feature = "stable")]
    pub type DxilShaderPassMeta = ShaderPassArtifact<
        Box<dyn CompileReflectShader<DXIL, SpirvCompilation, SpirvCross> + Send>,
    >;

    pub fn compile_passes_dxil(
        shaders: Vec<PassResource>,
        textures: &[TextureResource],
        disable_cache: bool,
    ) -> Result<(Vec<DxilShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = if !disable_cache {
            DXIL::compile_preset_passes::<
                CachedCompilation<SpirvCompilation>,
                SpirvCross,
                FilterChainError,
            >(shaders, textures.iter().map(|t| &t.meta))?
        } else {
            DXIL::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
                shaders,
                textures.iter().map(|t| &t.meta),
            )?
        };

        Ok((passes, semantics))
    }

    #[cfg(not(feature = "stable"))]
    pub type HlslShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<HLSL, SpirvCompilation, SpirvCross> + Send>;

    #[cfg(feature = "stable")]
    pub type HlslShaderPassMeta = ShaderPassArtifact<
        Box<dyn CompileReflectShader<HLSL, SpirvCompilation, SpirvCross> + Send>,
    >;

    pub fn compile_passes_hlsl(
        shaders: Vec<PassResource>,
        textures: &[TextureResource],
        disable_cache: bool,
    ) -> Result<(Vec<HlslShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = if !disable_cache {
            HLSL::compile_preset_passes::<
                CachedCompilation<SpirvCompilation>,
                SpirvCross,
                FilterChainError,
            >(shaders, textures.iter().map(|t| &t.meta))?
        } else {
            HLSL::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
                shaders,
                textures.iter().map(|t| &t.meta),
            )?
        };

        Ok((passes, semantics))
    }
}

use crate::resource::OutlivesFrame;
use compile::{compile_passes_dxil, compile_passes_hlsl, DxilShaderPassMeta, HlslShaderPassMeta};
use librashader_pack::{ShaderPresetPack, TextureResource};
use librashader_runtime::parameters::RuntimeParameters;

impl FilterChainD3D12 {
    /// Load the shader preset at the given path into a filter chain.
    pub unsafe fn load_from_path(
        path: impl AsRef<Path>,
        device: &ID3D12Device,
        options: Option<&FilterChainOptionsD3D12>,
    ) -> error::Result<FilterChainD3D12> {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::Direct3D12)?;

        unsafe { Self::load_from_preset(preset, device, options) }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub unsafe fn load_from_preset(
        preset: ShaderPreset,
        device: &ID3D12Device,
        options: Option<&FilterChainOptionsD3D12>,
    ) -> error::Result<FilterChainD3D12> {
        let preset = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        unsafe { Self::load_from_pack(preset, device, options) }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub unsafe fn load_from_pack(
        preset: ShaderPresetPack,
        device: &ID3D12Device,
        options: Option<&FilterChainOptionsD3D12>,
    ) -> error::Result<FilterChainD3D12> {
        unsafe {
            // 1 time queue infrastructure for lut uploads
            let command_pool: ID3D12CommandAllocator =
                device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT)?;
            let cmd: ID3D12GraphicsCommandList =
                device.CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, &command_pool, None)?;
            let queue: ID3D12CommandQueue =
                device.CreateCommandQueue(&D3D12_COMMAND_QUEUE_DESC {
                    Type: D3D12_COMMAND_LIST_TYPE_DIRECT,
                    Priority: 0,
                    Flags: D3D12_COMMAND_QUEUE_FLAG_NONE,
                    NodeMask: 0,
                })?;

            let fence_event = CreateEventA(None, false, false, None)?;
            let fence: ID3D12Fence = device.CreateFence(0, D3D12_FENCE_FLAG_NONE)?;

            let filter_chain = Self::load_from_pack_deferred(preset, device, &cmd, options)?;

            cmd.Close()?;
            queue.ExecuteCommandLists(&[Some(cmd.cast()?)]);
            queue.Signal(&fence, 1)?;

            if fence.GetCompletedValue() < 1 {
                fence.SetEventOnCompletion(1, fence_event)?;
                WaitForSingleObject(fence_event, INFINITE);
                CloseHandle(fence_event)?;
            }

            Ok(filter_chain)
        }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command list must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command list and immediately submitting it to a
    /// graphics queue. The command list must be completely executed before calling [`frame`](Self::frame).
    pub unsafe fn load_from_preset_deferred(
        preset: ShaderPreset,
        device: &ID3D12Device,
        cmd: &ID3D12GraphicsCommandList,
        options: Option<&FilterChainOptionsD3D12>,
    ) -> error::Result<FilterChainD3D12> {
        let preset = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        unsafe { Self::load_from_pack_deferred(preset, device, cmd, options) }
    }

    /// Load a filter chain from a pre-parsed, loaded `ShaderPresetPack`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command list must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command list and immediately submitting it to a
    /// graphics queue. The command list must be completely executed before calling [`frame`](Self::frame).
    pub unsafe fn load_from_pack_deferred(
        preset: ShaderPresetPack,
        device: &ID3D12Device,
        cmd: &ID3D12GraphicsCommandList,
        options: Option<&FilterChainOptionsD3D12>,
    ) -> error::Result<FilterChainD3D12> {
        let shader_count = preset.passes.len();
        let lut_count = preset.textures.len();

        let shader_copy = preset.passes.clone();
        let disable_cache = options.map_or(false, |o| o.disable_cache);

        let (passes, semantics) =
            compile_passes_dxil(preset.passes, &preset.textures, disable_cache)?;
        let (hlsl_passes, _) = compile_passes_hlsl(shader_copy, &preset.textures, disable_cache)?;

        let samplers = SamplerSet::new(device)?;
        let mipmap_gen = D3D12MipmapGen::new(device, false)?;

        let allocator = Arc::new(Mutex::new(Allocator::new(&AllocatorCreateDesc {
            device: ID3D12DeviceVersion::Device(device.clone()),
            debug_settings: Default::default(),
            allocation_sizes: Default::default(),
        })?));

        let draw_quad = DrawQuad::new(&allocator)?;
        let mut staging_heap = unsafe {
            D3D12DescriptorHeap::new(
                device,
                // add one, because technically the input image doesn't need to count
                (1 + MAX_BINDINGS_COUNT as usize) * shader_count
                    + MIPMAP_RESERVED_WORKHEAP_DESCRIPTORS
                    + lut_count,
            )
        }?;
        let rtv_heap = unsafe {
            D3D12DescriptorHeap::new(
                device,
                (1 + MAX_BINDINGS_COUNT as usize) * shader_count
                    + MIPMAP_RESERVED_WORKHEAP_DESCRIPTORS
                    + lut_count,
            )
        }?;

        let root_signature = D3D12RootSignature::new(device)?;

        let (texture_heap, sampler_heap, filters, mut mipmap_heap) = FilterChainD3D12::init_passes(
            device,
            &root_signature,
            &allocator,
            passes,
            hlsl_passes,
            &semantics,
            options.map_or(false, |o| o.force_hlsl_pipeline),
            disable_cache,
        )?;

        let mut residuals = FrameResiduals::new();

        let luts = FilterChainD3D12::load_luts(
            device,
            cmd,
            &allocator,
            &mut staging_heap,
            &mut mipmap_heap,
            &mut residuals,
            preset.textures,
        )?;

        let framebuffer_gen = || {
            OwnedImage::new(
                device,
                &allocator,
                Size::new(1, 1),
                ImageFormat::R8G8B8A8Unorm.into(),
                false,
            )
        };
        let input_gen = || None;
        let framebuffer_init = FramebufferInit::new(
            filters.iter().map(|f| &f.reflection.meta),
            &framebuffer_gen,
            &input_gen,
        );

        // initialize output framebuffers
        let (output_framebuffers, output_textures) = framebuffer_init.init_output_framebuffers()?;

        // initialize feedback framebuffers
        let (feedback_framebuffers, feedback_textures) =
            framebuffer_init.init_output_framebuffers()?;

        // initialize history
        let (history_framebuffers, history_textures) = framebuffer_init.init_history()?;

        Ok(FilterChainD3D12 {
            draw_last_pass_feedback: framebuffer_init.uses_final_pass_as_feedback(),
            common: FilterCommon {
                d3d12: device.clone(),
                samplers,
                allocator,
                output_textures,
                feedback_textures,
                luts,
                mipmap_gen,
                root_signature,
                draw_quad,
                config: RuntimeParameters::new(preset.pass_count as usize, preset.parameters),
                history_textures,
            },
            staging_heap,
            rtv_heap,
            passes: filters,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            work_heap: texture_heap,
            sampler_heap,
            mipmap_heap,
            disable_mipmaps: options.map_or(false, |o| o.force_no_mipmaps),
            residuals,
            default_options: Default::default(),
        })
    }

    fn load_luts(
        device: &ID3D12Device,
        cmd: &ID3D12GraphicsCommandList,
        allocator: &Arc<Mutex<Allocator>>,
        staging_heap: &mut D3D12DescriptorHeap<CpuStagingHeap>,
        mipmap_heap: &mut D3D12DescriptorHeap<ResourceWorkHeap>,
        gc: &mut FrameResiduals,
        textures: Vec<TextureResource>,
    ) -> error::Result<FastHashMap<usize, LutTexture>> {
        // use separate mipgen to load luts.
        let mipmap_gen = D3D12MipmapGen::new(device, true)?;

        let mut luts = FastHashMap::default();
        let textures = textures
            .into_par_iter()
            .map(|texture| LoadedTexture::from_texture(texture, UVDirection::TopLeft))
            .collect::<Result<Vec<LoadedTexture>, ImageError>>()?;

        for (index, LoadedTexture { meta, image }) in textures.iter().enumerate() {
            let texture = LutTexture::new(
                device,
                allocator,
                staging_heap,
                cmd,
                &image,
                meta.filter_mode,
                meta.wrap_mode,
                meta.mipmap,
                gc,
            )?;
            luts.insert(index, texture);
        }

        let (residual_mipmap, residual_barrier) =
            mipmap_gen.mipmapping_context(cmd, mipmap_heap, |context| {
                for lut in luts.values() {
                    lut.generate_mipmaps(context)?;
                }

                Ok::<(), FilterChainError>(())
            })?;

        gc.dispose_mipmap_handles(residual_mipmap);
        gc.dispose_mipmap_gen(mipmap_gen);

        unsafe {
            gc.dispose_barriers(residual_barrier);
        }

        Ok(luts)
    }

    fn init_passes(
        device: &ID3D12Device,
        root_signature: &D3D12RootSignature,
        allocator: &Arc<Mutex<Allocator>>,
        passes: Vec<DxilShaderPassMeta>,
        hlsl_passes: Vec<HlslShaderPassMeta>,
        semantics: &ShaderSemantics,
        force_hlsl: bool,
        disable_cache: bool,
    ) -> error::Result<(
        ID3D12DescriptorHeap,
        ID3D12DescriptorHeap,
        Vec<FilterPass>,
        D3D12DescriptorHeap<ResourceWorkHeap>,
    )> {
        let shader_count = passes.len();
        let D3D12PartitionedHeap {
            partitioned: work_heaps,
            reserved: mipmap_heap,
            handle: texture_heap_handle,
        } = unsafe {
            let work_heap = D3D12PartitionableHeap::<ResourceWorkHeap>::new(
                device,
                (MAX_BINDINGS_COUNT as usize) * shader_count + MIPMAP_RESERVED_WORKHEAP_DESCRIPTORS,
            )?;

            work_heap.into_partitioned(
                MAX_BINDINGS_COUNT as usize,
                MIPMAP_RESERVED_WORKHEAP_DESCRIPTORS,
            )?
        };

        let D3D12PartitionedHeap {
            partitioned: sampler_work_heaps,
            reserved: _,
            handle: sampler_heap_handle,
        } = unsafe {
            let sampler_heap =
                D3D12PartitionableHeap::new(device, (MAX_BINDINGS_COUNT as usize) * shader_count)?;
            sampler_heap.into_partitioned(MAX_BINDINGS_COUNT as usize, 0)?
        };

        let filters: Vec<error::Result<_>> = passes
            .into_par_iter()
            .zip(hlsl_passes)
            .zip(work_heaps)
            .zip(sampler_work_heaps)
            .enumerate()
            .map_init(
                || {
                    let validator: IDxcValidator =
                        unsafe { DxcCreateInstance(&CLSID_DxcValidator)? };
                    let library: IDxcUtils = unsafe { DxcCreateInstance(&CLSID_DxcLibrary)? };
                    let compiler: IDxcCompiler = unsafe { DxcCreateInstance(&CLSID_DxcCompiler)? };
                    Ok::<_, FilterChainError>((validator, library, compiler))
                },
                |dxc,
                 (
                    index,
                    ((((config, mut dxil), (_, mut hlsl)), mut texture_heap), mut sampler_heap),
                )| {
                    let Ok((validator, library, compiler)) = dxc else {
                        return Err(FilterChainError::Direct3DOperationError(
                            "Could not initialize DXC for thread",
                        ));
                    };

                    let dxil_reflection = dxil.reflect(index, semantics)?;
                    let dxil = dxil.compile(Some(
                        librashader_reflect::back::dxil::ShaderModel::ShaderModel6_0,
                    ))?;

                    let render_format = if let Some(format) = config.meta.get_format_override() {
                        format
                    } else if config.data.format != ImageFormat::Unknown {
                        config.data.format
                    } else {
                        ImageFormat::R8G8B8A8Unorm
                    }
                    .into();

                    // incredibly cursed.
                    let (reflection, graphics_pipeline) = 'pipeline: {
                        'dxil: {
                            if force_hlsl {
                                break 'dxil;
                            }

                            if let Ok(graphics_pipeline) = D3D12GraphicsPipeline::new_from_dxil(
                                device,
                                library,
                                validator,
                                &dxil,
                                root_signature,
                                render_format,
                                disable_cache,
                            ) {
                                break 'pipeline (dxil_reflection, graphics_pipeline);
                            }
                        }

                        let hlsl_reflection = hlsl.reflect(index, semantics)?;
                        let hlsl = hlsl.compile(Some(
                            librashader_reflect::back::hlsl::HlslShaderModel::ShaderModel6_0,
                        ))?;

                        let graphics_pipeline = D3D12GraphicsPipeline::new_from_hlsl(
                            device,
                            library,
                            compiler,
                            &hlsl,
                            root_signature,
                            render_format,
                            disable_cache,
                        )?;
                        (hlsl_reflection, graphics_pipeline)
                    };

                    // minimum size here has to be 1 byte.
                    let ubo_size = reflection.ubo.as_ref().map_or(1, |ubo| ubo.size as usize);
                    let push_size = reflection
                        .push_constant
                        .as_ref()
                        .map_or(1, |push| push.size as usize);

                    let uniform_storage = UniformStorage::new_with_storage(
                        RawD3D12Buffer::new(D3D12Buffer::new(allocator, ubo_size)?)?,
                        RawD3D12Buffer::new(D3D12Buffer::new(allocator, push_size)?)?,
                    );

                    let uniform_bindings =
                        reflection.meta.create_binding_map(|param| param.offset());

                    let texture_heap = texture_heap.allocate_descriptor_range()?;
                    let sampler_heap = sampler_heap.allocate_descriptor_range()?;

                    Ok(FilterPass {
                        reflection,
                        uniform_bindings,
                        uniform_storage,
                        pipeline: graphics_pipeline,
                        meta: config.meta,
                        texture_heap,
                        sampler_heap,
                        source: config.data,
                    })
                },
            )
            .collect();

        let filters: error::Result<Vec<_>> = filters.into_iter().collect();
        let filters = filters?;

        // Panic SAFETY: mipmap_heap is always 1024 descriptors.
        Ok((
            texture_heap_handle,
            sampler_heap_handle,
            filters,
            mipmap_heap.unwrap(),
        ))
    }

    fn push_history(
        &mut self,
        cmd: &ID3D12GraphicsCommandList,
        input: &InputTexture,
    ) -> error::Result<()> {
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            if back.size != input.size
                || (input.format != DXGI_FORMAT_UNKNOWN && input.format != back.format.into())
            {
                // eprintln!("[history] resizing");
                // old back will get dropped.. do we need to defer?
                let _old_back = std::mem::replace(
                    &mut back,
                    OwnedImage::new(
                        &self.common.d3d12,
                        &self.common.allocator,
                        input.size,
                        input.format,
                        false,
                    )?,
                );
            }
            unsafe {
                back.copy_from(cmd, input)?;
            }
            self.history_framebuffers.push_front(back);
        }

        Ok(())
    }

    /// Records shader rendering commands to the provided command list.
    ///
    /// * The input image must be in the `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` resource state.
    /// * The output image must be in `D3D12_RESOURCE_STATE_RENDER_TARGET` resource state.
    ///
    /// librashader **will not** create a resource barrier for the final pass. The output image will
    /// remain in `D3D12_RESOURCE_STATE_RENDER_TARGET` after all shader passes. The caller must transition
    /// the output image to the final resource state.
    ///
    /// The input and output images must stay alive until the command list is submitted and work is complete.
    pub unsafe fn frame(
        &mut self,
        cmd: &ID3D12GraphicsCommandList,
        input: D3D12InputImage,
        viewport: &Viewport<D3D12OutputView>,
        frame_count: usize,
        options: Option<&FrameOptionsD3D12>,
    ) -> error::Result<()> {
        self.residuals.dispose();

        // limit number of passes to those enabled.
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled());
        let passes = &mut self.passes[0..max];
        if passes.is_empty() {
            return Ok(());
        }

        if let Some(options) = options {
            if options.clear_history {
                for framebuffer in &mut self.history_framebuffers {
                    framebuffer.clear(cmd, &mut self.rtv_heap)?;
                }
            }
        }

        let options = options.unwrap_or(&self.default_options);

        let filter = passes[0].meta.filter;
        let wrap_mode = passes[0].meta.wrap_mode;

        for ((texture, fbo), pass) in self
            .common
            .feedback_textures
            .iter_mut()
            .zip(self.feedback_framebuffers.iter())
            .zip(passes.iter())
        {
            *texture = Some(fbo.create_shader_resource_view(
                &mut self.staging_heap,
                pass.meta.filter,
                pass.meta.wrap_mode,
            )?);
        }

        for (texture, fbo) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            *texture =
                Some(fbo.create_shader_resource_view(&mut self.staging_heap, filter, wrap_mode)?);
        }

        let original = unsafe {
            match input {
                D3D12InputImage::Managed(input) => InputTexture::new_from_resource(
                    input,
                    filter,
                    wrap_mode,
                    &self.common.d3d12,
                    &mut self.staging_heap,
                )?,
                D3D12InputImage::External {
                    resource,
                    descriptor,
                } => InputTexture::new_from_raw(resource, descriptor, filter, wrap_mode),
            }
        };

        let mut source = original.clone();

        // swap output and feedback **before** recording command buffers
        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        // rescale render buffers to ensure all bindings are valid.
        OwnedImage::scale_framebuffers(
            source.size(),
            viewport.output.size,
            original.size(),
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
            passes,
            Some(&mut |index, pass, output, feedback| {
                // refresh inputs
                self.common.feedback_textures[index] = Some(feedback.create_shader_resource_view(
                    &mut self.staging_heap,
                    pass.meta.filter,
                    pass.meta.wrap_mode,
                )?);
                self.common.output_textures[index] = Some(output.create_shader_resource_view(
                    &mut self.staging_heap,
                    pass.meta.filter,
                    pass.meta.wrap_mode,
                )?);

                Ok(())
            }),
        )?;

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);

        unsafe {
            let heaps = [
                Some(self.work_heap.clone()),
                Some(self.sampler_heap.clone()),
            ];
            cmd.SetDescriptorHeaps(&heaps);
            cmd.SetGraphicsRootSignature(&self.common.root_signature.handle);
            self.common.mipmap_gen.pin_root_signature(cmd);
        }

        self.common.draw_quad.bind_vertices_for_frame(cmd);

        for (index, pass) in pass.iter_mut().enumerate() {
            source.filter = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;

            let target = &self.output_framebuffers[index];

            if !pass.pipeline.has_format(target.format) {
                // eprintln!("recompiling final pipeline");
                pass.pipeline.recompile(
                    target.format,
                    &self.common.root_signature,
                    &self.common.d3d12,
                )?;
            }

            util::d3d12_resource_transition::<OutlivesFrame, _>(
                cmd,
                &target.resource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
            );

            let view = target.create_render_target_view(&mut self.rtv_heap)?;
            let out = RenderTarget::identity(&view)?;

            pass.draw(
                cmd,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Offscreen,
            )?;

            util::d3d12_resource_transition::<OutlivesFrame, _>(
                cmd,
                &target.resource,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            );

            if target.max_mipmap > 1 && !self.disable_mipmaps {
                // barriers don't get disposed because the context is OutlivesFrame
                let (residuals, _residual_barriers) = self.common.mipmap_gen.mipmapping_context(
                    cmd,
                    &mut self.mipmap_heap,
                    |ctx| {
                        ctx.generate_mipmaps::<OutlivesFrame, _>(
                            &target.resource,
                            target.max_mipmap,
                            target.size,
                            target.format.into(),
                        )?;
                        Ok::<(), FilterChainError>(())
                    },
                )?;

                self.residuals.dispose_mipmap_handles(residuals);
            }

            self.residuals.dispose_output(view.descriptor);
            source = self.common.output_textures[index].as_ref().unwrap().clone()
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);
        if let Some(pass) = last.iter_mut().next() {
            let index = passes_len - 1;

            source.filter = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;

            if self.draw_last_pass_feedback {
                let feedback_target = &self.output_framebuffers[index];

                if !pass.pipeline.has_format(feedback_target.format) {
                    // eprintln!("recompiling final pipeline");
                    pass.pipeline.recompile(
                        feedback_target.format,
                        &self.common.root_signature,
                        &self.common.d3d12,
                    )?;
                }

                util::d3d12_resource_transition::<OutlivesFrame, _>(
                    cmd,
                    &feedback_target.resource,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                );

                let view = feedback_target.create_render_target_view(&mut self.rtv_heap)?;
                let out = RenderTarget::viewport_with_output(&view, viewport);
                pass.draw(
                    cmd,
                    index,
                    &self.common,
                    pass.meta.get_frame_count(frame_count),
                    options,
                    viewport,
                    &original,
                    &source,
                    &out,
                    QuadType::Final,
                )?;

                util::d3d12_resource_transition::<OutlivesFrame, _>(
                    cmd,
                    &feedback_target.resource,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                );
            }

            if !pass.pipeline.has_format(viewport.output.format) {
                // eprintln!("recompiling final pipeline");
                pass.pipeline.recompile(
                    viewport.output.format,
                    &self.common.root_signature,
                    &self.common.d3d12,
                )?;
            }

            let out = RenderTarget::viewport(viewport);
            pass.draw(
                cmd,
                passes_len - 1,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Final,
            )?;
        }

        self.push_history(cmd, &original)?;

        Ok(())
    }
}
