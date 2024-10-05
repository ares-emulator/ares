use ash::vk;
use gpu_allocator::vulkan::Allocator;
use librashader::runtime::vk::VulkanObjects;
use parking_lot::Mutex;
use std::ffi::CStr;
use std::sync::Arc;

pub struct VulkanBase {
    device: Arc<ash::Device>,
    graphics_queue: vk::Queue,
    allocator: Arc<Mutex<Allocator>>,
    cmd_buffer: vk::CommandBuffer,
    pool: vk::CommandPool,
}

impl From<&VulkanBase> for VulkanObjects {
    fn from(value: &VulkanBase) -> Self {
        VulkanObjects {
            device: Arc::clone(&value.device),
            alloc: Arc::clone(&value.allocator),
            queue: value.graphics_queue.clone(),
        }
    }
}

const KHRONOS_VALIDATION: &[u8] = b"VK_LAYER_KHRONOS_validation\0";
const LIBRASHADER_VULKAN: &[u8] = b"librashader Vulkan\0";

impl VulkanBase {
    pub fn new() -> anyhow::Result<Self> {
        let entry = unsafe { ash::Entry::load() }?;
        let layers = [KHRONOS_VALIDATION.as_ptr().cast()];
        let app_info = vk::ApplicationInfo::default()
            .application_name(unsafe { CStr::from_bytes_with_nul_unchecked(LIBRASHADER_VULKAN) })
            .engine_name(unsafe { CStr::from_bytes_with_nul_unchecked(LIBRASHADER_VULKAN) })
            .engine_version(0)
            .application_version(0)
            .api_version(vk::make_api_version(0, 1, 3, 0));

        let create_info = vk::InstanceCreateInfo::default()
            .application_info(&app_info)
            .enabled_layer_names(&layers)
            .enabled_extension_names(&[]);

        let instance = unsafe { entry.create_instance(&create_info, None) }?;

        let physical_device = super::physical_device::pick_physical_device(&instance)?;

        let (device, queue, cmd_pool) = Self::create_device(&instance, &physical_device)?;

        let alloc = super::memory::create_allocator(
            device.clone(),
            instance.clone(),
            physical_device.clone(),
        );

        let buffer_info = vk::CommandBufferAllocateInfo::default()
            .command_pool(cmd_pool)
            .level(vk::CommandBufferLevel::PRIMARY)
            .command_buffer_count(1);

        let buffers = unsafe { device.allocate_command_buffers(&buffer_info) }?
            .into_iter()
            .next()
            .unwrap();

        Ok(Self {
            device: Arc::new(device),
            graphics_queue: queue,
            // debug,
            allocator: alloc,
            pool: cmd_pool,
            cmd_buffer: buffers,
        })
    }

    pub(crate) fn device(&self) -> &Arc<ash::Device> {
        &self.device
    }
    pub(crate) fn allocator(&self) -> &Arc<Mutex<Allocator>> {
        &self.allocator
    }

    fn create_device(
        instance: &ash::Instance,
        physical_device: &vk::PhysicalDevice,
    ) -> anyhow::Result<(ash::Device, vk::Queue, vk::CommandPool)> {
        let _debug = [unsafe { CStr::from_bytes_with_nul_unchecked(KHRONOS_VALIDATION).as_ptr() }];
        let indices = super::physical_device::find_queue_family(&instance, *physical_device);

        let queue_info = [vk::DeviceQueueCreateInfo::default()
            .queue_family_index(indices.graphics_family()?)
            .queue_priorities(&[1.0f32])];

        let mut physical_device_features =
            vk::PhysicalDeviceVulkan13Features::default().dynamic_rendering(true);

        let extensions = [ash::khr::dynamic_rendering::NAME.as_ptr()];

        let device_create_info = vk::DeviceCreateInfo::default()
            .queue_create_infos(&queue_info)
            .enabled_extension_names(&extensions)
            .push_next(&mut physical_device_features);

        let device =
            unsafe { instance.create_device(*physical_device, &device_create_info, None)? };

        let queue = unsafe { device.get_device_queue(indices.graphics_family()?, 0) };

        let create_info = vk::CommandPoolCreateInfo::default()
            .flags(vk::CommandPoolCreateFlags::RESET_COMMAND_BUFFER)
            .queue_family_index(indices.graphics_family()?);
        let pool = unsafe { device.create_command_pool(&create_info, None) }?;

        Ok((device, queue, pool))
    }

    /// Simple helper function to synchronously queue work on the graphics queue
    pub fn queue_work<T>(&self, f: impl FnOnce(vk::CommandBuffer) -> T) -> anyhow::Result<T> {
        unsafe {
            self.device.begin_command_buffer(
                self.cmd_buffer,
                &vk::CommandBufferBeginInfo::default()
                    .flags(vk::CommandBufferUsageFlags::ONE_TIME_SUBMIT),
            )?
        }

        let result = f(self.cmd_buffer);

        unsafe {
            self.device.end_command_buffer(self.cmd_buffer)?;

            self.device.queue_submit(
                self.graphics_queue,
                &[vk::SubmitInfo::default().command_buffers(&[self.cmd_buffer])],
                vk::Fence::null(),
            )?;
            self.device.queue_wait_idle(self.graphics_queue)?;
            self.device
                .reset_command_buffer(self.cmd_buffer, vk::CommandBufferResetFlags::empty())?;
        }

        Ok(result)
    }
}

impl Drop for VulkanBase {
    fn drop(&mut self) {
        unsafe {
            self.device.destroy_command_pool(self.pool, None);
        }
    }
}
