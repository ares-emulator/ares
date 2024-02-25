use crate::descriptor_heap::{D3D12DescriptorHeap, D3D12DescriptorHeapSlot, ResourceWorkHeap};
use crate::util::dxc_validate_shader;
use crate::{error, util};
use bytemuck::{Pod, Zeroable};
use librashader_common::Size;
use librashader_runtime::scaling::MipmapSize;
use std::mem::ManuallyDrop;
use std::ops::Deref;
use windows::Win32::Graphics::Direct3D::Dxc::{
    CLSID_DxcLibrary, CLSID_DxcValidator, DxcCreateInstance,
};
use windows::Win32::Graphics::Direct3D12::{
    ID3D12DescriptorHeap, ID3D12Device, ID3D12GraphicsCommandList, ID3D12PipelineState,
    ID3D12Resource, ID3D12RootSignature, D3D12_COMPUTE_PIPELINE_STATE_DESC,
    D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, D3D12_RESOURCE_BARRIER, D3D12_RESOURCE_BARRIER_0,
    D3D12_RESOURCE_BARRIER_TYPE_UAV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_UAV_BARRIER, D3D12_SHADER_BYTECODE,
    D3D12_SHADER_RESOURCE_VIEW_DESC, D3D12_SHADER_RESOURCE_VIEW_DESC_0,
    D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_TEX2D_SRV, D3D12_TEX2D_UAV, D3D12_UAV_DIMENSION_TEXTURE2D,
    D3D12_UNORDERED_ACCESS_VIEW_DESC, D3D12_UNORDERED_ACCESS_VIEW_DESC_0,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_FORMAT;

const GENERATE_MIPMAPS_CS: &[u8] = include_bytes!("../shader/mipmap.dxil");

pub struct D3D12MipmapGen {
    device: ID3D12Device,
    root_signature: ID3D12RootSignature,
    pipeline: ID3D12PipelineState,
    own_heaps: bool,
}

#[derive(Copy, Clone, Zeroable, Pod)]
#[repr(C)]
struct MipConstants {
    inv_out_texel_size: [f32; 2],
    src_mip_index: u32,
}

pub struct MipmapGenContext<'a> {
    gen: &'a D3D12MipmapGen,
    cmd: &'a ID3D12GraphicsCommandList,
    heap: &'a mut D3D12DescriptorHeap<ResourceWorkHeap>,
    residuals: Vec<D3D12DescriptorHeapSlot<ResourceWorkHeap>>,
    residual_uav_descs: Vec<D3D12_RESOURCE_UAV_BARRIER>,
}

impl<'a> MipmapGenContext<'a> {
    fn new(
        gen: &'a D3D12MipmapGen,
        cmd: &'a ID3D12GraphicsCommandList,
        heap: &'a mut D3D12DescriptorHeap<ResourceWorkHeap>,
    ) -> MipmapGenContext<'a> {
        Self {
            gen,
            cmd,
            heap,
            residuals: Vec::new(),
            residual_uav_descs: Vec::new(),
        }
    }

    /// Generate a set of mipmaps for the resource.
    /// This is a "cheap" action and only dispatches a compute shader.
    pub fn generate_mipmaps(
        &mut self,
        resource: &ID3D12Resource,
        miplevels: u16,
        size: Size<u32>,
        format: DXGI_FORMAT,
    ) -> error::Result<()> {
        unsafe {
            let (residuals_heap, residual_barriers) = self
                .gen
                .generate_mipmaps(self.cmd, resource, miplevels, size, format, self.heap)?;
            self.residuals.extend(residuals_heap);
            self.residual_uav_descs.extend(residual_barriers);
        }

        Ok(())
    }

    fn close(
        self,
    ) -> (
        Vec<D3D12DescriptorHeapSlot<ResourceWorkHeap>>,
        Vec<D3D12_RESOURCE_UAV_BARRIER>,
    ) {
        (self.residuals, self.residual_uav_descs)
    }
}

impl D3D12MipmapGen {
    pub fn new(device: &ID3D12Device, own_heaps: bool) -> error::Result<D3D12MipmapGen> {
        unsafe {
            let library = DxcCreateInstance(&CLSID_DxcLibrary)?;
            let validator = DxcCreateInstance(&CLSID_DxcValidator)?;

            let blob = dxc_validate_shader(&library, &validator, GENERATE_MIPMAPS_CS)?;

            let blob =
                std::slice::from_raw_parts(blob.GetBufferPointer().cast(), blob.GetBufferSize());

            let root_signature: ID3D12RootSignature = device.CreateRootSignature(0, blob)?;

            let desc = D3D12_COMPUTE_PIPELINE_STATE_DESC {
                pRootSignature: ManuallyDrop::new(Some(root_signature.clone())),
                CS: D3D12_SHADER_BYTECODE {
                    pShaderBytecode: blob.as_ptr().cast(),
                    BytecodeLength: blob.len(),
                },
                NodeMask: 0,
                ..Default::default()
            };

            let pipeline = device.CreateComputePipelineState(&desc)?;
            drop(ManuallyDrop::into_inner(desc.pRootSignature));
            Ok(D3D12MipmapGen {
                device: device.clone(),
                root_signature,
                pipeline,
                own_heaps,
            })
        }
    }

    /// If own_heap is false, this sets the compute root signature.
    /// Otherwise, this does nothing and the root signature is set upon entering a
    /// Mipmapping Context
    pub fn pin_root_signature(&self, cmd: &ID3D12GraphicsCommandList) {
        if !self.own_heaps {
            unsafe {
                cmd.SetComputeRootSignature(&self.root_signature);
            }
        }
    }

    /// Enters a mipmapping compute context.
    /// This is a relatively expensive operation if set_heap is true,
    /// and should only be done at most a few times per frame.
    ///
    /// If own_heap is false, then you must ensure that the compute root signature
    /// is already bound before entering the context.
    ///
    /// The list of returned descriptors must be kept around until the command list has been
    /// submitted.
    #[must_use]
    pub fn mipmapping_context<F, E>(
        &self,
        cmd: &ID3D12GraphicsCommandList,
        work_heap: &mut D3D12DescriptorHeap<ResourceWorkHeap>,
        mut f: F,
    ) -> Result<
        (
            Vec<D3D12DescriptorHeapSlot<ResourceWorkHeap>>,
            Vec<D3D12_RESOURCE_UAV_BARRIER>,
        ),
        E,
    >
    where
        F: FnMut(&mut MipmapGenContext) -> Result<(), E>,
    {
        let heap: ID3D12DescriptorHeap = (&(*work_heap)).into();
        unsafe {
            cmd.SetPipelineState(&self.pipeline);

            if self.own_heaps {
                cmd.SetComputeRootSignature(&self.root_signature);
                cmd.SetDescriptorHeaps(&[Some(heap)]);
            }
        }

        let mut context = MipmapGenContext::new(self, cmd, work_heap);
        f(&mut context)?;
        Ok(context.close())
    }

    /// SAFETY:
    ///   - handle must be a CPU handle to an SRV
    ///   - work_heap must have enough descriptors to fit all miplevels.
    unsafe fn generate_mipmaps(
        &self,
        cmd: &ID3D12GraphicsCommandList,
        resource: &ID3D12Resource,
        miplevels: u16,
        size: Size<u32>,
        format: DXGI_FORMAT,
        work_heap: &mut D3D12DescriptorHeap<ResourceWorkHeap>,
    ) -> error::Result<(
        Vec<D3D12DescriptorHeapSlot<ResourceWorkHeap>>,
        Vec<D3D12_RESOURCE_UAV_BARRIER>,
    )> {
        // create views for mipmap generation
        let srv = work_heap.alloc_slot()?;
        unsafe {
            let srv_desc = D3D12_SHADER_RESOURCE_VIEW_DESC {
                Format: format,
                ViewDimension: D3D12_SRV_DIMENSION_TEXTURE2D,
                Shader4ComponentMapping: D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                Anonymous: D3D12_SHADER_RESOURCE_VIEW_DESC_0 {
                    Texture2D: D3D12_TEX2D_SRV {
                        MipLevels: miplevels as u32,
                        ..Default::default()
                    },
                },
            };

            self.device
                .CreateShaderResourceView(resource, Some(&srv_desc), *srv.deref().as_ref());
        }

        let mut heap_slots = Vec::with_capacity(miplevels as usize);
        heap_slots.push(srv);

        for i in 1..miplevels {
            let descriptor = work_heap.alloc_slot()?;
            let desc = D3D12_UNORDERED_ACCESS_VIEW_DESC {
                Format: format,
                ViewDimension: D3D12_UAV_DIMENSION_TEXTURE2D,
                Anonymous: D3D12_UNORDERED_ACCESS_VIEW_DESC_0 {
                    Texture2D: D3D12_TEX2D_UAV {
                        MipSlice: i as u32,
                        ..Default::default()
                    },
                },
            };

            unsafe {
                self.device.CreateUnorderedAccessView(
                    resource,
                    None,
                    Some(&desc),
                    *descriptor.deref().as_ref(),
                );
            }
            heap_slots.push(descriptor);
        }

        unsafe {
            cmd.SetComputeRootDescriptorTable(0, *heap_slots[0].deref().as_ref());
        }

        let mut residual_uavs = Vec::new();
        for i in 1..miplevels as u32 {
            let scaled = size.scale_mipmap(i);
            let mipmap_params = MipConstants {
                inv_out_texel_size: [1.0 / scaled.width as f32, 1.0 / scaled.height as f32],
                src_mip_index: (i - 1),
            };

            let mipmap_params = bytemuck::bytes_of(&mipmap_params);

            let barriers = [
                util::d3d12_get_resource_transition_subresource(
                    resource,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    i - 1,
                ),
                util::d3d12_get_resource_transition_subresource(
                    resource,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    i,
                ),
            ];

            unsafe {
                cmd.ResourceBarrier(&barriers);

                cmd.SetComputeRootDescriptorTable(1, *heap_slots[i as usize].deref().as_ref());
                cmd.SetComputeRoot32BitConstants(
                    2,
                    (std::mem::size_of::<MipConstants>() / std::mem::size_of::<u32>()) as u32,
                    mipmap_params.as_ptr().cast(),
                    0,
                );

                cmd.Dispatch(
                    std::cmp::max(scaled.width.div_ceil(8), 1),
                    std::cmp::max(scaled.height.div_ceil(8), 1),
                    1,
                );
            }

            let uav_barrier = ManuallyDrop::new(D3D12_RESOURCE_UAV_BARRIER {
                pResource: ManuallyDrop::new(Some(resource.clone())),
            });

            let barriers = [
                D3D12_RESOURCE_BARRIER {
                    Type: D3D12_RESOURCE_BARRIER_TYPE_UAV,
                    Anonymous: D3D12_RESOURCE_BARRIER_0 { UAV: uav_barrier },
                    ..Default::default()
                },
                util::d3d12_get_resource_transition_subresource(
                    resource,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    i,
                ),
                util::d3d12_get_resource_transition_subresource(
                    resource,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    i - 1,
                ),
            ];

            unsafe {
                cmd.ResourceBarrier(&barriers);
            }

            let uav = unsafe {
                let [barrier, ..] = barriers;
                barrier.Anonymous.UAV
            };

            residual_uavs.push(ManuallyDrop::into_inner(uav))
        }

        Ok((heap_slots, residual_uavs))
    }
}
