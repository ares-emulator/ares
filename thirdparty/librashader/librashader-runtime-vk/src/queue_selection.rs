use ash::vk;

fn find_graphics_queue_family(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
) -> u32 {
    let queue_families =
        unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
    // find the most specialized transfer queue.
    for (index, queue_family) in queue_families.iter().enumerate() {
        if queue_family.queue_count > 0
            && queue_family.queue_flags.contains(vk::QueueFlags::GRAPHICS)
        {
            return index as u32;
        }
    }

    0
}

pub fn get_graphics_queue(
    instance: &ash::Instance,
    device: &ash::Device,
    physical_device: vk::PhysicalDevice,
) -> vk::Queue {
    let queue_family = find_graphics_queue_family(instance, physical_device);
    unsafe { device.get_device_queue(queue_family, 0) }
}
