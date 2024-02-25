use crate::error;
use crate::error::FilterChainError;
use ash::vk;
use gpu_allocator::vulkan::{Allocation, AllocationCreateDesc, AllocationScheme, Allocator};
use gpu_allocator::MemoryLocation;
use librashader_runtime::uniforms::UniformStorageAccess;
use parking_lot::RwLock;

use std::ffi::c_void;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};
use std::ptr::NonNull;
use std::sync::Arc;

pub struct VulkanImageMemory {
    allocation: Option<Allocation>,
    allocator: Arc<RwLock<Allocator>>,
}

impl VulkanImageMemory {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<RwLock<Allocator>>,
        requirements: vk::MemoryRequirements,
        image: &vk::Image,
    ) -> error::Result<VulkanImageMemory> {
        let allocation = allocator.write().allocate(&AllocationCreateDesc {
            name: "imagemem",
            requirements,
            location: MemoryLocation::GpuOnly,
            linear: false,
            allocation_scheme: AllocationScheme::DedicatedImage(*image),
        })?;

        unsafe {
            device.bind_image_memory(*image, allocation.memory(), 0)?;
            Ok(VulkanImageMemory {
                allocation: Some(allocation),
                allocator: Arc::clone(allocator),
            })
        }
    }
}

impl Drop for VulkanImageMemory {
    fn drop(&mut self) {
        let allocation = self.allocation.take();
        if let Some(allocation) = allocation {
            if let Err(e) = self.allocator.write().free(allocation) {
                println!("librashader-runtime-vk: [warn] failed to deallocate image buffer {e}")
            }
        }
    }
}

pub struct VulkanBuffer {
    pub handle: vk::Buffer,
    device: Arc<ash::Device>,
    memory: Option<Allocation>,
    allocator: Arc<RwLock<Allocator>>,
    size: vk::DeviceSize,
}

impl VulkanBuffer {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<RwLock<Allocator>>,
        usage: vk::BufferUsageFlags,
        size: usize,
    ) -> error::Result<VulkanBuffer> {
        unsafe {
            let buffer_info = vk::BufferCreateInfo::builder()
                .size(size as vk::DeviceSize)
                .usage(usage)
                .sharing_mode(vk::SharingMode::EXCLUSIVE);
            let buffer = device.create_buffer(&buffer_info, None)?;

            let memory_reqs = device.get_buffer_memory_requirements(buffer);

            let alloc = allocator.write().allocate(&AllocationCreateDesc {
                name: "buffer",
                requirements: memory_reqs,
                location: MemoryLocation::CpuToGpu,
                linear: true,
                allocation_scheme: AllocationScheme::DedicatedBuffer(buffer),
            })?;

            // let alloc = device.allocate_memory(&alloc_info, None)?;
            device.bind_buffer_memory(buffer, alloc.memory(), 0)?;

            Ok(VulkanBuffer {
                handle: buffer,
                memory: Some(alloc),
                allocator: Arc::clone(allocator),
                size: size as vk::DeviceSize,
                device: device.clone(),
            })
        }
    }

    pub fn as_mut_slice(&mut self) -> error::Result<&mut [u8]> {
        let Some(allocation) = self.memory.as_mut() else {
            return Err(FilterChainError::AllocationDoesNotExist);
        };
        let Some(allocation) = allocation.mapped_slice_mut() else {
            return Err(FilterChainError::AllocationDoesNotExist);
        };
        Ok(allocation)
    }
}

impl Drop for VulkanBuffer {
    fn drop(&mut self) {
        unsafe {
            if let Some(allocation) = self.memory.take() {
                if let Err(e) = self.allocator.write().free(allocation) {
                    println!(
                        "librashader-runtime-vk: [warn] failed to deallocate buffer memory {e}"
                    )
                }
            }

            if self.handle != vk::Buffer::null() {
                self.device.destroy_buffer(self.handle, None);
            }
        }
    }
}

/// SAFETY: Creating the pointer should be safe in multithreaded contexts.
///
/// Mutation is guarded by DerefMut<Target=[u8]>, so exclusive access is guaranteed.
/// We do not ever leak the pointer to C.
unsafe impl Send for RawVulkanBuffer {}
unsafe impl Sync for RawVulkanBuffer {}
pub struct RawVulkanBuffer {
    buffer: ManuallyDrop<VulkanBuffer>,
    ptr: NonNull<c_void>,
}

impl RawVulkanBuffer {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<RwLock<Allocator>>,
        usage: vk::BufferUsageFlags,
        size: usize,
    ) -> error::Result<Self> {
        let buffer = ManuallyDrop::new(VulkanBuffer::new(device, allocator, usage, size)?);

        let Some(ptr) = buffer.memory.as_ref().map(|m| m.mapped_ptr()).flatten() else {
            return Err(FilterChainError::AllocationDoesNotExist);
        };

        Ok(RawVulkanBuffer { buffer, ptr })
    }

    pub fn bind_to_descriptor_set(
        &self,
        descriptor_set: vk::DescriptorSet,
        binding: u32,
        storage: &impl UniformStorageAccess,
    ) -> error::Result<()> {
        unsafe {
            let buffer_info = vk::DescriptorBufferInfo::builder()
                .buffer(self.buffer.handle)
                .offset(0)
                .range(storage.ubo_slice().len() as vk::DeviceSize);

            let buffer_info = [*buffer_info];
            let write_info = vk::WriteDescriptorSet::builder()
                .descriptor_type(vk::DescriptorType::UNIFORM_BUFFER)
                .dst_set(descriptor_set)
                .dst_binding(binding)
                .dst_array_element(0)
                .buffer_info(&buffer_info);

            self.buffer
                .device
                .update_descriptor_sets(&[*write_info], &[])
        }
        Ok(())
    }
}

impl Drop for RawVulkanBuffer {
    fn drop(&mut self) {
        unsafe {
            ManuallyDrop::drop(&mut self.buffer);
        }
    }
}

impl Deref for RawVulkanBuffer {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        unsafe { std::slice::from_raw_parts(self.ptr.as_ptr().cast(), self.buffer.size as usize) }
    }
}

impl DerefMut for RawVulkanBuffer {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe {
            std::slice::from_raw_parts_mut(self.ptr.as_ptr().cast(), self.buffer.size as usize)
        }
    }
}
