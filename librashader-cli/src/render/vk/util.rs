use ash::vk;

#[inline(always)]
pub unsafe fn vulkan_image_layout_transition_levels(
    device: &ash::Device,
    cmd: vk::CommandBuffer,
    image: vk::Image,
    levels: u32,
    old_layout: vk::ImageLayout,
    new_layout: vk::ImageLayout,
    src_access: vk::AccessFlags,
    dst_access: vk::AccessFlags,
    src_stage: vk::PipelineStageFlags,
    dst_stage: vk::PipelineStageFlags,

    src_queue_family_index: u32,
    dst_queue_family_index: u32,
) {
    let mut barrier = vk::ImageMemoryBarrier::default();
    barrier.s_type = vk::StructureType::IMAGE_MEMORY_BARRIER;
    barrier.p_next = std::ptr::null();
    barrier.src_access_mask = src_access;
    barrier.dst_access_mask = dst_access;
    barrier.old_layout = old_layout;
    barrier.new_layout = new_layout;
    barrier.src_queue_family_index = src_queue_family_index;
    barrier.dst_queue_family_index = dst_queue_family_index;
    barrier.image = image;
    barrier.subresource_range.aspect_mask = vk::ImageAspectFlags::COLOR;
    barrier.subresource_range.base_array_layer = 0;
    barrier.subresource_range.level_count = levels;
    barrier.subresource_range.layer_count = vk::REMAINING_ARRAY_LAYERS;
    device.cmd_pipeline_barrier(
        cmd,
        src_stage,
        dst_stage,
        vk::DependencyFlags::empty(),
        &[],
        &[],
        &[barrier],
    )
}
