use crate::draw_quad::DrawQuad;
use crate::error::FilterChainError;
use crate::filter_pass::FilterPass;
use crate::framebuffer::OutputImage;
use crate::graphics_pipeline::VulkanGraphicsPipeline;
use crate::luts::LutTexture;
use crate::memory::RawVulkanBuffer;
use crate::options::{FilterChainOptionsVulkan, FrameOptionsVulkan};
use crate::queue_selection::get_graphics_queue;
use crate::samplers::SamplerSet;
use crate::texture::{InputImage, OwnedImage, OwnedImageLayout, VulkanImage};
use crate::{error, memory, util};
use ash::vk;
use librashader_common::{ImageFormat, Size, Viewport};

use ash::vk::Handle;
use gpu_allocator::vulkan::Allocator;
use librashader_cache::CachedCompilation;
use librashader_common::map::FastHashMap;
use librashader_presets::context::VideoDriver;
use librashader_presets::ShaderPreset;
use librashader_reflect::back::targets::SPIRV;
use librashader_reflect::back::{CompileReflectShader, CompileShader};
use librashader_reflect::front::SpirvCompilation;
use librashader_reflect::reflect::cross::SpirvCross;
use librashader_reflect::reflect::presets::{CompilePresetTarget, ShaderPassArtifact};
use librashader_reflect::reflect::semantics::ShaderSemantics;
use librashader_reflect::reflect::ReflectShader;
use librashader_runtime::binding::BindingUtil;
use librashader_runtime::framebuffer::FramebufferInit;
use librashader_runtime::image::{ImageError, LoadedTexture, UVDirection, BGRA8};
use librashader_runtime::quad::QuadType;
use librashader_runtime::render_target::RenderTarget;
use librashader_runtime::scaling::ScaleFramebuffer;
use librashader_runtime::uniforms::UniformStorage;
use parking_lot::Mutex;
use rayon::prelude::*;
use std::collections::VecDeque;
use std::convert::Infallible;
use std::path::Path;
use std::sync::Arc;

/// A Vulkan device and metadata that is required by the shader runtime.
pub struct VulkanObjects {
    /// The handle to the initialized `ash::Device`
    pub device: Arc<ash::Device>,
    /// The instance of the `gpu-allocator` to use.
    pub alloc: Arc<Mutex<Allocator>>,
    /// The graphics queue to do work on.
    pub queue: vk::Queue,
}

/// A collection of handles needed to access the Vulkan instance.
#[derive(Clone)]
pub struct VulkanInstance {
    /// A `VkDevice` handle.
    pub device: vk::Device,
    /// A `VkInstance` handle.
    pub instance: vk::Instance,
    /// A `VkPhysicalDevice` handle.
    pub physical_device: vk::PhysicalDevice,
    /// A function pointer to the Vulkan library entry point.
    /// If this is `None`, [`FilterChainError::HandleIsNull`] will be returned.
    pub get_instance_proc_addr: Option<vk::PFN_vkGetInstanceProcAddr>,
    /// The graphics queue to use to submit commands. If this is `None`,
    /// a queue will be chosen.
    pub queue: Option<vk::Queue>,
}

impl TryFrom<VulkanInstance> for VulkanObjects {
    type Error = FilterChainError;

    fn try_from(vulkan: VulkanInstance) -> Result<Self, FilterChainError> {
        if vulkan.queue.is_some_and(|q| q.is_null())
            || vulkan.device.is_null()
            || vulkan.instance.is_null()
        {
            return Err(FilterChainError::HandleIsNull);
        };

        let Some(get_instance_proc_addr) = vulkan.get_instance_proc_addr else {
            return Err(FilterChainError::HandleIsNull);
        };

        unsafe {
            let instance = ash::Instance::load(
                &ash::StaticFn {
                    get_instance_proc_addr,
                },
                vulkan.instance,
            );

            let device = ash::Device::load(instance.fp_v1_0(), vulkan.device);

            let queue = vulkan.queue.unwrap_or(get_graphics_queue(
                &instance,
                &device,
                vulkan.physical_device,
            ));

            let alloc = memory::create_allocator(device.clone(), instance, vulkan.physical_device)?;

            Ok(VulkanObjects {
                device: Arc::new(device),
                alloc,
                queue,
            })
        }
    }
}

impl TryFrom<(vk::PhysicalDevice, ash::Instance, ash::Device)> for VulkanObjects {
    type Error = FilterChainError;

    fn try_from(value: (vk::PhysicalDevice, ash::Instance, ash::Device)) -> error::Result<Self> {
        if value.0.is_null() {
            return Err(FilterChainError::HandleIsNull);
        }

        let device = value.2;

        let queue = get_graphics_queue(&value.1, &device, value.0);

        let alloc = memory::create_allocator(device.clone(), value.1, value.0)?;

        Ok(VulkanObjects {
            alloc,
            device: Arc::new(device),
            queue,
        })
    }
}

impl TryFrom<(vk::PhysicalDevice, ash::Instance, ash::Device, vk::Queue)> for VulkanObjects {
    type Error = FilterChainError;

    fn try_from(
        value: (vk::PhysicalDevice, ash::Instance, ash::Device, vk::Queue),
    ) -> error::Result<Self> {
        if value.0.is_null() {
            return Err(FilterChainError::HandleIsNull);
        }

        let device = value.2;
        let queue = if value.3.is_null() {
            get_graphics_queue(&value.1, &device, value.0)
        } else {
            value.3
        };

        let alloc = memory::create_allocator(device.clone(), value.1, value.0)?;

        Ok(VulkanObjects {
            alloc,
            device: Arc::new(device),
            queue,
        })
    }
}

/// A Vulkan filter chain.
pub struct FilterChainVulkan {
    pub(crate) common: FilterCommon,
    passes: Box<[FilterPass]>,
    vulkan: VulkanObjects,
    output_framebuffers: Box<[OwnedImage]>,
    feedback_framebuffers: Box<[OwnedImage]>,
    history_framebuffers: VecDeque<OwnedImage>,
    disable_mipmaps: bool,
    residuals: Box<[FrameResiduals]>,
    default_options: FrameOptionsVulkan,
    draw_last_pass_feedback: bool,
}

pub(crate) struct FilterCommon {
    pub(crate) luts: FastHashMap<usize, LutTexture>,
    pub samplers: SamplerSet,
    pub(crate) draw_quad: DrawQuad,
    pub output_textures: Box<[Option<InputImage>]>,
    pub feedback_textures: Box<[Option<InputImage>]>,
    pub history_textures: Box<[Option<InputImage>]>,
    pub config: RuntimeParameters,
    pub device: Arc<ash::Device>,
    pub(crate) internal_frame_count: usize,
}

/// Contains residual intermediate `VkImageView` and `VkImage` objects created
/// for intermediate shader passes.
///
/// These Vulkan objects must stay alive until the command buffer is submitted
/// to the rendering queue, and the GPU is done with the objects.
#[must_use]
struct FrameResiduals {
    device: ash::Device,
    image_views: Vec<vk::ImageView>,
    owned: Vec<OwnedImage>,
    framebuffers: Vec<Option<vk::Framebuffer>>,
}

impl FrameResiduals {
    pub(crate) fn new(device: &ash::Device) -> Self {
        FrameResiduals {
            device: device.clone(),
            image_views: Vec::new(),
            owned: Vec::new(),
            framebuffers: Vec::new(),
        }
    }

    pub(crate) fn dispose_outputs(&mut self, output_framebuffer: OutputImage) {
        self.image_views.push(output_framebuffer.image_view);
    }

    pub(crate) fn dispose_owned(&mut self, owned: OwnedImage) {
        self.owned.push(owned)
    }

    pub(crate) fn dispose_framebuffers(&mut self, fb: Option<vk::Framebuffer>) {
        self.framebuffers.push(fb)
    }

    /// Dispose of the intermediate objects created during a frame.
    pub fn dispose(&mut self) {
        for image_view in self.image_views.drain(0..) {
            if image_view != vk::ImageView::null() {
                unsafe {
                    self.device.destroy_image_view(image_view, None);
                }
            }
        }
        for framebuffer in self.framebuffers.drain(0..) {
            if let Some(framebuffer) =
                framebuffer.filter(|framebuffer| *framebuffer != vk::Framebuffer::null())
            {
                unsafe {
                    self.device.destroy_framebuffer(framebuffer, None);
                }
            }
        }
        self.owned.clear()
    }
}

impl Drop for FrameResiduals {
    fn drop(&mut self) {
        // Will not double free because dispose removes active items from storage.
        self.dispose()
    }
}

mod compile {
    use super::*;
    use librashader_pack::PassResource;

    #[cfg(not(feature = "stable"))]
    pub type ShaderPassMeta =
        ShaderPassArtifact<impl CompileReflectShader<SPIRV, SpirvCompilation, SpirvCross> + Send>;

    #[cfg(feature = "stable")]
    pub type ShaderPassMeta = ShaderPassArtifact<
        Box<dyn CompileReflectShader<SPIRV, SpirvCompilation, SpirvCross> + Send>,
    >;

    pub fn compile_passes(
        shaders: Vec<PassResource>,
        textures: &[TextureResource],
        disable_cache: bool,
    ) -> Result<(Vec<ShaderPassMeta>, ShaderSemantics), FilterChainError> {
        let (passes, semantics) = if !disable_cache {
            SPIRV::compile_preset_passes::<
                CachedCompilation<SpirvCompilation>,
                SpirvCross,
                FilterChainError,
            >(shaders, textures.iter().map(|t| &t.meta))?
        } else {
            SPIRV::compile_preset_passes::<SpirvCompilation, SpirvCross, FilterChainError>(
                shaders,
                textures.iter().map(|t| &t.meta),
            )?
        };

        Ok((passes, semantics))
    }
}

use compile::{compile_passes, ShaderPassMeta};
use librashader_pack::{ShaderPresetPack, TextureResource};
use librashader_runtime::parameters::RuntimeParameters;

impl FilterChainVulkan {
    /// Load the shader preset at the given path into a filter chain.
    pub unsafe fn load_from_path<V, E>(
        path: impl AsRef<Path>,
        vulkan: V,
        options: Option<&FilterChainOptionsVulkan>,
    ) -> error::Result<FilterChainVulkan>
    where
        V: TryInto<VulkanObjects, Error = E>,
        FilterChainError: From<E>,
    {
        // load passes from preset
        let preset = ShaderPreset::try_parse_with_driver_context(path, VideoDriver::Vulkan)?;
        unsafe { Self::load_from_preset::<V, E>(preset, vulkan, options) }
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`.
    pub unsafe fn load_from_preset<V, E>(
        preset: ShaderPreset,
        vulkan: V,
        options: Option<&FilterChainOptionsVulkan>,
    ) -> error::Result<FilterChainVulkan>
    where
        V: TryInto<VulkanObjects, Error = E>,
        FilterChainError: From<E>,
    {
        let pack = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        unsafe { Self::load_from_pack(pack, vulkan, options) }
    }

    /// Load a filter chain from a pre-parsed and loaded `ShaderPresetPack`.
    pub unsafe fn load_from_pack<V, E>(
        preset: ShaderPresetPack,
        vulkan: V,
        options: Option<&FilterChainOptionsVulkan>,
    ) -> error::Result<FilterChainVulkan>
    where
        V: TryInto<VulkanObjects, Error = E>,
        FilterChainError: From<E>,
    {
        let vulkan = vulkan.try_into().map_err(|e| e.into())?;
        let device = Arc::clone(&vulkan.device);
        let queue = vulkan.queue.clone();

        let command_pool = unsafe {
            device.create_command_pool(
                &vk::CommandPoolCreateInfo::default()
                    .flags(vk::CommandPoolCreateFlags::RESET_COMMAND_BUFFER),
                None,
            )?
        };

        let command_buffer = unsafe {
            // panic safety: command buffer count = 1
            device.allocate_command_buffers(
                &vk::CommandBufferAllocateInfo::default()
                    .command_pool(command_pool)
                    .level(vk::CommandBufferLevel::PRIMARY)
                    .command_buffer_count(1),
            )?[0]
        };

        unsafe {
            device.begin_command_buffer(
                command_buffer,
                &vk::CommandBufferBeginInfo::default()
                    .flags(vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT),
            )?
        }

        let filter_chain = unsafe {
            Self::load_from_pack_deferred::<_, Infallible>(preset, vulkan, command_buffer, options)?
        };

        unsafe {
            device.end_command_buffer(command_buffer)?;

            let buffers = [command_buffer];
            let submit_info = vk::SubmitInfo::default().command_buffers(&buffers);

            device.queue_submit(queue, &[submit_info], vk::Fence::null())?;
            device.queue_wait_idle(queue)?;
            device.free_command_buffers(command_pool, &buffers);
            device.destroy_command_pool(command_pool, None);
        }

        Ok(filter_chain)
    }

    /// Load a filter chain from a pre-parsed `ShaderPreset`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    pub unsafe fn load_from_preset_deferred<V, E>(
        preset: ShaderPreset,
        vulkan: V,
        cmd: vk::CommandBuffer,
        options: Option<&FilterChainOptionsVulkan>,
    ) -> error::Result<FilterChainVulkan>
    where
        V: TryInto<VulkanObjects, Error = E>,
        FilterChainError: From<E>,
    {
        let pack = ShaderPresetPack::load_from_preset::<FilterChainError>(preset)?;
        unsafe { Self::load_from_pack_deferred(pack, vulkan, cmd, options) }
    }

    /// Load a filter chain from a pre-parsed, loaded `ShaderPresetPack`, deferring and GPU-side initialization
    /// to the caller. This function therefore requires no external synchronization of the device queue.
    ///
    /// ## Safety
    /// The provided command buffer must be ready for recording and contain no prior commands.
    /// The caller is responsible for ending the command buffer and immediately submitting it to a
    /// graphics queue. The command buffer must be completely executed before calling [`frame`](Self::frame).
    pub unsafe fn load_from_pack_deferred<V, E>(
        preset: ShaderPresetPack,
        vulkan: V,
        cmd: vk::CommandBuffer,
        options: Option<&FilterChainOptionsVulkan>,
    ) -> error::Result<FilterChainVulkan>
    where
        V: TryInto<VulkanObjects, Error = E>,
        FilterChainError: From<E>,
    {
        let disable_cache = options.map_or(false, |o| o.disable_cache);
        let (passes, semantics) = compile_passes(preset.passes, &preset.textures, disable_cache)?;

        let device = vulkan.try_into().map_err(From::from)?;

        let mut frames_in_flight = options.map_or(0, |o| o.frames_in_flight);
        if frames_in_flight == 0 {
            frames_in_flight = 3;
        }

        // initialize passes
        let filters = Self::init_passes(
            &device,
            passes,
            &semantics,
            frames_in_flight,
            options.map_or(false, |o| o.use_dynamic_rendering),
            disable_cache,
        )?;

        let luts = FilterChainVulkan::load_luts(&device, cmd, preset.textures)?;
        let samplers = SamplerSet::new(&device.device)?;

        let framebuffer_gen =
            || OwnedImage::new(&device, Size::new(1, 1), ImageFormat::R8G8B8A8Unorm, 1);
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

        let mut intermediates = Vec::new();
        intermediates.resize_with(frames_in_flight as usize, || {
            FrameResiduals::new(&device.device)
        });

        Ok(FilterChainVulkan {
            draw_last_pass_feedback: framebuffer_init.uses_final_pass_as_feedback(),
            common: FilterCommon {
                luts,
                samplers,
                config: RuntimeParameters::new(preset.pass_count as usize, preset.parameters),
                draw_quad: DrawQuad::new(&device.device, &device.alloc)?,
                device: device.device.clone(),
                output_textures,
                feedback_textures,
                history_textures,
                internal_frame_count: 0,
            },
            passes: filters,
            vulkan: device,
            output_framebuffers,
            feedback_framebuffers,
            history_framebuffers,
            residuals: intermediates.into_boxed_slice(),
            disable_mipmaps: options.map_or(false, |o| o.force_no_mipmaps),
            default_options: Default::default(),
        })
    }

    fn init_passes(
        vulkan: &VulkanObjects,
        passes: Vec<ShaderPassMeta>,
        semantics: &ShaderSemantics,
        frames_in_flight: u32,
        use_dynamic_rendering: bool,
        disable_cache: bool,
    ) -> error::Result<Box<[FilterPass]>> {
        let frames_in_flight = std::cmp::max(1, frames_in_flight);

        let filters: Vec<error::Result<FilterPass>> = passes
            .into_par_iter()
            .enumerate()
            .map(|(index, (config, mut reflect))| {
                let reflection = reflect.reflect(index, semantics)?;
                let spirv_words = reflect.compile(None)?;

                let ubo_size = reflection.ubo.as_ref().map_or(0, |ubo| ubo.size as usize);
                let uniform_storage = UniformStorage::new_with_ubo_storage(
                    RawVulkanBuffer::new(
                        &vulkan.device,
                        &vulkan.alloc,
                        vk::BufferUsageFlags::UNIFORM_BUFFER,
                        ubo_size,
                    )?,
                    reflection
                        .push_constant
                        .as_ref()
                        .map_or(0, |push| push.size as usize),
                );

                let uniform_bindings = reflection.meta.create_binding_map(|param| param.offset());

                let render_pass_format = if use_dynamic_rendering {
                    vk::Format::UNDEFINED
                } else if let Some(format) = config.meta.get_format_override() {
                    format.into()
                } else if config.data.format != ImageFormat::Unknown {
                    config.data.format.into()
                } else {
                    ImageFormat::R8G8B8A8Unorm.into()
                };

                let graphics_pipeline = VulkanGraphicsPipeline::new(
                    &vulkan.device,
                    &spirv_words,
                    &reflection,
                    frames_in_flight,
                    render_pass_format,
                    disable_cache,
                )?;

                Ok(FilterPass {
                    reflection,
                    // compiled: spirv_words,
                    uniform_storage,
                    uniform_bindings,
                    source: config.data,
                    meta: config.meta,
                    graphics_pipeline,
                    // ubo_ring,
                    frames_in_flight,
                })
            })
            .collect();

        let filters: error::Result<Vec<FilterPass>> = filters.into_iter().collect();
        let filters = filters?;
        Ok(filters.into_boxed_slice())
    }

    fn load_luts(
        vulkan: &VulkanObjects,
        command_buffer: vk::CommandBuffer,
        textures: Vec<TextureResource>,
    ) -> error::Result<FastHashMap<usize, LutTexture>> {
        let mut luts = FastHashMap::default();
        let textures = textures
            .into_par_iter()
            .map(|texture| LoadedTexture::from_texture(texture, UVDirection::TopLeft))
            .collect::<Result<Vec<LoadedTexture<BGRA8>>, ImageError>>()?;
        for (index, LoadedTexture { meta, image }) in textures.into_iter().enumerate() {
            let texture = LutTexture::new(vulkan, command_buffer, image, &meta)?;
            luts.insert(index, texture);
        }
        Ok(luts)
    }

    // image must be in SHADER_READ_OPTIMAL
    fn push_history(&mut self, input: &VulkanImage, cmd: vk::CommandBuffer) -> error::Result<()> {
        if let Some(mut back) = self.history_framebuffers.pop_back() {
            if back.image.size != input.size
                || (input.format != vk::Format::UNDEFINED && input.format != back.image.format)
            {
                // eprintln!("[history] resizing");
                // old back will get dropped.. do we need to defer?
                let old_back = std::mem::replace(
                    &mut back,
                    OwnedImage::new(&self.vulkan, input.size, input.format.into(), 1)?,
                );
                self.residuals[self.common.internal_frame_count % self.residuals.len()]
                    .dispose_owned(old_back);
            }

            unsafe {
                util::vulkan_image_layout_transition_levels(
                    &self.vulkan.device,
                    cmd,
                    input.image,
                    1,
                    vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                    vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
                    vk::AccessFlags::SHADER_READ,
                    vk::AccessFlags::TRANSFER_READ,
                    vk::PipelineStageFlags::FRAGMENT_SHADER,
                    vk::PipelineStageFlags::TRANSFER,
                    vk::QUEUE_FAMILY_IGNORED,
                    vk::QUEUE_FAMILY_IGNORED,
                );

                back.copy_from(cmd, input, vk::ImageLayout::TRANSFER_SRC_OPTIMAL);

                util::vulkan_image_layout_transition_levels(
                    &self.vulkan.device,
                    cmd,
                    input.image,
                    1,
                    vk::ImageLayout::TRANSFER_SRC_OPTIMAL,
                    vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                    vk::AccessFlags::TRANSFER_READ,
                    vk::AccessFlags::SHADER_READ,
                    vk::PipelineStageFlags::TRANSFER,
                    vk::PipelineStageFlags::FRAGMENT_SHADER,
                    vk::QUEUE_FAMILY_IGNORED,
                    vk::QUEUE_FAMILY_IGNORED,
                );
            }

            self.history_framebuffers.push_front(back)
        }

        Ok(())
    }
    /// Records shader rendering commands to the provided command buffer.
    ///
    /// * The input image must be in the `VK_SHADER_READ_ONLY_OPTIMAL` layout.
    /// * The output image must be in `VK_COLOR_ATTACHMENT_OPTIMAL` layout.
    ///
    /// librashader **will not** create a pipeline barrier for the final pass. The output image will
    /// remain in `VK_COLOR_ATTACHMENT_OPTIMAL` after all shader passes. The caller must transition
    /// the output image to the final layout.
    pub unsafe fn frame(
        &mut self,
        input: &VulkanImage,
        viewport: &Viewport<VulkanImage>,
        cmd: vk::CommandBuffer,
        frame_count: usize,
        options: Option<&FrameOptionsVulkan>,
    ) -> error::Result<()> {
        let intermediates =
            &mut self.residuals[self.common.internal_frame_count % self.residuals.len()];
        intermediates.dispose();

        // limit number of passes to those enabled.
        let max = std::cmp::min(self.passes.len(), self.common.config.passes_enabled());
        let passes = &mut self.passes[0..max];

        if let Some(options) = &options {
            if options.clear_history {
                for history in &mut self.history_framebuffers {
                    history.clear(cmd);
                }
            }
        }

        if passes.is_empty() {
            return Ok(());
        }

        let original_image_view = unsafe {
            let create_info = vk::ImageViewCreateInfo::default()
                .image(input.image)
                .format(input.format)
                .view_type(vk::ImageViewType::TYPE_2D)
                .subresource_range(
                    vk::ImageSubresourceRange::default()
                        .aspect_mask(vk::ImageAspectFlags::COLOR)
                        .level_count(1)
                        .layer_count(1),
                )
                .components(
                    vk::ComponentMapping::default()
                        .r(vk::ComponentSwizzle::R)
                        .g(vk::ComponentSwizzle::G)
                        .b(vk::ComponentSwizzle::B)
                        .a(vk::ComponentSwizzle::A),
                );

            self.vulkan.device.create_image_view(&create_info, None)?
        };

        let filter = passes[0].meta.filter;
        let wrap_mode = passes[0].meta.wrap_mode;

        // update history
        for (texture, image) in self
            .common
            .history_textures
            .iter_mut()
            .zip(self.history_framebuffers.iter())
        {
            *texture = Some(image.as_input(filter, wrap_mode));
        }

        let original = InputImage {
            image: input.clone(),
            image_view: original_image_view,
            wrap_mode,
            filter_mode: filter,
            mip_filter: filter,
        };

        let mut source = original.clone();

        // swap output and feedback **before** recording command buffers
        std::mem::swap(
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
        );

        // rescale render buffers to ensure all bindings are valid.
        OwnedImage::scale_framebuffers_with_context(
            source.image.size,
            viewport.output.size,
            original.image.size,
            &mut self.output_framebuffers,
            &mut self.feedback_framebuffers,
            passes,
            &Some(OwnedImageLayout {
                dst_layout: vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                dst_access: vk::AccessFlags::SHADER_READ,
                src_stage: vk::PipelineStageFlags::TOP_OF_PIPE,
                dst_stage: vk::PipelineStageFlags::FRAGMENT_SHADER,
                cmd,
            }),
            Some(&mut |index: usize,
                       pass: &FilterPass,
                       output: &OwnedImage,
                       feedback: &OwnedImage| {
                // refresh inputs
                self.common.feedback_textures[index] =
                    Some(feedback.as_input(pass.meta.filter, pass.meta.wrap_mode));
                self.common.output_textures[index] =
                    Some(output.as_input(pass.meta.filter, pass.meta.wrap_mode));
                Ok(())
            }),
        )?;

        let passes_len = passes.len();
        let (pass, last) = passes.split_at_mut(passes_len - 1);

        let options = options.unwrap_or(&self.default_options);

        self.common
            .draw_quad
            .bind_vbo_for_frame(&self.vulkan.device, cmd);
        for (index, pass) in pass.iter_mut().enumerate() {
            let target = &self.output_framebuffers[index];
            source.filter_mode = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;
            source.mip_filter = pass.meta.filter;

            let output_image = OutputImage::new(&self.vulkan.device, target.image.clone())?;
            let out = RenderTarget::identity(&output_image)?;

            let residual_fb = pass.draw(
                cmd,
                target.image.format,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Offscreen,
                false,
            )?;

            if target.max_miplevels > 1 && !self.disable_mipmaps {
                target.generate_mipmaps_and_end_pass(cmd);
            } else {
                out.output.end_pass(&self.vulkan.device, cmd);
            }

            source = self.common.output_textures[index].clone().unwrap();
            intermediates.dispose_outputs(output_image);
            intermediates.dispose_framebuffers(residual_fb);
        }

        // try to hint the optimizer
        assert_eq!(last.len(), 1);
        if let Some(pass) = last.iter_mut().next() {
            let index = passes_len - 1;
            if pass
                .graphics_pipeline
                .render_passes
                .get(&viewport.output.format)
                .is_none()
            {
                // need to recompile
                pass.graphics_pipeline.recompile(viewport.output.format)?;
            }

            source.filter_mode = pass.meta.filter;
            source.wrap_mode = pass.meta.wrap_mode;
            source.mip_filter = pass.meta.filter;

            if self.draw_last_pass_feedback {
                let target = &self.output_framebuffers[index];

                let output_image = OutputImage::new(&self.vulkan.device, target.image.clone())?;
                let out = RenderTarget::viewport_with_output(&output_image, viewport);

                let residual_fb = pass.draw(
                    cmd,
                    target.image.format,
                    index,
                    &self.common,
                    pass.meta.get_frame_count(frame_count),
                    options,
                    viewport,
                    &original,
                    &source,
                    &out,
                    QuadType::Final,
                    true,
                )?;
                out.output.end_pass(&self.vulkan.device, cmd);
                intermediates.dispose_outputs(output_image);
                intermediates.dispose_framebuffers(residual_fb);
            }

            let output_image = OutputImage::new(&self.vulkan.device, viewport.output.clone())?;
            let out = RenderTarget::viewport_with_output(&output_image, viewport);

            let residual_fb = pass.draw(
                cmd,
                viewport.output.format,
                index,
                &self.common,
                pass.meta.get_frame_count(frame_count),
                options,
                viewport,
                &original,
                &source,
                &out,
                QuadType::Final,
                false,
            )?;

            intermediates.dispose_outputs(output_image);
            intermediates.dispose_framebuffers(residual_fb);
        }

        self.push_history(input, cmd)?;
        self.common.internal_frame_count = self.common.internal_frame_count.wrapping_add(1);
        Ok(())
    }
}
