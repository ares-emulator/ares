use ash::vk;
use gpu_allocator::vulkan::{Allocation, AllocationCreateDesc, AllocationScheme, Allocator};
use gpu_allocator::MemoryLocation;
use parking_lot::Mutex;

use ash::prelude::VkResult;
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
    ) -> VkResult<VulkanImageMemory> {
        let allocation = allocator
            .lock()
            .allocate(&AllocationCreateDesc {
                name: "imagemem",
                requirements,
                location: MemoryLocation::GpuOnly,
                linear: false,
                allocation_scheme: AllocationScheme::DedicatedImage(*image),
            })
            .unwrap();

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
