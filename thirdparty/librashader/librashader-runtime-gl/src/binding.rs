use glow::HasContext;
use librashader_reflect::reflect::semantics::{BindingStage, UniformMemberBlock};
use librashader_runtime::uniforms::{BindUniform, UniformScalar, UniformStorage};
use std::fmt::Display;

#[derive(Debug, Copy, Clone)]
pub struct VariableLocation {
    pub(crate) ubo: Option<UniformLocation<Option<glow::UniformLocation>>>,
    pub(crate) push: Option<UniformLocation<Option<glow::UniformLocation>>>,
}

impl VariableLocation {
    pub fn location(
        &self,
        offset_type: UniformMemberBlock,
    ) -> Option<UniformLocation<Option<glow::UniformLocation>>> {
        match offset_type {
            UniformMemberBlock::Ubo => self.ubo,
            UniformMemberBlock::PushConstant => self.push,
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub struct UniformLocation<T> {
    pub vertex: T,
    pub fragment: T,
}

impl UniformLocation<Option<glow::UniformLocation>> {
    #[allow(unused_comparisons)]
    pub fn is_valid(&self, stage: BindingStage) -> bool {
        // since glow::UniformLocation is None or NonZeroU32,
        // is_some is sufficient for validity
        (stage.contains(BindingStage::FRAGMENT) && self.fragment.is_some())
            || (stage.contains(BindingStage::VERTEX) && self.vertex.is_some())
    }

    pub fn bindable(&self) -> bool {
        self.is_valid(BindingStage::VERTEX | BindingStage::FRAGMENT)
    }
}

pub(crate) type GlUniformStorage =
    UniformStorage<GlUniformBinder, VariableLocation, Box<[u8]>, Box<[u8]>, glow::Context>;

pub trait GlUniformScalar: UniformScalar + Display {
    const FACTORY: unsafe fn(&glow::Context, Option<&glow::UniformLocation>, Self) -> ();
}

impl GlUniformScalar for f32 {
    const FACTORY: unsafe fn(&glow::Context, Option<&glow::UniformLocation>, Self) -> () =
        glow::Context::uniform_1_f32;
}

impl GlUniformScalar for i32 {
    const FACTORY: unsafe fn(&glow::Context, Option<&glow::UniformLocation>, Self) -> () =
        glow::Context::uniform_1_i32;
}

impl GlUniformScalar for u32 {
    const FACTORY: unsafe fn(&glow::Context, Option<&glow::UniformLocation>, Self) -> () =
        glow::Context::uniform_1_u32;
}

pub(crate) struct GlUniformBinder;
impl<T> BindUniform<VariableLocation, T, glow::Context> for GlUniformBinder
where
    T: GlUniformScalar,
{
    fn bind_uniform(
        block: UniformMemberBlock,
        value: T,
        location: VariableLocation,
        device: &glow::Context,
    ) -> Option<()> {
        if let Some(location) = location
            .location(block)
            .filter(|location| location.bindable())
        {
            if location.is_valid(BindingStage::VERTEX) {
                unsafe {
                    T::FACTORY(device, location.vertex.as_ref(), value);
                }
            }
            if location.is_valid(BindingStage::FRAGMENT) {
                unsafe {
                    T::FACTORY(device, location.fragment.as_ref(), value);
                }
            }
            Some(())
        } else {
            None
        }
    }
}

impl BindUniform<VariableLocation, &[f32; 4], glow::Context> for GlUniformBinder {
    fn bind_uniform(
        block: UniformMemberBlock,
        vec4: &[f32; 4],
        location: VariableLocation,
        device: &glow::Context,
    ) -> Option<()> {
        if let Some(location) = location
            .location(block)
            .filter(|location| location.bindable())
        {
            unsafe {
                if location.is_valid(BindingStage::VERTEX) {
                    device.uniform_4_f32_slice(location.vertex.as_ref(), vec4);
                }
                if location.is_valid(BindingStage::FRAGMENT) {
                    device.uniform_4_f32_slice(location.fragment.as_ref(), vec4);
                }
            }
            Some(())
        } else {
            None
        }
    }
}

impl BindUniform<VariableLocation, &[f32; 16], glow::Context> for GlUniformBinder {
    fn bind_uniform(
        block: UniformMemberBlock,
        mat4: &[f32; 16],
        location: VariableLocation,
        device: &glow::Context,
    ) -> Option<()> {
        if let Some(location) = location
            .location(block)
            .filter(|location| location.bindable())
        {
            unsafe {
                if location.is_valid(BindingStage::VERTEX) {
                    device.uniform_matrix_4_f32_slice(location.vertex.as_ref(), false, mat4);
                }
                if location.is_valid(BindingStage::FRAGMENT) {
                    device.uniform_matrix_4_f32_slice(location.fragment.as_ref(), false, mat4);
                }
            }
            Some(())
        } else {
            None
        }
    }
}
