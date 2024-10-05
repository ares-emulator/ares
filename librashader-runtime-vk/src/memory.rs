use crate::error;
use crate::error::FilterChainError;
use ash::vk;
use gpu_allocator::vulkan::{
    Allocation, AllocationCreateDesc, AllocationScheme, Allocator, AllocatorCreateDesc,
};
use gpu_allocator::{AllocationSizes, MemoryLocation};
use librashader_runtime::uniforms::UniformStorageAccess;
use parking_lot::Mutex;

use std::ffi::c_void;
use std::mem::ManuallyDrop;
use std::ops::{Deref, DerefMut};
use std::ptr::NonNull;
use std::sync::Arc;

pub struct VulkanImageMemory {
    allocation: Option<Allocation>,
    allocator: Arc<Mutex<Allocator>>,
}

impl VulkanImageMemory {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<Mutex<Allocator>>,
        requirements: vk::MemoryRequirements,
        image: &vk::Image,
    ) -> error::Result<VulkanImageMemory> {
        let allocation = allocator.lock().allocate(&AllocationCreateDesc {
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
            if let Err(e) = self.allocator.lock().free(allocation) {
                println!("librashader-runtime-vk: [warn] failed to deallocate image buffer {e}")
            }
        }
    }
}

pub struct VulkanBuffer {
    pub handle: vk::Buffer,
    device: Arc<ash::Device>,
    allocation: ManuallyDrop<Allocation>,
    allocator: Arc<Mutex<Allocator>>,
    size: vk::DeviceSize,
}

impl VulkanBuffer {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<Mutex<Allocator>>,
        usage: vk::BufferUsageFlags,
        size: usize,
    ) -> error::Result<VulkanBuffer> {
        unsafe {
            let buffer_info = vk::BufferCreateInfo::default()
                .size(size as vk::DeviceSize)
                .usage(usage)
                .sharing_mode(vk::SharingMode::EXCLUSIVE);
            let buffer = device.create_buffer(&buffer_info, None)?;

            let memory_reqs = device.get_buffer_memory_requirements(buffer);

            let alloc = allocator.lock().allocate(&AllocationCreateDesc {
                name: "buffer",
                requirements: memory_reqs,
                location: MemoryLocation::CpuToGpu,
                linear: true,
                allocation_scheme: AllocationScheme::DedicatedBuffer(buffer),
            })?;

            device.bind_buffer_memory(buffer, alloc.memory(), 0)?;

            Ok(VulkanBuffer {
                handle: buffer,
                allocation: ManuallyDrop::new(alloc),
                allocator: Arc::clone(allocator),
                size: size as vk::DeviceSize,
                device: device.clone(),
            })
        }
    }

    pub fn as_mut_slice(&mut self) -> error::Result<&mut [u8]> {
        let Some(allocation) = self.allocation.mapped_slice_mut() else {
            return Err(FilterChainError::AllocationDoesNotExist);
        };
        Ok(allocation)
    }
}

impl Drop for VulkanBuffer {
    fn drop(&mut self) {
        unsafe {
            // SAFETY: things can not be double dropped.
            let allocation = ManuallyDrop::take(&mut self.allocation);
            if let Err(e) = self.allocator.lock().free(allocation) {
                println!("librashader-runtime-vk: [warn] failed to deallocate buffer memory {e}")
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
        allocator: &Arc<Mutex<Allocator>>,
        usage: vk::BufferUsageFlags,
        size: usize,
    ) -> error::Result<Self> {
        let buffer = ManuallyDrop::new(VulkanBuffer::new(device, allocator, usage, size)?);

        let Some(ptr) = buffer.allocation.mapped_ptr() else {
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
            let buffer_info = [vk::DescriptorBufferInfo::default()
                .buffer(self.buffer.handle)
                .offset(0)
                .range(storage.ubo_slice().len() as vk::DeviceSize)];

            let write_info = vk::WriteDescriptorSet::default()
                .descriptor_type(vk::DescriptorType::UNIFORM_BUFFER)
                .dst_set(descriptor_set)
                .dst_binding(binding)
                .dst_array_element(0)
                .buffer_info(&buffer_info);

            self.buffer
                .device
                .update_descriptor_sets(&[write_info], &[])
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

#[allow(unused)]
pub fn find_vulkan_memory_type(
    props: &vk::PhysicalDeviceMemoryProperties,
    device_reqs: u32,
    host_reqs: vk::MemoryPropertyFlags,
) -> error::Result<u32> {
    for i in 0..vk::MAX_MEMORY_TYPES {
        if device_reqs & (1 << i) != 0
            && props.memory_types[i].property_flags & host_reqs == host_reqs
        {
            return Ok(i as u32);
        }
    }

    if host_reqs == vk::MemoryPropertyFlags::empty() {
        Err(FilterChainError::VulkanMemoryError(device_reqs))
    } else {
        Ok(find_vulkan_memory_type(
            props,
            device_reqs,
            vk::MemoryPropertyFlags::empty(),
        )?)
    }
}

pub fn create_allocator(
    device: ash::Device,
    instance: ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> error::Result<Arc<Mutex<Allocator>>> {
    let alloc = Allocator::new(&AllocatorCreateDesc {
        instance,
        device,
        physical_device,
        debug_settings: Default::default(),
        buffer_device_address: false,
        allocation_sizes: AllocationSizes::default(),
    })?;
    Ok(Arc::new(Mutex::new(alloc)))
}
