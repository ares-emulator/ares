use librashader_common::map::FastHashMap;
use librashader_reflect::back::hlsl::CrossHlslContext;
use librashader_reflect::reflect::semantics::{
    BindingMeta, MemberOffset, UniformMemberBlock, UniformMeta,
};
use librashader_runtime::binding::ContextOffset;
use librashader_runtime::uniforms::{BindUniform, UniformScalar, UniformStorage};
use num_traits::AsPrimitive;
use std::fmt::Debug;
use windows::Win32::Graphics::Direct3D9::IDirect3DDevice9;

// only cN (float) or sN (sampler) registers allowed.
#[derive(Debug, Copy, Clone)]
pub enum RegisterSet {
    Float,
    Sampler,
}

#[derive(Debug)]
pub struct ConstantDescriptor {
    pub assignment: RegisterAssignment,
    pub set: RegisterSet,
}

#[derive(Debug, Copy, Clone)]
pub struct RegisterAssignment {
    pub index: u32,
    pub count: u32,
}

#[derive(Debug, Copy, Clone)]
pub struct ConstantRegister {
    pub register: VariableRegister,
    pub _offset: MemberOffset,
}

impl ConstantRegister {
    pub fn reflect_register_assignment(
        meta: &dyn UniformMeta,
        ps_constants: &FastHashMap<String, ConstantDescriptor>,
        vs_constants: &FastHashMap<String, ConstantDescriptor>,
        context: &CrossHlslContext,
    ) -> Self {
        let uniform_name = meta.id();

        // Yeah this is n^2 but ah well.
        let ps = ps_constants
            .iter()
            .find_map(|(mangled_name, register)| {
                if context
                    .fragment_buffers
                    .contains_uniform(uniform_name, mangled_name)
                {
                    Some(register)
                } else {
                    None
                }
            })
            .map(|c| c.assignment);

        let vs = vs_constants
            .iter()
            .find_map(|(mangled_name, register)| {
                if context
                    .fragment_buffers
                    .contains_uniform(uniform_name, mangled_name)
                {
                    Some(register)
                } else {
                    None
                }
            })
            .map(|c| c.assignment);

        ConstantRegister {
            register: VariableRegister { ps, vs },
            _offset: meta.offset(),
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub struct VariableRegister {
    pub(crate) ps: Option<RegisterAssignment>,
    pub(crate) vs: Option<RegisterAssignment>,
}

impl ContextOffset<D3D9UniformBinder, ConstantRegister, IDirect3DDevice9> for ConstantRegister {
    fn offset(&self) -> MemberOffset {
        self._offset
    }

    fn context(&self) -> ConstantRegister {
        *self
    }
}

pub(crate) type D3D9UniformStorage =
    UniformStorage<D3D9UniformBinder, ConstantRegister, Box<[u8]>, Box<[u8]>, IDirect3DDevice9>;

trait D3D9UniformScalar: UniformScalar + AsPrimitive<f32> + Copy {}
impl D3D9UniformScalar for u32 {}
impl D3D9UniformScalar for i32 {}
impl D3D9UniformScalar for f32 {}

pub(crate) struct D3D9UniformBinder;
impl<T> BindUniform<ConstantRegister, T, IDirect3DDevice9> for D3D9UniformBinder
where
    T: D3D9UniformScalar,
{
    fn bind_uniform(
        _block: UniformMemberBlock,
        value: T,
        context: ConstantRegister,
        device: &IDirect3DDevice9,
    ) -> Option<()> {
        let zeroed = T::zeroed().as_();
        let value = [value.as_(), zeroed, zeroed, zeroed];
        let location = &context.register;
        unsafe {
            if let Some(location) = location.vs {
                if let Err(err) =
                    device.SetVertexShaderConstantF(location.index, value.as_ptr(), location.count)
                {
                    println!(
                        "[librashader-runtime-d3d9] unable to bind vertex {}: {err}",
                        location.index
                    );
                }
            }
        }
        unsafe {
            if let Some(location) = location.ps {
                if let Err(err) =
                    device.SetPixelShaderConstantF(location.index, value.as_ptr(), location.count)
                {
                    println!(
                        "[librashader-runtime-d3d9] unable to bind vertex {}: {err}",
                        location.index
                    );
                }
            }
        }
        Some(())
    }
}

impl BindUniform<ConstantRegister, &[f32; 4], IDirect3DDevice9> for D3D9UniformBinder {
    fn bind_uniform(
        _block: UniformMemberBlock,
        vec4: &[f32; 4],
        context: ConstantRegister,
        device: &IDirect3DDevice9,
    ) -> Option<()> {
        let location = &context.register;
        unsafe {
            if let Some(location) = location.vs {
                if let Err(err) =
                    device.SetVertexShaderConstantF(location.index, vec4.as_ptr(), location.count)
                {
                    println!(
                        "[librashader-runtime-d3d9] unable to bind vertex {}: {err}",
                        location.index
                    );
                }
            }
        }
        unsafe {
            if let Some(location) = location.ps {
                if let Err(err) =
                    device.SetPixelShaderConstantF(location.index, vec4.as_ptr(), location.count)
                {
                    println!(
                        "[librashader-runtime-d3d9] unable to bind fragment {}: {err}",
                        location.index
                    );
                }
            }
        }
        Some(())
    }
}

impl BindUniform<ConstantRegister, &[f32; 16], IDirect3DDevice9> for D3D9UniformBinder {
    fn bind_uniform(
        _block: UniformMemberBlock,
        mat4: &[f32; 16],
        context: ConstantRegister,
        device: &IDirect3DDevice9,
    ) -> Option<()> {
        let location = &context.register;
        unsafe {
            if let Some(location) = location.vs {
                if let Err(err) =
                    device.SetVertexShaderConstantF(location.index, mat4.as_ptr(), location.count)
                {
                    println!(
                        "[librashader-runtime-d3d9] unable to bind vertex {}: {err}",
                        location.index
                    );
                }
            }
        }
        unsafe {
            if let Some(location) = location.ps {
                if let Err(err) =
                    device.SetPixelShaderConstantF(location.index, mat4.as_ptr(), location.count)
                {
                    println!(
                        "[librashader-runtime-d3d9] unable to bind fragment {}: {err}",
                        location.index
                    );
                }
            }
        }
        Some(())
    }
}

pub fn update_sampler_bindings(
    meta: &mut BindingMeta,
    ps_constants: &FastHashMap<String, ConstantDescriptor>,
) {
    for (_, binding) in meta.texture_meta.iter_mut() {
        let Some(descriptor) = ps_constants.get(&format!("LIBRA_SAMPLER2D_{}", binding.binding))
        else {
            continue;
        };

        // eprintln!(
        //     "updating binding {} to {}",
        //     binding.binding, descriptor.assignment.index
        // );
        binding.binding = descriptor.assignment.index;
    }
}
