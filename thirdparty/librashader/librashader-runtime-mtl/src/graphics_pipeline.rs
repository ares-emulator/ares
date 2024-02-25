use crate::error::{FilterChainError, Result};
use crate::select_optimal_pixel_format;
use bytemuck::offset_of;
use icrate::Foundation::NSString;
use icrate::Metal::{
    MTLBlendFactorOneMinusSourceAlpha, MTLBlendFactorSourceAlpha, MTLCommandBuffer,
    MTLCommandEncoder, MTLDevice, MTLFunction, MTLLibrary, MTLLoadActionDontCare, MTLPixelFormat,
    MTLPrimitiveTopologyClassTriangle, MTLRenderCommandEncoder, MTLRenderPassDescriptor,
    MTLRenderPipelineColorAttachmentDescriptor, MTLRenderPipelineDescriptor,
    MTLRenderPipelineState, MTLScissorRect, MTLStoreActionStore, MTLTexture,
    MTLVertexAttributeDescriptor, MTLVertexBufferLayoutDescriptor, MTLVertexDescriptor,
    MTLVertexFormatFloat2, MTLVertexFormatFloat4, MTLVertexStepFunctionPerVertex, MTLViewport,
};
use librashader_reflect::back::msl::{CrossMslContext, NagaMslContext};
use librashader_reflect::back::ShaderCompilerOutput;
use librashader_runtime::quad::VertexInput;
use librashader_runtime::render_target::RenderTarget;

use objc2::rc::Id;
use objc2::runtime::ProtocolObject;

/// This is only really plausible for SPIRV-Cross, for Naga we need to supply the next plausible binding.
pub const VERTEX_BUFFER_INDEX: usize = 4;

pub struct MetalGraphicsPipeline {
    pub layout: PipelineLayoutObjects,
    render_pipeline: Id<ProtocolObject<dyn MTLRenderPipelineState>>,
    pub render_pass_format: MTLPixelFormat,
}

pub struct PipelineLayoutObjects {
    _vertex_lib: Id<ProtocolObject<dyn MTLLibrary>>,
    _fragment_lib: Id<ProtocolObject<dyn MTLLibrary>>,
    vertex_entry: Id<ProtocolObject<dyn MTLFunction>>,
    fragment_entry: Id<ProtocolObject<dyn MTLFunction>>,
}

pub(crate) trait MslEntryPoint {
    fn entry_point() -> Id<NSString>;
}

impl MslEntryPoint for CrossMslContext {
    fn entry_point() -> Id<NSString> {
        NSString::from_str("main0")
    }
}

impl MslEntryPoint for NagaMslContext {
    fn entry_point() -> Id<NSString> {
        NSString::from_str("main_")
    }
}

impl PipelineLayoutObjects {
    pub fn new<T: MslEntryPoint>(
        shader_assembly: &ShaderCompilerOutput<String, T>,
        device: &ProtocolObject<dyn MTLDevice>,
    ) -> Result<Self> {
        let entry = T::entry_point();

        let vertex = NSString::from_str(&shader_assembly.vertex);
        let vertex = device.newLibraryWithSource_options_error(&vertex, None)?;
        let vertex_entry = vertex
            .newFunctionWithName(&entry)
            .ok_or(FilterChainError::ShaderWrongEntryName)?;

        let fragment = NSString::from_str(&shader_assembly.fragment);
        let fragment = device.newLibraryWithSource_options_error(&fragment, None)?;
        let fragment_entry = fragment
            .newFunctionWithName(&entry)
            .ok_or(FilterChainError::ShaderWrongEntryName)?;

        Ok(Self {
            _vertex_lib: vertex,
            _fragment_lib: fragment,
            vertex_entry,
            fragment_entry,
        })
    }

    unsafe fn create_vertex_descriptor() -> Id<MTLVertexDescriptor> {
        let descriptor = MTLVertexDescriptor::new();
        let attributes = descriptor.attributes();
        let layouts = descriptor.layouts();

        let binding = MTLVertexBufferLayoutDescriptor::new();

        let position = MTLVertexAttributeDescriptor::new();
        let texcoord = MTLVertexAttributeDescriptor::new();

        // hopefully metal fills in vertices otherwise we'll need to use the vec4 stuff.
        position.setFormat(MTLVertexFormatFloat4);
        position.setBufferIndex(VERTEX_BUFFER_INDEX);
        position.setOffset(offset_of!(VertexInput, position));

        texcoord.setFormat(MTLVertexFormatFloat2);
        texcoord.setBufferIndex(VERTEX_BUFFER_INDEX);
        texcoord.setOffset(offset_of!(VertexInput, texcoord));

        attributes.setObject_atIndexedSubscript(Some(&position), 0);

        attributes.setObject_atIndexedSubscript(Some(&texcoord), 1);

        binding.setStepFunction(MTLVertexStepFunctionPerVertex);
        binding.setStride(std::mem::size_of::<VertexInput>());
        layouts.setObject_atIndexedSubscript(Some(&binding), VERTEX_BUFFER_INDEX);

        descriptor
    }

    unsafe fn create_color_attachments(
        ca: Id<MTLRenderPipelineColorAttachmentDescriptor>,
        format: MTLPixelFormat,
    ) -> Id<MTLRenderPipelineColorAttachmentDescriptor> {
        ca.setPixelFormat(select_optimal_pixel_format(format));
        ca.setBlendingEnabled(false);
        ca.setSourceAlphaBlendFactor(MTLBlendFactorSourceAlpha);
        ca.setSourceRGBBlendFactor(MTLBlendFactorSourceAlpha);
        ca.setDestinationAlphaBlendFactor(MTLBlendFactorOneMinusSourceAlpha);
        ca.setDestinationRGBBlendFactor(MTLBlendFactorOneMinusSourceAlpha);

        ca
    }

    pub fn create_pipeline(
        &self,
        device: &ProtocolObject<dyn MTLDevice>,
        format: MTLPixelFormat,
    ) -> Result<Id<ProtocolObject<dyn MTLRenderPipelineState>>> {
        let descriptor = MTLRenderPipelineDescriptor::new();

        unsafe {
            let vertex = Self::create_vertex_descriptor();
            descriptor.setInputPrimitiveTopology(MTLPrimitiveTopologyClassTriangle);
            descriptor.setVertexDescriptor(Some(&vertex));

            let ca = descriptor.colorAttachments().objectAtIndexedSubscript(0);
            Self::create_color_attachments(ca, format);

            descriptor.setRasterSampleCount(1);

            descriptor.setVertexFunction(Some(&self.vertex_entry));
            descriptor.setFragmentFunction(Some(&self.fragment_entry));
        }

        Ok(device.newRenderPipelineStateWithDescriptor_error(descriptor.as_ref())?)
    }
}

impl MetalGraphicsPipeline {
    pub fn new<T: MslEntryPoint>(
        device: &ProtocolObject<dyn MTLDevice>,
        shader_assembly: &ShaderCompilerOutput<String, T>,
        render_pass_format: MTLPixelFormat,
    ) -> Result<Self> {
        let layout = PipelineLayoutObjects::new(shader_assembly, device)?;
        let pipeline = layout.create_pipeline(device, render_pass_format)?;
        Ok(Self {
            layout,
            render_pipeline: pipeline,
            render_pass_format,
        })
    }

    pub fn recompile(
        &mut self,
        device: &ProtocolObject<dyn MTLDevice>,
        format: MTLPixelFormat,
    ) -> Result<()> {
        let render_pipeline = self.layout.create_pipeline(device, format)?;
        self.render_pipeline = render_pipeline;
        Ok(())
    }

    pub fn begin_rendering<'pass>(
        &self,
        output: &RenderTarget<ProtocolObject<dyn MTLTexture>>,
        buffer: &ProtocolObject<dyn MTLCommandBuffer>,
    ) -> Result<Id<ProtocolObject<dyn MTLRenderCommandEncoder>>> {
        unsafe {
            let descriptor = MTLRenderPassDescriptor::new();
            let ca = descriptor.colorAttachments().objectAtIndexedSubscript(0);
            ca.setLoadAction(MTLLoadActionDontCare);
            ca.setStoreAction(MTLStoreActionStore);
            ca.setTexture(Some(output.output));

            let rpass = buffer
                .renderCommandEncoderWithDescriptor(&descriptor)
                .ok_or(FilterChainError::FailedToCreateRenderPass)?;

            rpass.setLabel(Some(&*NSString::from_str("librashader rpass")));
            rpass.setRenderPipelineState(&self.render_pipeline);

            rpass.setScissorRect(MTLScissorRect {
                x: output.x as usize,
                y: output.y as usize,
                width: output.output.width(),
                height: output.output.height(),
            });

            rpass.setViewport(MTLViewport {
                originX: output.x as f64,
                originY: output.y as f64,
                width: output.output.width() as f64,
                height: output.output.height() as f64,
                znear: 0.0,
                zfar: 1.0,
            });

            Ok(rpass)
        }
    }
}
