use gl::types::GLint;
use librashader_reflect::reflect::semantics::{BindingStage, UniformMemberBlock};
use librashader_runtime::uniforms::{BindUniform, UniformScalar, UniformStorage};

#[derive(Debug, Copy, Clone)]
pub struct VariableLocation {
    pub(crate) ubo: Option<UniformLocation<GLint>>,
    pub(crate) push: Option<UniformLocation<GLint>>,
}

impl VariableLocation {
    pub fn location(&self, offset_type: UniformMemberBlock) -> Option<UniformLocation<GLint>> {
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

impl UniformLocation<GLint> {
    pub fn is_valid(&self, stage: BindingStage) -> bool {
        let mut validity = false;
        if stage.contains(BindingStage::FRAGMENT) {
            validity = validity || self.fragment >= 0;
        }
        if stage.contains(BindingStage::VERTEX) {
            validity = validity || self.vertex >= 0;
        }
        validity
    }

    pub fn bindable(&self) -> bool {
        self.is_valid(BindingStage::VERTEX | BindingStage::FRAGMENT)
    }
}

pub(crate) type GlUniformStorage = UniformStorage<GlUniformBinder, VariableLocation>;

pub trait GlUniformScalar: UniformScalar {
    const FACTORY: unsafe fn(GLint, Self) -> ();
}

impl GlUniformScalar for f32 {
    const FACTORY: unsafe fn(GLint, Self) -> () = gl::Uniform1f;
}

impl GlUniformScalar for i32 {
    const FACTORY: unsafe fn(GLint, Self) -> () = gl::Uniform1i;
}

impl GlUniformScalar for u32 {
    const FACTORY: unsafe fn(GLint, Self) -> () = gl::Uniform1ui;
}

pub(crate) struct GlUniformBinder;
impl<T> BindUniform<VariableLocation, T, ()> for GlUniformBinder
where
    T: GlUniformScalar,
{
    fn bind_uniform(
        block: UniformMemberBlock,
        value: T,
        location: VariableLocation,
        _: &(),
    ) -> Option<()> {
        if let Some(location) = location.location(block)
            && location.bindable()
        {
            if location.is_valid(BindingStage::VERTEX) {
                unsafe {
                    T::FACTORY(location.vertex, value);
                }
            }
            if location.is_valid(BindingStage::FRAGMENT) {
                unsafe {
                    T::FACTORY(location.fragment, value);
                }
            }
            Some(())
        } else {
            None
        }
    }
}

impl BindUniform<VariableLocation, &[f32; 4], ()> for GlUniformBinder {
    fn bind_uniform(
        block: UniformMemberBlock,
        vec4: &[f32; 4],
        location: VariableLocation,
        _: &(),
    ) -> Option<()> {
        if let Some(location) = location.location(block)
            && location.bindable()
        {
            unsafe {
                if location.is_valid(BindingStage::VERTEX) {
                    gl::Uniform4fv(location.vertex, 1, vec4.as_ptr());
                }
                if location.is_valid(BindingStage::FRAGMENT) {
                    gl::Uniform4fv(location.fragment, 1, vec4.as_ptr());
                }
            }
            Some(())
        } else {
            None
        }
    }
}

impl BindUniform<VariableLocation, &[f32; 16], ()> for GlUniformBinder {
    fn bind_uniform(
        block: UniformMemberBlock,
        mat4: &[f32; 16],
        location: VariableLocation,
        _: &(),
    ) -> Option<()> {
        if let Some(location) = location.location(block)
            && location.bindable()
        {
            unsafe {
                if location.is_valid(BindingStage::VERTEX) {
                    gl::UniformMatrix4fv(location.vertex, 1, gl::FALSE, mat4.as_ptr());
                }
                if location.is_valid(BindingStage::FRAGMENT) {
                    gl::UniformMatrix4fv(location.fragment, 1, gl::FALSE, mat4.as_ptr());
                }
            }
            Some(())
        } else {
            None
        }
    }
}
