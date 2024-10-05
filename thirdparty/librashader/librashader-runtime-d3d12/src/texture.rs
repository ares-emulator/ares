use crate::descriptor_heap::{CpuStagingHeap, RenderTargetHeap};
use crate::error::FilterChainError;
use crate::resource::{OutlivesFrame, ResourceHandleStrategy};
use crate::{error, FilterChainD3D12};
use d3d12_descriptor_heap::{D3D12DescriptorHeap, D3D12DescriptorHeapSlot};
use librashader_common::{FilterMode, GetSize, Size, WrapMode};
use std::mem::ManuallyDrop;
use std::ops::Deref;
use windows::core::InterfaceRef;
use windows::Win32::Graphics::Direct3D12::{
    ID3D12Device, ID3D12Resource, D3D12_CPU_DESCRIPTOR_HANDLE,
    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, D3D12_RENDER_TARGET_VIEW_DESC,
    D3D12_RENDER_TARGET_VIEW_DESC_0, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    D3D12_RTV_DIMENSION_TEXTURE2D, D3D12_SHADER_RESOURCE_VIEW_DESC,
    D3D12_SHADER_RESOURCE_VIEW_DESC_0, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_TEX2D_RTV,
    D3D12_TEX2D_SRV,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT;

/// An image for use as shader resource view.
#[derive(Clone)]
pub enum D3D12InputImage {
    /// The filter chain manages the CPU descriptor to the shader resource view.
    Managed(ManuallyDrop<ID3D12Resource>),
    /// The CPU descriptor to the shader resource view is managed externally.
    External {
        /// The ID3D12Resource that holds the image data.
        resource: ManuallyDrop<ID3D12Resource>,
        /// The CPU descriptor to the shader resource view.
        descriptor: D3D12_CPU_DESCRIPTOR_HANDLE,
    },
}

impl<'a> From<InterfaceRef<'a, ID3D12Resource>> for D3D12InputImage {
    fn from(value: InterfaceRef<'a, ID3D12Resource>) -> Self {
        Self::Managed(unsafe { std::mem::transmute(value) })
    }
}

#[derive(Clone)]
pub(crate) enum InputDescriptor {
    Owned(D3D12DescriptorHeapSlot<CpuStagingHeap>),
    Raw(D3D12_CPU_DESCRIPTOR_HANDLE),
}

#[derive(Clone)]
pub(crate) enum OutputDescriptor {
    Owned(D3D12DescriptorHeapSlot<RenderTargetHeap>),
    Raw(D3D12_CPU_DESCRIPTOR_HANDLE),
}

impl AsRef<D3D12_CPU_DESCRIPTOR_HANDLE> for InputDescriptor {
    fn as_ref(&self) -> &D3D12_CPU_DESCRIPTOR_HANDLE {
        match self {
            InputDescriptor::Owned(h) => h.as_ref(),
            InputDescriptor::Raw(h) => h,
        }
    }
}

impl AsRef<D3D12_CPU_DESCRIPTOR_HANDLE> for OutputDescriptor {
    fn as_ref(&self) -> &D3D12_CPU_DESCRIPTOR_HANDLE {
        match self {
            OutputDescriptor::Owned(h) => h.as_ref(),
            OutputDescriptor::Raw(h) => h,
        }
    }
}

/// An image view for use as a render target.
///
/// Can be created from a CPU descriptor handle, and a size.
#[derive(Clone)]
pub struct D3D12OutputView {
    pub(crate) descriptor: OutputDescriptor,
    pub(crate) size: Size<u32>,
    pub(crate) format: DXGI_FORMAT,
}

impl D3D12OutputView {
    pub(crate) fn new(
        handle: D3D12DescriptorHeapSlot<RenderTargetHeap>,
        size: Size<u32>,
        format: DXGI_FORMAT,
    ) -> D3D12OutputView {
        let descriptor = OutputDescriptor::Owned(handle);
        D3D12OutputView {
            descriptor,
            size,
            format,
        }
    }

    // unsafe since the lifetime of the handle has to survive
    /// Create a new D3D12OutputView from a CPU descriptor handle of a render target view.
    ///
    /// SAFETY: the handle must be valid until the command list is submitted.
    pub unsafe fn new_from_raw(
        handle: D3D12_CPU_DESCRIPTOR_HANDLE,
        size: Size<u32>,
        format: DXGI_FORMAT,
    ) -> D3D12OutputView {
        let descriptor = OutputDescriptor::Raw(handle);
        D3D12OutputView {
            descriptor,
            size,
            format,
        }
    }

    /// Create a new output view from a resource ref, linked to the chain.
    ///
    /// The output view will be automatically disposed on drop.
    ///
    /// SAFETY: the image must be valid until the command list is submitted.
    pub unsafe fn new_from_resource(
        image: ManuallyDrop<ID3D12Resource>,
        chain: &mut FilterChainD3D12,
    ) -> error::Result<D3D12OutputView> {
        unsafe {
            Self::new_from_resource_internal(
                std::mem::transmute(image),
                &chain.common.d3d12,
                &mut chain.rtv_heap,
            )
        }
    }

    /// Create a new output view from a resource ref
    pub(crate) unsafe fn new_from_resource_internal(
        image: InterfaceRef<ID3D12Resource>,
        device: &ID3D12Device,
        heap: &mut D3D12DescriptorHeap<RenderTargetHeap>,
    ) -> error::Result<D3D12OutputView> {
        let desc = unsafe { image.GetDesc() };
        if desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D {
            return Err(crate::error::FilterChainError::InvalidDimensionError(
                desc.Dimension,
            ));
        }

        let slot = heap.allocate_descriptor()?;
        unsafe {
            let rtv_desc = D3D12_RENDER_TARGET_VIEW_DESC {
                Format: desc.Format,
                ViewDimension: D3D12_RTV_DIMENSION_TEXTURE2D,
                Anonymous: D3D12_RENDER_TARGET_VIEW_DESC_0 {
                    Texture2D: D3D12_TEX2D_RTV {
                        MipSlice: 0,
                        ..Default::default()
                    },
                },
            };

            device.CreateRenderTargetView(image.deref(), Some(&rtv_desc), *slot.as_ref());
        }

        Ok(Self::new(
            slot,
            Size::new(desc.Width as u32, desc.Height),
            desc.Format,
        ))
    }
}

pub(crate) struct InputTexture {
    pub(crate) resource: ManuallyDrop<ID3D12Resource>,
    pub(crate) descriptor: InputDescriptor,
    pub(crate) size: Size<u32>,
    pub(crate) format: DXGI_FORMAT,
    pub(crate) wrap_mode: WrapMode,
    pub(crate) filter: FilterMode,
}

impl InputTexture {
    // Create a new input texture, with runtime lifetime tracking.
    // The source owned framebuffer must outlive this input.
    pub fn new_owned(
        resource: &ManuallyDrop<ID3D12Resource>,
        handle: D3D12DescriptorHeapSlot<CpuStagingHeap>,
        size: Size<u32>,
        format: DXGI_FORMAT,
        filter: FilterMode,
        wrap_mode: WrapMode,
    ) -> InputTexture {
        let srv = InputDescriptor::Owned(handle);
        InputTexture {
            // SAFETY: `new` is only used for owned textures. We know this because
            // we also hold `handle`, so the texture must be valid for at least
            // as valid for the lifetime of handle.
            // Also, resource is non-null by construction.
            // Option<T> and <T> have the same layout.
            resource: unsafe { std::mem::transmute(OutlivesFrame::obtain(resource)) },
            descriptor: srv,
            size,
            format,
            wrap_mode,
            filter,
        }
    }

    // unsafe since the lifetime of the handle has to survive
    pub unsafe fn new_from_resource(
        image: ManuallyDrop<ID3D12Resource>,
        filter: FilterMode,
        wrap_mode: WrapMode,
        device: &ID3D12Device,
        heap: &mut D3D12DescriptorHeap<CpuStagingHeap>,
    ) -> error::Result<InputTexture> {
        let desc = unsafe { image.GetDesc() };
        if desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D {
            return Err(FilterChainError::InvalidDimensionError(desc.Dimension));
        }

        let descriptor = {
            let slot = heap.allocate_descriptor()?;
            unsafe {
                let srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC {
                    Format: desc.Format,
                    ViewDimension: D3D12_SRV_DIMENSION_TEXTURE2D,
                    Shader4ComponentMapping: D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    Anonymous: D3D12_SHADER_RESOURCE_VIEW_DESC_0 {
                        Texture2D: D3D12_TEX2D_SRV {
                            MipLevels: desc.MipLevels as u32,
                            ..Default::default()
                        },
                    },
                };
                device.CreateShaderResourceView(image.deref(), Some(&srv_desc), *slot.as_ref());
            }

            Ok::<_, FilterChainError>(InputDescriptor::Owned(slot))
        }?;

        Ok(InputTexture {
            resource: image,
            descriptor,
            size: Size::new(desc.Width as u32, desc.Height),
            format: desc.Format,
            wrap_mode,
            filter,
        })
    }

    // unsafe since the lifetime of the handle has to survive
    pub unsafe fn new_from_raw(
        image: ManuallyDrop<ID3D12Resource>,
        descriptor: D3D12_CPU_DESCRIPTOR_HANDLE,
        filter: FilterMode,
        wrap_mode: WrapMode,
    ) -> InputTexture {
        let desc = unsafe { image.GetDesc() };
        InputTexture {
            resource: unsafe { std::mem::transmute(image) },
            descriptor: InputDescriptor::Raw(descriptor),
            size: Size::new(desc.Width as u32, desc.Height),
            format: desc.Format,
            wrap_mode,
            filter,
        }
    }
}

impl Clone for InputTexture {
    fn clone(&self) -> Self {
        // SAFETY: the parent doesn't have drop flag, so that means
        // we don't need to handle drop.
        InputTexture {
            resource: unsafe { std::mem::transmute_copy(&self.resource) },
            descriptor: self.descriptor.clone(),
            size: self.size,
            format: self.format,
            wrap_mode: self.wrap_mode,
            filter: self.filter,
        }
    }
}

impl AsRef<InputTexture> for InputTexture {
    fn as_ref(&self) -> &InputTexture {
        self
    }
}

impl GetSize<u32> for D3D12OutputView {
    type Error = std::convert::Infallible;

    fn size(&self) -> Result<Size<u32>, Self::Error> {
        Ok(self.size)
    }
}
