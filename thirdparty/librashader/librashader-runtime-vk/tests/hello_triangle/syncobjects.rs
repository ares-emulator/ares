use ash::prelude::VkResult;
use ash::vk;

pub struct SyncObjects {
    pub image_available: Vec<vk::Semaphore>,
    pub render_finished: Vec<vk::Semaphore>,
    pub in_flight: Vec<vk::Fence>,
}

impl SyncObjects {
    pub fn new(device: &ash::Device, frames_in_flight: usize) -> VkResult<SyncObjects> {
        unsafe {
            let mut image_available = Vec::new();
            let mut render_finished = Vec::new();
            let mut in_flight = Vec::new();

            for _ in 0..frames_in_flight {
                image_available
                    .push(device.create_semaphore(&vk::SemaphoreCreateInfo::default(), None)?);
                render_finished
                    .push(device.create_semaphore(&vk::SemaphoreCreateInfo::default(), None)?);
                in_flight.push(device.create_fence(
                    &vk::FenceCreateInfo::default().flags(vk::FenceCreateFlags::SIGNALED),
                    None,
                )?)
            }

            Ok(SyncObjects {
                image_available,
                render_finished,
                in_flight,
            })
        }
    }
}
