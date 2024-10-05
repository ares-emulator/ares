use ash::vk;
use gpu_allocator::vulkan::{
    Allocation, AllocationCreateDesc, AllocationScheme, Allocator, AllocatorCreateDesc,
};
use gpu_allocator::{AllocationSizes, MemoryLocation};
use parking_lot::Mutex;

use anyhow::anyhow;
use std::mem::ManuallyDrop;
use std::sync::Arc;

pub struct VulkanImageMemory {
    pub(crate) allocation: ManuallyDrop<Allocation>,
    allocator: Arc<Mutex<Allocator>>,
}

impl VulkanImageMemory {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<Mutex<Allocator>>,
        requirements: vk::MemoryRequirements,
        image: &vk::Image,
        location: MemoryLocation,
    ) -> anyhow::Result<VulkanImageMemory> {
        let allocation = allocator.lock().allocate(&AllocationCreateDesc {
            name: "imagemem",
            requirements,
            location,
            linear: false,
            allocation_scheme: AllocationScheme::DedicatedImage(*image),
        })?;

        unsafe {
            device.bind_image_memory(*image, allocation.memory(), 0)?;
            Ok(VulkanImageMemory {
                allocation: ManuallyDrop::new(allocation),
                allocator: Arc::clone(allocator),
            })
        }
    }
}

impl Drop for VulkanImageMemory {
    fn drop(&mut self) {
        let allocation = unsafe { ManuallyDrop::take(&mut self.allocation) };
        if let Err(e) = self.allocator.lock().free(allocation) {
            println!("librashader-runtime-vk: [warn] failed to deallocate image buffer {e}")
        }
    }
}

pub struct VulkanBuffer {
    pub handle: vk::Buffer,
    device: Arc<ash::Device>,
    allocation: ManuallyDrop<Allocation>,
    allocator: Arc<Mutex<Allocator>>,
}

impl VulkanBuffer {
    pub fn new(
        device: &Arc<ash::Device>,
        allocator: &Arc<Mutex<Allocator>>,
        usage: vk::BufferUsageFlags,
        size: usize,
    ) -> anyhow::Result<VulkanBuffer> {
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

            // let alloc = device.allocate_memory(&alloc_info, None)?;
            device.bind_buffer_memory(buffer, alloc.memory(), 0)?;

            Ok(VulkanBuffer {
                handle: buffer,
                allocation: ManuallyDrop::new(alloc),
                allocator: Arc::clone(allocator),
                device: device.clone(),
            })
        }
    }

    pub fn as_mut_slice(&mut self) -> anyhow::Result<&mut [u8]> {
        let Some(allocation) = self.allocation.mapped_slice_mut() else {
            return Err(anyhow!("Allocation is not host visible"));
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
                println!("librashader-test-vk: [warn] failed to deallocate buffer memory {e}")
            }

            if self.handle != vk::Buffer::null() {
                self.device.destroy_buffer(self.handle, None);
            }
        }
    }
}

pub fn create_allocator(
    device: ash::Device,
    instance: ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> Arc<Mutex<Allocator>> {
    let alloc = Allocator::new(&AllocatorCreateDesc {
        instance,
        device,
        physical_device,
        debug_settings: Default::default(),
        buffer_device_address: false,
        allocation_sizes: AllocationSizes::default(),
    })
    .unwrap();
    Arc::new(Mutex::new(alloc))
}
