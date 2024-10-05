use librashader_reflect::reflect::semantics::{MemberOffset, UniformMemberBlock};
use std::marker::PhantomData;
use std::ops::{Deref, DerefMut};

/// A scalar value that is valid as a uniform member
pub trait UniformScalar: Copy + bytemuck::Pod {}
impl UniformScalar for f32 {}
impl UniformScalar for i32 {}
impl UniformScalar for u32 {}

/// A trait for a binder that binds the given value and context into the uniform for a shader pass.
pub trait BindUniform<C, T, D> {
    /// Bind the given value to the shader uniforms given the input context.
    ///
    /// A `BindUniform` implementation should not write to a backing buffer from a [`UniformStorage`].
    /// If the binding is successful and no writes to a backing buffer is necessary, this function should return `Some(())`.
    /// If this function returns `None`, then the value will instead be written to the backing buffer.
    fn bind_uniform(block: UniformMemberBlock, value: T, ctx: C, device: &D) -> Option<()>;
}

/// A trait to access the raw pointer to a backing uniform storage.
pub trait UniformStorageAccess {
    /// Get a pointer to the backing UBO storage. This pointer must be valid for the lifetime
    /// of the implementing struct.
    fn ubo_pointer(&self) -> *const u8;

    /// Get a pointer to the backing UBO storage. This pointer must be valid for the lifetime
    /// of the implementing struct.
    fn ubo_slice(&self) -> &[u8];

    /// Get a pointer to the backing Push Constant buffer storage.
    /// This pointer must be valid for the lifetime of the implementing struct.
    fn push_pointer(&self) -> *const u8;

    /// Get a slice to the backing Push Constant buffer storage.
    /// This pointer must be valid for the lifetime of the implementing struct.
    fn push_slice(&self) -> &[u8];
}

impl<D, T, H, U, P> UniformStorageAccess for UniformStorage<T, H, U, P, D>
where
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
{
    fn ubo_pointer(&self) -> *const u8 {
        self.ubo.as_ptr()
    }

    fn ubo_slice(&self) -> &[u8] {
        &self.ubo
    }

    fn push_pointer(&self) -> *const u8 {
        self.push.as_ptr()
    }

    fn push_slice(&self) -> &[u8] {
        &self.push
    }
}

/// A uniform binder that always returns `None`, and does not do any binding of uniforms.
/// All uniform data is thus written into the backing buffer storage.
pub struct NoUniformBinder;
impl<T, D> BindUniform<Option<()>, T, D> for NoUniformBinder {
    fn bind_uniform(_: UniformMemberBlock, _: T, _: Option<()>, _: &D) -> Option<()> {
        None
    }
}

/// A helper to bind uniform variables to UBO or Push Constant Buffers.
pub struct UniformStorage<H = NoUniformBinder, C = Option<()>, U = Box<[u8]>, P = Box<[u8]>, D = ()>
where
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
{
    ubo: U,
    push: P,
    _h: PhantomData<H>,
    _c: PhantomData<C>,
    _d: PhantomData<D>,
}

impl<H, C, U, P, D> UniformStorage<H, C, U, P, D>
where
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
{
    /// Access the backing storage for the UBO.
    pub fn inner_ubo(&self) -> &U {
        &self.ubo
    }

    /// Access the backing storage for the Push storage.
    pub fn inner_push(&self) -> &P {
        &self.push
    }

    pub(crate) fn buffer(&mut self, ty: UniformMemberBlock) -> &mut [u8] {
        match ty {
            UniformMemberBlock::Ubo => self.ubo.deref_mut(),
            UniformMemberBlock::PushConstant => self.push.deref_mut(),
        }
    }
}

impl<H, C, U, P, D> UniformStorage<H, C, U, P, D>
where
    C: Copy,
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
{
    #[inline(always)]
    fn write_scalar_inner<T: UniformScalar>(buffer: &mut [u8], value: T) {
        let buffer = bytemuck::cast_slice_mut(buffer);
        buffer[0] = value;
    }

    /// Bind a scalar to the given offset.
    #[inline(always)]
    pub fn bind_scalar<T: UniformScalar>(
        &mut self,
        offset: MemberOffset,
        value: T,
        ctx: C,
        device: &D,
    ) where
        H: BindUniform<C, T, D>,
    {
        for ty in UniformMemberBlock::TYPES {
            if H::bind_uniform(ty, value, ctx, device).is_some() {
                continue;
            }

            if let Some(offset) = offset.offset(ty) {
                let buffer = self.buffer(ty);
                Self::write_scalar_inner(&mut buffer[offset..][..std::mem::size_of::<T>()], value)
            }
        }
    }

    /// Create a new `UniformStorage` with the given backing storage
    pub fn new_with_storage(ubo: U, push: P) -> UniformStorage<H, C, U, P, D> {
        UniformStorage {
            ubo,
            push,
            _h: Default::default(),
            _c: Default::default(),
            _d: Default::default(),
        }
    }
}

impl<H, C, U, D> UniformStorage<H, C, U, Box<[u8]>, D>
where
    C: Copy,
    U: Deref<Target = [u8]> + DerefMut,
{
    /// Create a new `UniformStorage` with the given backing storage
    pub fn new_with_ubo_storage(
        storage: U,
        push_size: usize,
    ) -> UniformStorage<H, C, U, Box<[u8]>, D> {
        UniformStorage {
            ubo: storage,
            push: vec![0u8; push_size].into_boxed_slice(),
            _h: Default::default(),
            _c: Default::default(),
            _d: Default::default(),
        }
    }
}

impl<H, C, D> UniformStorage<H, C, Box<[u8]>, Box<[u8]>, D> {
    /// Create a new `UniformStorage` with the given size for UBO and Push Constant Buffer sizes.
    pub fn new(ubo_size: usize, push_size: usize) -> UniformStorage<H, C, Box<[u8]>, Box<[u8]>, D> {
        UniformStorage {
            ubo: vec![0u8; ubo_size].into_boxed_slice(),
            push: vec![0u8; push_size].into_boxed_slice(),
            _h: Default::default(),
            _c: Default::default(),
            _d: Default::default(),
        }
    }
}

impl<H, C, U, P, D> UniformStorage<H, C, U, P, D>
where
    C: Copy,
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
    H: for<'a> BindUniform<C, &'a [f32; 4], D>,
{
    #[inline(always)]
    fn write_vec4_inner(buffer: &mut [u8], vec4: &[f32; 4]) {
        let vec4 = bytemuck::cast_slice(vec4);
        buffer.copy_from_slice(vec4);
    }
    /// Bind a `vec4` to the given offset.
    #[inline(always)]
    pub fn bind_vec4(
        &mut self,
        offset: MemberOffset,
        value: impl Into<[f32; 4]>,
        ctx: C,
        device: &D,
    ) {
        let vec4 = value.into();

        for ty in UniformMemberBlock::TYPES {
            if H::bind_uniform(ty, &vec4, ctx, device).is_some() {
                continue;
            }
            if let Some(offset) = offset.offset(ty) {
                let buffer = self.buffer(ty);
                Self::write_vec4_inner(
                    &mut buffer[offset..][..4 * std::mem::size_of::<f32>()],
                    &vec4,
                );
            }
        }
    }
}

impl<H, C, U, P, D> UniformStorage<H, C, U, P, D>
where
    C: Copy,
    U: Deref<Target = [u8]> + DerefMut,
    P: Deref<Target = [u8]> + DerefMut,
    H: for<'a> BindUniform<C, &'a [f32; 16], D>,
{
    #[inline(always)]
    fn write_mat4_inner(buffer: &mut [u8], mat4: &[f32; 16]) {
        let mat4 = bytemuck::cast_slice(mat4);
        buffer.copy_from_slice(mat4);
    }

    /// Bind a `mat4` to the given offset.
    #[inline(always)]
    pub fn bind_mat4(&mut self, offset: MemberOffset, value: &[f32; 16], ctx: C, device: &D) {
        for ty in UniformMemberBlock::TYPES {
            if H::bind_uniform(ty, value, ctx, device).is_some() {
                continue;
            }
            if let Some(offset) = offset.offset(ty) {
                let buffer = self.buffer(ty);
                Self::write_mat4_inner(
                    &mut buffer[offset..][..16 * std::mem::size_of::<f32>()],
                    value,
                );
            }
        }
    }
}
