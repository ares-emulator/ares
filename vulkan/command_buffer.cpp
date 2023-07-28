/* Copyright (c) 2017-2023 Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define NOMINMAX
#include "command_buffer.hpp"
#include "device.hpp"
#include "format.hpp"
#include "thread_id.hpp"
#include "vulkan_prerotate.hpp"
#include "indirect_layout.hpp"
#include "timer.hpp"
#include <string.h>

using namespace Util;

namespace Vulkan
{
static inline uint32_t get_combined_spec_constant_mask(const DeferredPipelineCompile &compile)
{
	return compile.potential_static_state.spec_constant_mask |
	       (compile.potential_static_state.internal_spec_constant_mask << VULKAN_NUM_USER_SPEC_CONSTANTS);
}

CommandBuffer::CommandBuffer(Device *device_, VkCommandBuffer cmd_, VkPipelineCache cache, Type type_)
    : device(device_)
    , table(device_->get_device_table())
    , cmd(cmd_)
    , type(type_)
{
	pipeline_state.cache = cache;
	begin_compute();
	set_opaque_state();
	memset(&pipeline_state.static_state, 0, sizeof(pipeline_state.static_state));
	memset(&bindings, 0, sizeof(bindings));

	// Set up extra state which PSO creation depends on implicitly.
	// This needs to affect hashing to make Fossilize path behave as expected.
	auto &features = device->get_device_features();
	pipeline_state.subgroup_size_tag =
			(features.subgroup_size_control_properties.minSubgroupSize << 0) |
			(features.subgroup_size_control_properties.maxSubgroupSize << 8);

	device->lock.read_only_cache.lock_read();
}

CommandBuffer::~CommandBuffer()
{
	VK_ASSERT(vbo_block.mapped == nullptr);
	VK_ASSERT(ibo_block.mapped == nullptr);
	VK_ASSERT(ubo_block.mapped == nullptr);
	VK_ASSERT(staging_block.mapped == nullptr);
	device->lock.read_only_cache.unlock_read();
}

void CommandBuffer::fill_buffer(const Buffer &dst, uint32_t value)
{
	fill_buffer(dst, value, 0, VK_WHOLE_SIZE);
}

void CommandBuffer::fill_buffer(const Buffer &dst, uint32_t value, VkDeviceSize offset, VkDeviceSize size)
{
	table.vkCmdFillBuffer(cmd, dst.get_buffer(), offset, size, value);
}

void CommandBuffer::copy_buffer(const Buffer &dst, VkDeviceSize dst_offset, const Buffer &src, VkDeviceSize src_offset,
                                VkDeviceSize size)
{
	const VkBufferCopy region = {
		src_offset, dst_offset, size,
	};
	table.vkCmdCopyBuffer(cmd, src.get_buffer(), dst.get_buffer(), 1, &region);
}

void CommandBuffer::copy_buffer(const Buffer &dst, const Buffer &src)
{
	VK_ASSERT(dst.get_create_info().size == src.get_create_info().size);
	copy_buffer(dst, 0, src, 0, dst.get_create_info().size);
}

void CommandBuffer::copy_buffer(const Buffer &dst, const Buffer &src, const VkBufferCopy *copies, size_t count)
{
	table.vkCmdCopyBuffer(cmd, src.get_buffer(), dst.get_buffer(), count, copies);
}

void CommandBuffer::copy_image(const Vulkan::Image &dst, const Vulkan::Image &src, const VkOffset3D &dst_offset,
                               const VkOffset3D &src_offset, const VkExtent3D &extent,
                               const VkImageSubresourceLayers &dst_subresource,
                               const VkImageSubresourceLayers &src_subresource)
{
	VkImageCopy region = {};
	region.dstOffset = dst_offset;
	region.srcOffset = src_offset;
	region.extent = extent;
	region.srcSubresource = src_subresource;
	region.dstSubresource = dst_subresource;

	table.vkCmdCopyImage(cmd, src.get_image(), src.get_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
	               dst.get_image(), dst.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
	               1, &region);
}

void CommandBuffer::copy_image(const Image &dst, const Image &src)
{
	uint32_t levels = src.get_create_info().levels;
	VK_ASSERT(src.get_create_info().levels == dst.get_create_info().levels);
	VK_ASSERT(src.get_create_info().width == dst.get_create_info().width);
	VK_ASSERT(src.get_create_info().height == dst.get_create_info().height);
	VK_ASSERT(src.get_create_info().depth == dst.get_create_info().depth);
	VK_ASSERT(src.get_create_info().type == dst.get_create_info().type);
	VK_ASSERT(src.get_create_info().layers == dst.get_create_info().layers);
	VK_ASSERT(src.get_create_info().levels == dst.get_create_info().levels);

	VkImageCopy regions[32] = {};

	for (uint32_t i = 0; i < levels; i++)
	{
		auto &region = regions[i];
		region.extent.width = src.get_create_info().width;
		region.extent.height = src.get_create_info().height;
		region.extent.depth = src.get_create_info().depth;
		region.srcSubresource.aspectMask = format_to_aspect_mask(src.get_format());
		region.srcSubresource.layerCount = src.get_create_info().layers;
		region.dstSubresource.aspectMask = format_to_aspect_mask(dst.get_format());
		region.dstSubresource.layerCount = dst.get_create_info().layers;
		region.srcSubresource.mipLevel = i;
		region.dstSubresource.mipLevel = i;
		VK_ASSERT(region.srcSubresource.aspectMask == region.dstSubresource.aspectMask);
	}

	table.vkCmdCopyImage(cmd, src.get_image(), src.get_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
	                     dst.get_image(), dst.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
	                     levels, regions);
}

void CommandBuffer::copy_buffer_to_image(const Image &image, const Buffer &buffer, unsigned num_blits,
                                         const VkBufferImageCopy *blits)
{
	table.vkCmdCopyBufferToImage(cmd, buffer.get_buffer(),
	                             image.get_image(), image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL), num_blits, blits);
}

void CommandBuffer::copy_image_to_buffer(const Buffer &buffer, const Image &image, unsigned num_blits,
                                         const VkBufferImageCopy *blits)
{
	table.vkCmdCopyImageToBuffer(cmd, image.get_image(), image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
	                             buffer.get_buffer(), num_blits, blits);
}

void CommandBuffer::copy_buffer_to_image(const Image &image, const Buffer &src, VkDeviceSize buffer_offset,
                                         const VkOffset3D &offset, const VkExtent3D &extent, unsigned row_length,
                                         unsigned slice_height, const VkImageSubresourceLayers &subresource)
{
	const VkBufferImageCopy region = {
		buffer_offset,
		row_length, slice_height,
		subresource, offset, extent,
	};
	table.vkCmdCopyBufferToImage(cmd, src.get_buffer(), image.get_image(), image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
	                             1, &region);
}

void CommandBuffer::copy_image_to_buffer(const Buffer &buffer, const Image &image, VkDeviceSize buffer_offset,
                                         const VkOffset3D &offset, const VkExtent3D &extent, unsigned row_length,
                                         unsigned slice_height, const VkImageSubresourceLayers &subresource)
{
	const VkBufferImageCopy region = {
		buffer_offset,
		row_length, slice_height,
		subresource, offset, extent,
	};
	table.vkCmdCopyImageToBuffer(cmd, image.get_image(), image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
	                             buffer.get_buffer(), 1, &region);
}

void CommandBuffer::clear_image(const Image &image, const VkClearValue &value)
{
	auto aspect = format_to_aspect_mask(image.get_format());
	clear_image(image, value, aspect);
}

void CommandBuffer::clear_image(const Image &image, const VkClearValue &value, VkImageAspectFlags aspect)
{
	VK_ASSERT(!framebuffer);
	VK_ASSERT(!actual_render_pass);

	VkImageSubresourceRange range = {};
	range.aspectMask = aspect;
	range.baseArrayLayer = 0;
	range.baseMipLevel = 0;
	range.levelCount = image.get_create_info().levels;
	range.layerCount = image.get_create_info().layers;
	if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
	{
		table.vkCmdClearDepthStencilImage(cmd, image.get_image(), image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
		                                  &value.depthStencil, 1, &range);
	}
	else
	{
		table.vkCmdClearColorImage(cmd, image.get_image(), image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
		                           &value.color, 1, &range);
	}
}

void CommandBuffer::clear_quad(unsigned attachment, const VkClearRect &rect, const VkClearValue &value,
                               VkImageAspectFlags aspect)
{
	VK_ASSERT(framebuffer);
	VK_ASSERT(actual_render_pass);
	VkClearAttachment att = {};
	att.clearValue = value;
	att.colorAttachment = attachment;
	att.aspectMask = aspect;

	auto tmp_rect = rect;
	rect2d_transform_xy(tmp_rect.rect, current_framebuffer_surface_transform,
						framebuffer->get_width(), framebuffer->get_height());
	table.vkCmdClearAttachments(cmd, 1, &att, 1, &tmp_rect);
}

void CommandBuffer::clear_quad(const VkClearRect &rect, const VkClearAttachment *attachments, unsigned num_attachments)
{
	VK_ASSERT(framebuffer);
	VK_ASSERT(actual_render_pass);
	auto tmp_rect = rect;
	rect2d_transform_xy(tmp_rect.rect, current_framebuffer_surface_transform,
	                    framebuffer->get_width(), framebuffer->get_height());
	table.vkCmdClearAttachments(cmd, num_attachments, attachments, 1, &tmp_rect);
}

void CommandBuffer::full_barrier()
{
	VK_ASSERT(!actual_render_pass);
	VK_ASSERT(!framebuffer);
	barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_WRITE_BIT,
	        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT);
}

void CommandBuffer::pixel_barrier()
{
	VK_ASSERT(actual_render_pass);
	VK_ASSERT(framebuffer);
	VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	table.vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                           VK_DEPENDENCY_BY_REGION_BIT, 1, &barrier, 0, nullptr, 0, nullptr);
}

void CommandBuffer::barrier(VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_access,
                            VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_access)
{
	VkDependencyInfo dep = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	VkMemoryBarrier2 b = { VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
	dep.memoryBarrierCount = 1;
	dep.pMemoryBarriers = &b;
	b.srcStageMask = src_stages;
	b.dstStageMask = dst_stages;
	b.srcAccessMask = src_access;
	b.dstAccessMask = dst_access;
	barrier(dep);
}

struct Sync1CompatData
{
	Util::SmallVector<VkMemoryBarrier> mem_barriers;
	Util::SmallVector<VkBufferMemoryBarrier> buf_barriers;
	Util::SmallVector<VkImageMemoryBarrier> img_barriers;
	VkPipelineStageFlags src_stages = 0;
	VkPipelineStageFlags dst_stages = 0;
};

static void convert_vk_dependency_info(const VkDependencyInfo &dep, Sync1CompatData &sync1)
{
	VkPipelineStageFlags2 src_stages = 0;
	VkPipelineStageFlags2 dst_stages = 0;

	for (uint32_t i = 0; i < dep.memoryBarrierCount; i++)
	{
		auto &mem = dep.pMemoryBarriers[i];
		src_stages |= mem.srcStageMask;
		dst_stages |= mem.dstStageMask;
		VkMemoryBarrier barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = convert_vk_access_flags2(mem.srcAccessMask);
		barrier.dstAccessMask = convert_vk_access_flags2(mem.dstAccessMask);
		sync1.mem_barriers.push_back(barrier);
	}

	for (uint32_t i = 0; i < dep.bufferMemoryBarrierCount; i++)
	{
		auto &buf = dep.pBufferMemoryBarriers[i];
		src_stages |= buf.srcStageMask;
		dst_stages |= buf.dstStageMask;

		VkBufferMemoryBarrier barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
		barrier.srcAccessMask = convert_vk_access_flags2(buf.srcAccessMask);
		barrier.dstAccessMask = convert_vk_access_flags2(buf.dstAccessMask);
		barrier.buffer = buf.buffer;
		barrier.offset = buf.offset;
		barrier.size = buf.size;
		barrier.srcQueueFamilyIndex = buf.srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = buf.dstQueueFamilyIndex;
		sync1.buf_barriers.push_back(barrier);
	}

	for (uint32_t i = 0; i < dep.imageMemoryBarrierCount; i++)
	{
		auto &img = dep.pImageMemoryBarriers[i];
		VK_ASSERT(img.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);
		src_stages |= img.srcStageMask;
		dst_stages |= img.dstStageMask;

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.srcAccessMask = convert_vk_access_flags2(img.srcAccessMask);
		barrier.dstAccessMask = convert_vk_access_flags2(img.dstAccessMask);
		barrier.image = img.image;
		barrier.subresourceRange = img.subresourceRange;
		barrier.oldLayout = img.oldLayout;
		barrier.newLayout = img.newLayout;
		barrier.srcQueueFamilyIndex = img.srcQueueFamilyIndex;
		barrier.dstQueueFamilyIndex = img.dstQueueFamilyIndex;
		sync1.img_barriers.push_back(barrier);
	}

	sync1.src_stages |= convert_vk_src_stage2(src_stages);
	sync1.dst_stages |= convert_vk_dst_stage2(dst_stages);
}

void CommandBuffer::barrier(const VkDependencyInfo &dep)
{
	VK_ASSERT(!actual_render_pass);
	VK_ASSERT(!framebuffer);

#ifdef VULKAN_DEBUG
	VkPipelineStageFlags2 stages = 0;
	VkAccessFlags2 access = 0;

	for (uint32_t i = 0; i < dep.memoryBarrierCount; i++)
	{
		auto &b = dep.pMemoryBarriers[i];
		stages |= b.srcStageMask | b.dstStageMask;
		access |= b.srcAccessMask | b.dstAccessMask;
	}

	for (uint32_t i = 0; i < dep.bufferMemoryBarrierCount; i++)
	{
		auto &b = dep.pBufferMemoryBarriers[i];
		stages |= b.srcStageMask | b.dstStageMask;
		access |= b.srcAccessMask | b.dstAccessMask;
	}

	for (uint32_t i = 0; i < dep.imageMemoryBarrierCount; i++)
	{
		auto &b = dep.pImageMemoryBarriers[i];
		stages |= b.srcStageMask | b.dstStageMask;
		access |= b.srcAccessMask | b.dstAccessMask;
	}

	if (stages & VK_PIPELINE_STAGE_TRANSFER_BIT)
		LOGW("Using deprecated TRANSFER stage.\n");
	if (stages & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
		LOGW("Using deprecated BOTTOM_OF_PIPE stage.\n");
	if (stages & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
		LOGW("Using deprecated TOP_OF_PIPE stage.\n");

	if (access & VK_ACCESS_SHADER_READ_BIT)
		LOGW("Using deprecated SHADER_READ access.\n");
	if (stages & VK_ACCESS_SHADER_WRITE_BIT)
		LOGW("Using deprecated SHADER_WRITE access.\n");

	// We cannot convert these automatically so easily to sync1 without more context.
	for (uint32_t i = 0; i < dep.imageMemoryBarrierCount; i++)
	{
		VK_ASSERT(dep.pImageMemoryBarriers[i].oldLayout != VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL &&
		          dep.pImageMemoryBarriers[i].newLayout != VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
	}
#endif

	if (device->get_device_features().sync2_features.synchronization2)
	{
		Util::SmallVector<VkBufferMemoryBarrier2> tmp_buffer;
		Util::SmallVector<VkImageMemoryBarrier2> tmp_image;
		Util::SmallVector<VkMemoryBarrier2> tmp_memory;
		const VkDependencyInfo *final_dep = &dep;
		VkDependencyInfo tmp_dep;

		if (device->get_workarounds().force_sync1_access)
		{
			VkAccessFlags2 merged_access = 0;
			for (uint32_t i = 0; i < dep.memoryBarrierCount; i++)
				merged_access |= dep.pMemoryBarriers[i].srcAccessMask | dep.pMemoryBarriers[i].dstAccessMask;
			for (uint32_t i = 0; i < dep.bufferMemoryBarrierCount; i++)
				merged_access |= dep.pBufferMemoryBarriers[i].srcAccessMask | dep.pBufferMemoryBarriers[i].dstAccessMask;
			for (uint32_t i = 0; i < dep.imageMemoryBarrierCount; i++)
				merged_access |= dep.pImageMemoryBarriers[i].srcAccessMask | dep.pImageMemoryBarriers[i].dstAccessMask;

			if ((merged_access & (VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT |
			                      VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
			                      VK_ACCESS_2_SHADER_STORAGE_READ_BIT |
			                      VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR)) != 0)
			{
				final_dep = &tmp_dep;
				tmp_dep = dep;

				if (dep.memoryBarrierCount != 0)
				{
					tmp_memory.insert(tmp_memory.end(), dep.pMemoryBarriers,
					                  dep.pMemoryBarriers + dep.memoryBarrierCount);
					for (auto &b : tmp_memory)
					{
						b.srcAccessMask = convert_vk_access_flags2(b.srcAccessMask);
						b.dstAccessMask = convert_vk_access_flags2(b.dstAccessMask);
					}
					tmp_dep.pMemoryBarriers = tmp_memory.data();
				}

				if (dep.bufferMemoryBarrierCount != 0)
				{
					tmp_buffer.insert(tmp_buffer.end(), dep.pBufferMemoryBarriers,
					                  dep.pBufferMemoryBarriers + dep.bufferMemoryBarrierCount);
					for (auto &b : tmp_buffer)
					{
						b.srcAccessMask = convert_vk_access_flags2(b.srcAccessMask);
						b.dstAccessMask = convert_vk_access_flags2(b.dstAccessMask);
					}
					tmp_dep.pBufferMemoryBarriers = tmp_buffer.data();
				}

				if (dep.imageMemoryBarrierCount != 0)
				{
					tmp_image.insert(tmp_image.end(), dep.pImageMemoryBarriers,
					                 dep.pImageMemoryBarriers + dep.imageMemoryBarrierCount);
					for (auto &b : tmp_image)
					{
						b.srcAccessMask = convert_vk_access_flags2(b.srcAccessMask);
						b.dstAccessMask = convert_vk_access_flags2(b.dstAccessMask);
					}
					tmp_dep.pImageMemoryBarriers = tmp_image.data();
				}
			}
		}

		table.vkCmdPipelineBarrier2KHR(cmd, final_dep);
	}
	else
	{
		Sync1CompatData sync1;
		convert_vk_dependency_info(dep, sync1);
		table.vkCmdPipelineBarrier(cmd, sync1.src_stages, sync1.dst_stages,
		                           dep.dependencyFlags,
		                           uint32_t(sync1.mem_barriers.size()), sync1.mem_barriers.data(),
		                           uint32_t(sync1.buf_barriers.size()), sync1.buf_barriers.data(),
		                           uint32_t(sync1.img_barriers.size()), sync1.img_barriers.data());
	}
}

void CommandBuffer::buffer_barrier(const Buffer &buffer,
                                   VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_access,
                                   VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_access)
{
	VkBufferMemoryBarrier2 b = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
	VkDependencyInfo dep = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };

	b.srcAccessMask = src_access;
	b.dstAccessMask = dst_access;
	b.buffer = buffer.get_buffer();
	b.offset = 0;
	b.size = VK_WHOLE_SIZE;
	b.srcStageMask = src_stages;
	b.dstStageMask = dst_stages;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	dep.bufferMemoryBarrierCount = 1;
	dep.pBufferMemoryBarriers = &b;

	barrier(dep);
}

// Buffers are always CONCURRENT.
static uint32_t deduce_acquire_release_family_index(Device &device)
{
	uint32_t family = VK_QUEUE_FAMILY_IGNORED;
	auto &queue_info = device.get_queue_info();
	for (auto &i : queue_info.family_indices)
	{
		if (i != VK_QUEUE_FAMILY_IGNORED)
		{
			if (family == VK_QUEUE_FAMILY_IGNORED)
				family = i;
			else if (i != family)
				return VK_QUEUE_FAMILY_IGNORED;
		}
	}

	return family;
}

static uint32_t deduce_acquire_release_family_index(Device &device, const Image &image, uint32_t family_index)
{
	uint32_t family = family_index;
	auto &queue_info = device.get_queue_info();

	if (image.get_create_info().misc & IMAGE_MISC_CONCURRENT_QUEUE_GRAPHICS_BIT)
		if (queue_info.family_indices[QUEUE_INDEX_GRAPHICS] != family)
			return VK_QUEUE_FAMILY_IGNORED;

	if (image.get_create_info().misc &
	    (IMAGE_MISC_CONCURRENT_QUEUE_ASYNC_GRAPHICS_BIT | IMAGE_MISC_CONCURRENT_QUEUE_ASYNC_COMPUTE_BIT))
	{
		if (queue_info.family_indices[QUEUE_INDEX_COMPUTE] != family)
			return VK_QUEUE_FAMILY_IGNORED;
	}

	if (image.get_create_info().misc & IMAGE_MISC_CONCURRENT_QUEUE_ASYNC_TRANSFER_BIT)
		if (queue_info.family_indices[QUEUE_INDEX_COMPUTE] != family)
			return VK_QUEUE_FAMILY_IGNORED;

	return family;
}

void CommandBuffer::release_external_image_barrier(
		const Image &image,
		VkImageLayout old_layout, VkImageLayout new_layout,
		VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access)
{
	VkImageMemoryBarrier2 barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	uint32_t family_index = device->get_queue_info().family_indices[device->get_physical_queue_type(type)];

	barrier.image = image.get_image();
	barrier.subresourceRange = {
		format_to_aspect_mask(image.get_format()),
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	};
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcAccessMask = src_access;

	barrier.srcQueueFamilyIndex = deduce_acquire_release_family_index(*device, image, family_index);
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	barrier.srcStageMask = src_stage;
	barrier.dstStageMask = VK_PIPELINE_STAGE_NONE;

	image_barriers(1, &barrier);
}

void CommandBuffer::acquire_external_image_barrier(
		const Image &image,
		VkImageLayout old_layout, VkImageLayout new_layout,
		VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)
{
	VkImageMemoryBarrier2 b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	uint32_t family_index = device->get_queue_info().family_indices[device->get_physical_queue_type(type)];

	b.image = image.get_image();
	b.subresourceRange = {
		format_to_aspect_mask(image.get_format()),
		0, VK_REMAINING_MIP_LEVELS,
		0, VK_REMAINING_ARRAY_LAYERS
	};
	b.oldLayout = old_layout;
	b.newLayout = new_layout;
	b.dstAccessMask = dst_access;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	b.dstQueueFamilyIndex = deduce_acquire_release_family_index(*device, image, family_index);
	b.srcStageMask = dst_stage;
	b.dstStageMask = dst_stage;
	image_barriers(1, &b);
}

void CommandBuffer::release_external_buffer_barrier(
		const Buffer &buffer,
		VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access)
{
	VkBufferMemoryBarrier2 b = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
	b.buffer = buffer.get_buffer();
	b.size = buffer.get_create_info().size;
	b.srcAccessMask = src_access;
	b.srcQueueFamilyIndex = deduce_acquire_release_family_index(*device);
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	b.srcStageMask = src_stage;
	b.dstStageMask = VK_PIPELINE_STAGE_NONE;
	buffer_barriers(1, &b);
}

void CommandBuffer::acquire_external_buffer_barrier(
		const Buffer &buffer,
		VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)
{
	VkBufferMemoryBarrier2 b = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
	b.buffer = buffer.get_buffer();
	b.size = buffer.get_create_info().size;
	b.dstAccessMask = dst_access;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	b.dstQueueFamilyIndex = deduce_acquire_release_family_index(*device);
	b.srcStageMask = dst_stage;
	b.dstStageMask = dst_stage;
	buffer_barriers(1, &b);
}

void CommandBuffer::image_barrier_acquire(const Vulkan::Image &image,
                                          VkImageLayout old_layout, VkImageLayout new_layout,
                                          VkPipelineStageFlags2 src_stage, uint32_t src_queue_family,
                                          VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access)
{
	VK_ASSERT(!actual_render_pass);
	VK_ASSERT(!framebuffer);
	VK_ASSERT(image.get_create_info().domain != ImageDomain::Transient);

	VkImageMemoryBarrier2 b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	b.srcAccessMask = 0;
	b.dstAccessMask = dst_access;
	b.oldLayout = old_layout;
	b.newLayout = new_layout;
	b.image = image.get_image();
	b.subresourceRange.aspectMask = format_to_aspect_mask(image.get_create_info().format);
	b.subresourceRange.levelCount = image.get_create_info().levels;
	b.subresourceRange.layerCount = image.get_create_info().layers;
	b.srcQueueFamilyIndex = src_queue_family;
	b.dstQueueFamilyIndex = device->get_queue_info().family_indices[device->get_physical_queue_type(type)];
	b.srcStageMask = src_stage;
	b.dstStageMask = dst_stage;

	image_barriers(1, &b);
}

void CommandBuffer::image_barrier_release(const Vulkan::Image &image,
                                          VkImageLayout old_layout, VkImageLayout new_layout,
                                          VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                                          uint32_t dst_queue_family)
{
	VK_ASSERT(!actual_render_pass);
	VK_ASSERT(!framebuffer);
	VK_ASSERT(image.get_create_info().domain != ImageDomain::Transient);

	VkImageMemoryBarrier2 b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	b.srcAccessMask = src_access;
	b.dstAccessMask = 0;
	b.oldLayout = old_layout;
	b.newLayout = new_layout;
	b.image = image.get_image();
	b.subresourceRange.aspectMask = format_to_aspect_mask(image.get_create_info().format);
	b.subresourceRange.levelCount = image.get_create_info().levels;
	b.subresourceRange.layerCount = image.get_create_info().layers;
	b.srcQueueFamilyIndex = device->get_queue_info().family_indices[device->get_physical_queue_type(type)];
	b.dstQueueFamilyIndex = dst_queue_family;
	b.srcStageMask = src_stage;
	b.dstStageMask = 0;

	image_barriers(1, &b);
}

void CommandBuffer::image_barrier(const Image &image,
                                  VkImageLayout old_layout, VkImageLayout new_layout,
                                  VkPipelineStageFlags2 src_stages, VkAccessFlags2 src_access,
                                  VkPipelineStageFlags2 dst_stages, VkAccessFlags2 dst_access)
{
	VK_ASSERT(!actual_render_pass);
	VK_ASSERT(!framebuffer);
	VK_ASSERT(image.get_create_info().domain != ImageDomain::Transient);

	VkImageMemoryBarrier2 b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	b.srcAccessMask = src_access;
	b.dstAccessMask = dst_access;
	b.oldLayout = old_layout;
	b.newLayout = new_layout;
	b.image = image.get_image();
	b.subresourceRange.aspectMask = format_to_aspect_mask(image.get_create_info().format);
	b.subresourceRange.levelCount = image.get_create_info().levels;
	b.subresourceRange.layerCount = image.get_create_info().layers;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.srcStageMask = src_stages;
	b.dstStageMask = dst_stages;

	image_barriers(1, &b);
}

void CommandBuffer::buffer_barriers(uint32_t buffer_barriers, const VkBufferMemoryBarrier2 *buffers)
{
	VkDependencyInfo dep = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dep.bufferMemoryBarrierCount = buffer_barriers;
	dep.pBufferMemoryBarriers = buffers;
	barrier(dep);
}

void CommandBuffer::image_barriers(uint32_t image_barriers, const VkImageMemoryBarrier2 *images)
{
	VkDependencyInfo dep = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dep.imageMemoryBarrierCount = image_barriers;
	dep.pImageMemoryBarriers = images;
	barrier(dep);
}

void CommandBuffer::barrier_prepare_generate_mipmap(const Image &image, VkImageLayout base_level_layout,
                                                    VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
                                                    bool need_top_level_barrier)
{
	auto &create_info = image.get_create_info();
	VkImageMemoryBarrier2 barriers[2] = {};
	VK_ASSERT(create_info.levels > 1);
	(void)create_info;

	for (unsigned i = 0; i < 2; i++)
	{
		barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barriers[i].image = image.get_image();
		barriers[i].subresourceRange.aspectMask = format_to_aspect_mask(image.get_format());
		barriers[i].subresourceRange.layerCount = image.get_create_info().layers;
		barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].srcStageMask = src_stage;
		barriers[i].dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		if (i == 0)
		{
			barriers[i].oldLayout = base_level_layout;
			barriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barriers[i].srcAccessMask = src_access;
			barriers[i].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barriers[i].subresourceRange.baseMipLevel = 0;
			barriers[i].subresourceRange.levelCount = 1;
		}
		else
		{
			barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barriers[i].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barriers[i].srcAccessMask = 0;
			barriers[i].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barriers[i].subresourceRange.baseMipLevel = 1;
			barriers[i].subresourceRange.levelCount = image.get_create_info().levels - 1;
		}
	}

	image_barriers(need_top_level_barrier ? 2 : 1, need_top_level_barrier ? barriers : barriers + 1);
}

void CommandBuffer::generate_mipmap(const Image &image)
{
	auto &create_info = image.get_create_info();
	VkOffset3D size = { int(create_info.width), int(create_info.height), int(create_info.depth) };
	const VkOffset3D origin = { 0, 0, 0 };

	VK_ASSERT(image.get_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkImageMemoryBarrier2 b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	b.image = image.get_image();
	b.subresourceRange.levelCount = 1;
	b.subresourceRange.layerCount = image.get_create_info().layers;
	b.subresourceRange.aspectMask = format_to_aspect_mask(image.get_format());
	b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	b.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

	for (unsigned i = 1; i < create_info.levels; i++)
	{
		VkOffset3D src_size = size;
		size.x = std::max(size.x >> 1, 1);
		size.y = std::max(size.y >> 1, 1);
		size.z = std::max(size.z >> 1, 1);

		blit_image(image, image,
		           origin, size, origin, src_size, i, i - 1, 0, 0, create_info.layers, VK_FILTER_LINEAR);

		b.subresourceRange.baseMipLevel = i;
		image_barriers(1, &b);
	}
}

void CommandBuffer::blit_image(const Image &dst, const Image &src,
                               const VkOffset3D &dst_offset,
                               const VkOffset3D &dst_extent, const VkOffset3D &src_offset, const VkOffset3D &src_extent,
                               unsigned dst_level, unsigned src_level, unsigned dst_base_layer, unsigned src_base_layer,
                               unsigned num_layers, VkFilter filter)
{
	const auto add_offset = [](const VkOffset3D &a, const VkOffset3D &b) -> VkOffset3D {
		return { a.x + b.x, a.y + b.y, a.z + b.z };
	};

	const VkImageBlit blit = {
		{ format_to_aspect_mask(src.get_create_info().format), src_level, src_base_layer, num_layers },
		{ src_offset, add_offset(src_offset, src_extent) },
		{ format_to_aspect_mask(dst.get_create_info().format), dst_level, dst_base_layer, num_layers },
		{ dst_offset, add_offset(dst_offset, dst_extent) },
	};

	table.vkCmdBlitImage(cmd,
	                     src.get_image(), src.get_layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
	                     dst.get_image(), dst.get_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
	                     1, &blit, filter);
}

void CommandBuffer::begin_context()
{
	dirty = ~0u;
	dirty_sets = ~0u;
	dirty_vbos = ~0u;
	current_pipeline = {};
	current_pipeline_layout = VK_NULL_HANDLE;
	pipeline_state.layout = nullptr;
	pipeline_state.program = nullptr;
	pipeline_state.potential_static_state.spec_constant_mask = 0;
	pipeline_state.potential_static_state.internal_spec_constant_mask = 0;
	memset(bindings.cookies, 0, sizeof(bindings.cookies));
	memset(bindings.secondary_cookies, 0, sizeof(bindings.secondary_cookies));
	memset(&index_state, 0, sizeof(index_state));
	memset(vbo.buffers, 0, sizeof(vbo.buffers));

	if (debug_channel_buffer)
		set_storage_buffer(VULKAN_NUM_DESCRIPTOR_SETS - 1, VULKAN_NUM_BINDINGS - 1, *debug_channel_buffer);
}

void CommandBuffer::begin_compute()
{
	is_compute = true;
	begin_context();
}

void CommandBuffer::begin_graphics()
{
	is_compute = false;
	begin_context();

	// Vertex shaders which support prerotate are expected to include inc/prerotate.h and
	// call prerotate_fixup_clip_xy().
	if (current_framebuffer_surface_transform != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		set_surface_transform_specialization_constants();
}

void CommandBuffer::init_viewport_scissor(const RenderPassInfo &info, const Framebuffer *fb)
{
	VkRect2D rect = info.render_area;

	uint32_t fb_width = fb->get_width();
	uint32_t fb_height = fb->get_height();

	// Convert fb_width / fb_height to logical width / height if need be.
	if (surface_transform_swaps_xy(current_framebuffer_surface_transform))
		std::swap(fb_width, fb_height);

	rect.offset.x = std::min(int32_t(fb_width), rect.offset.x);
	rect.offset.y = std::min(int32_t(fb_height), rect.offset.y);
	rect.extent.width = std::min(fb_width - rect.offset.x, rect.extent.width);
	rect.extent.height = std::min(fb_height - rect.offset.y, rect.extent.height);

	viewport = {
		float(rect.offset.x), float(rect.offset.y),
		float(rect.extent.width), float(rect.extent.height),
		0.0f, 1.0f
	};
	scissor = rect;
}

CommandBufferHandle CommandBuffer::request_secondary_command_buffer(Device &device, const RenderPassInfo &info,
                                                                    unsigned thread_index, unsigned subpass)
{
	auto *fb = &device.request_framebuffer(info);
	auto cmd = device.request_secondary_command_buffer_for_thread(thread_index, fb, subpass);
	cmd->init_surface_transform(info);
	cmd->begin_graphics();

	cmd->framebuffer = fb;
	cmd->pipeline_state.compatible_render_pass = &fb->get_compatible_render_pass();
	cmd->actual_render_pass = &device.request_render_pass(info, false);

	unsigned i;
	for (i = 0; i < info.num_color_attachments; i++)
		cmd->framebuffer_attachments[i] = info.color_attachments[i];
	if (info.depth_stencil)
		cmd->framebuffer_attachments[i++] = info.depth_stencil;

	cmd->init_viewport_scissor(info, fb);
	cmd->pipeline_state.subpass_index = subpass;
	cmd->current_contents = VK_SUBPASS_CONTENTS_INLINE;

	return cmd;
}

CommandBufferHandle CommandBuffer::request_secondary_command_buffer(unsigned thread_index_, unsigned subpass_)
{
	VK_ASSERT(framebuffer);
	VK_ASSERT(!is_secondary);

	auto secondary_cmd = device->request_secondary_command_buffer_for_thread(thread_index_, framebuffer, subpass_);
	secondary_cmd->begin_graphics();

	secondary_cmd->framebuffer = framebuffer;
	secondary_cmd->pipeline_state.compatible_render_pass = pipeline_state.compatible_render_pass;
	secondary_cmd->actual_render_pass = actual_render_pass;
	memcpy(secondary_cmd->framebuffer_attachments, framebuffer_attachments, sizeof(framebuffer_attachments));

	secondary_cmd->pipeline_state.subpass_index = subpass_;
	secondary_cmd->viewport = viewport;
	secondary_cmd->scissor = scissor;
	secondary_cmd->current_contents = VK_SUBPASS_CONTENTS_INLINE;

	return secondary_cmd;
}

void CommandBuffer::submit_secondary(CommandBufferHandle secondary)
{
	VK_ASSERT(!is_secondary);
	VK_ASSERT(secondary->is_secondary);
	VK_ASSERT(pipeline_state.subpass_index == secondary->pipeline_state.subpass_index);
	VK_ASSERT(current_contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	device->submit_secondary(*this, *secondary);
}

void CommandBuffer::next_subpass(VkSubpassContents contents)
{
	VK_ASSERT(framebuffer);
	VK_ASSERT(pipeline_state.compatible_render_pass);
	VK_ASSERT(actual_render_pass);
	pipeline_state.subpass_index++;
	VK_ASSERT(pipeline_state.subpass_index < actual_render_pass->get_num_subpasses());
	table.vkCmdNextSubpass(cmd, contents);
	current_contents = contents;
	begin_graphics();
}

void CommandBuffer::set_surface_transform_specialization_constants()
{
	float transform[4];
	pipeline_state.potential_static_state.internal_spec_constant_mask = 0xf;
	build_prerotate_matrix_2x2(current_framebuffer_surface_transform, transform);
	for (unsigned i = 0; i < 4; i++)
	{
		memcpy(pipeline_state.potential_static_state.spec_constants + VULKAN_NUM_USER_SPEC_CONSTANTS,
		       transform, sizeof(transform));
	}
}

void CommandBuffer::init_surface_transform(const RenderPassInfo &info)
{
	// Validate that all prerotate state matches, unless the attachments are transient, since we don't really care,
	// and it gets messy to forward rotation state to them.
	VkSurfaceTransformFlagBitsKHR prerorate = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;
	for (unsigned i = 0; i < info.num_color_attachments; i++)
	{
		auto usage = info.color_attachments[i]->get_image().get_create_info().usage;
		if ((usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0)
		{
			auto image_prerotate = info.color_attachments[i]->get_image().get_surface_transform();
			if (prerorate == VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR)
			{
				prerorate = image_prerotate;
			}
			else if (prerorate != image_prerotate)
			{
				LOGE("Mismatch in prerotate state for color attachment %u! (%u != %u)\n",
				     i, unsigned(prerorate), unsigned(image_prerotate));
			}
		}
	}

	if (prerorate != VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR && info.depth_stencil)
	{
		auto usage = info.depth_stencil->get_image().get_create_info().usage;
		if ((usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0)
		{
			auto image_prerotate = info.depth_stencil->get_image().get_surface_transform();
			if (prerorate != image_prerotate)
				LOGE("Mismatch in prerotate state for depth-stencil! (%u != %u)\n", unsigned(prerorate), unsigned(image_prerotate));
		}
	}

	if (prerorate == VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR)
		prerorate = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	current_framebuffer_surface_transform = prerorate;
}

void CommandBuffer::begin_render_pass(const RenderPassInfo &info, VkSubpassContents contents)
{
	VK_ASSERT(!framebuffer);
	VK_ASSERT(!pipeline_state.compatible_render_pass);
	VK_ASSERT(!actual_render_pass);

	framebuffer = &device->request_framebuffer(info);
	init_surface_transform(info);
	pipeline_state.compatible_render_pass = &framebuffer->get_compatible_render_pass();
	actual_render_pass = &device->request_render_pass(info, false);
	pipeline_state.subpass_index = 0;
	framebuffer_is_multiview = info.num_layers > 1;

	memset(framebuffer_attachments, 0, sizeof(framebuffer_attachments));
	unsigned att;
	for (att = 0; att < info.num_color_attachments; att++)
		framebuffer_attachments[att] = info.color_attachments[att];
	if (info.depth_stencil)
		framebuffer_attachments[att++] = info.depth_stencil;

	init_viewport_scissor(info, framebuffer);

	VkClearValue clear_values[VULKAN_NUM_ATTACHMENTS + 1];
	unsigned num_clear_values = 0;

	for (unsigned i = 0; i < info.num_color_attachments; i++)
	{
		VK_ASSERT(info.color_attachments[i]);
		if (info.clear_attachments & (1u << i))
		{
			clear_values[i].color = info.clear_color[i];
			num_clear_values = i + 1;
		}

		if (info.color_attachments[i]->get_image().is_swapchain_image())
			swapchain_touch_in_stages(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	if (info.depth_stencil && (info.op_flags & RENDER_PASS_OP_CLEAR_DEPTH_STENCIL_BIT) != 0)
	{
		clear_values[info.num_color_attachments].depthStencil = info.clear_depth_stencil;
		num_clear_values = info.num_color_attachments + 1;
	}

	VkRenderPassBeginInfo begin_info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	begin_info.renderPass = actual_render_pass->get_render_pass();
	begin_info.framebuffer = framebuffer->get_framebuffer();
	begin_info.renderArea = scissor;
	begin_info.clearValueCount = num_clear_values;
	begin_info.pClearValues = clear_values;

	// In the render pass interface, we pretend we are rendering with normal
	// un-rotated coordinates.
	rect2d_transform_xy(begin_info.renderArea, current_framebuffer_surface_transform,
	                    framebuffer->get_width(), framebuffer->get_height());

	table.vkCmdBeginRenderPass(cmd, &begin_info, contents);

	current_contents = contents;
	begin_graphics();
}

void CommandBuffer::end_render_pass()
{
	VK_ASSERT(framebuffer);
	VK_ASSERT(actual_render_pass);
	VK_ASSERT(pipeline_state.compatible_render_pass);

	table.vkCmdEndRenderPass(cmd);

	framebuffer = nullptr;
	actual_render_pass = nullptr;
	pipeline_state.compatible_render_pass = nullptr;
	begin_compute();
}

static void log_compile_time(const char *tag, Hash hash,
                             int64_t time_ns, VkResult result,
                             CommandBuffer::CompileMode mode)
{
	bool stall = time_ns >= 5 * 1000 * 1000 && mode != CommandBuffer::CompileMode::AsyncThread;
#ifndef VULKAN_DEBUG
	// If a compile takes more than 5 ms and it's not happening on an async thread,
	// we consider it a stall.
	if (stall)
#endif
	{
		double time_us = 1e-3 * double(time_ns);
		const char *mode_str;

		switch (mode)
		{
		case CommandBuffer::CompileMode::Sync:
			mode_str = "sync";
			break;

		case CommandBuffer::CompileMode::FailOnCompileRequired:
			mode_str = "fail-on-compile-required";
			break;

		default:
			mode_str = "async-thread";
			break;
		}

#ifdef VULKAN_DEBUG
		if (!stall)
		{
			LOGI("Compile (%s, %016llx): thread %u - %.3f us (mode: %s, success: %s).\n",
			     tag, static_cast<unsigned long long>(hash),
			     get_current_thread_index(),
			     time_us, mode_str, result == VK_SUCCESS ? "yes" : "no");
		}
		else
#endif
		{
			LOGW("Stalled compile (%s, %016llx): thread %u - %.3f us (mode: %s, success: %s).\n",
			     tag, static_cast<unsigned long long>(hash),
			     get_current_thread_index(),
			     time_us, mode_str, result == VK_SUCCESS ? "yes" : "no");
		}
	}
}

Pipeline CommandBuffer::build_compute_pipeline(Device *device, const DeferredPipelineCompile &compile,
                                               CompileMode mode)
{
	// This can be called from outside a CommandBuffer content, so need to hold lock.
	Util::RWSpinLockReadHolder holder{device->lock.read_only_cache};

	// If we don't have pipeline creation cache control feature,
	// we must assume compilation can be synchronous.
	if (mode == CompileMode::FailOnCompileRequired &&
	    (device->get_workarounds().broken_pipeline_cache_control ||
	     !device->get_device_features().pipeline_creation_cache_control_features.pipelineCreationCacheControl))
	{
		return {};
	}

	auto &shader = *compile.program->get_shader(ShaderStage::Compute);
	VkComputePipelineCreateInfo info = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	info.layout = compile.layout->get_layout();
	info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.module = shader.get_module();
	info.stage.pName = "main";
	info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

	VkSpecializationInfo spec_info = {};
	VkSpecializationMapEntry spec_entries[VULKAN_NUM_TOTAL_SPEC_CONSTANTS];
	auto mask = compile.layout->get_resource_layout().combined_spec_constant_mask &
	            get_combined_spec_constant_mask(compile);

	uint32_t spec_constants[VULKAN_NUM_TOTAL_SPEC_CONSTANTS];

	if (mask)
	{
		info.stage.pSpecializationInfo = &spec_info;
		spec_info.pData = spec_constants;
		spec_info.pMapEntries = spec_entries;

		for_each_bit(mask, [&](uint32_t bit) {
			auto &entry = spec_entries[spec_info.mapEntryCount];
			entry.offset = sizeof(uint32_t) * spec_info.mapEntryCount;
			entry.size = sizeof(uint32_t);
			entry.constantID = bit;

			spec_constants[spec_info.mapEntryCount] = compile.potential_static_state.spec_constants[bit];
			spec_info.mapEntryCount++;
		});
		spec_info.dataSize = spec_info.mapEntryCount * sizeof(uint32_t);
	}

	VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_info;

	if (compile.static_state.state.subgroup_control_size)
	{
		if (!setup_subgroup_size_control(*device, info.stage, subgroup_size_info,
		                                 VK_SHADER_STAGE_COMPUTE_BIT,
		                                 compile.static_state.state.subgroup_full_group,
		                                 compile.static_state.state.subgroup_minimum_size_log2,
		                                 compile.static_state.state.subgroup_maximum_size_log2))
		{
			LOGE("Subgroup size configuration not supported.\n");
			return {};
		}
	}

	VkPipeline compute_pipeline = VK_NULL_HANDLE;
#ifdef GRANITE_VULKAN_FOSSILIZE
	device->register_compute_pipeline(compile.hash, info);
#endif

	auto &table = device->get_device_table();

	if (mode == CompileMode::FailOnCompileRequired)
		info.flags |= VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT;

	auto start_ts = Util::get_current_time_nsecs();
	VkResult vr = table.vkCreateComputePipelines(device->get_device(), compile.cache, 1, &info, nullptr, &compute_pipeline);
	auto end_ts = Util::get_current_time_nsecs();
	log_compile_time("compute", compile.hash, end_ts - start_ts, vr, mode);

	if (vr != VK_SUCCESS || compute_pipeline == VK_NULL_HANDLE)
	{
		if (vr < 0)
			LOGE("Failed to create compute pipeline!\n");
		return {};
	}

	auto returned_pipeline = compile.program->add_pipeline(compile.hash, { compute_pipeline, 0 });
	if (returned_pipeline.pipeline != compute_pipeline)
		table.vkDestroyPipeline(device->get_device(), compute_pipeline, nullptr);
	return returned_pipeline;
}

void CommandBuffer::extract_pipeline_state(DeferredPipelineCompile &compile) const
{
	compile = pipeline_state;

	if (!compile.program)
	{
		LOGE("Attempting to extract pipeline state when no program is bound.\n");
		return;
	}

	if (is_compute)
		update_hash_compute_pipeline(compile);
	else
		update_hash_graphics_pipeline(compile, CompileMode::AsyncThread, nullptr);
}

bool CommandBuffer::setup_subgroup_size_control(
		Vulkan::Device &device, VkPipelineShaderStageCreateInfo &stage_info,
		VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT &required_info,
		VkShaderStageFlagBits stage, bool full_group,
		unsigned min_size_log2, unsigned max_size_log2)
{
	if (!device.supports_subgroup_size_log2(full_group, min_size_log2, max_size_log2, stage))
		return false;

	auto &features = device.get_device_features();

	if (full_group)
		stage_info.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT_EXT;

	uint32_t min_subgroups = 1u << min_size_log2;
	uint32_t max_subgroups = 1u << max_size_log2;
	if (min_subgroups <= features.subgroup_size_control_properties.minSubgroupSize &&
	    max_subgroups >= features.subgroup_size_control_properties.maxSubgroupSize)
	{
		stage_info.flags |= VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT;
	}
	else
	{
		required_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT };

		// Pick a fixed subgroup size. Prefer smallest subgroup size.
		if (min_subgroups < features.subgroup_size_control_properties.minSubgroupSize)
			required_info.requiredSubgroupSize = features.subgroup_size_control_properties.minSubgroupSize;
		else
			required_info.requiredSubgroupSize = min_subgroups;

		required_info.pNext = const_cast<void *>(stage_info.pNext);
		stage_info.pNext = &required_info;
	}

	return true;
}

Pipeline CommandBuffer::build_graphics_pipeline(Device *device, const DeferredPipelineCompile &compile,
                                                CompileMode mode)
{
	// This can be called from outside a CommandBuffer content, so need to hold lock.
	Util::RWSpinLockReadHolder holder{device->lock.read_only_cache};

	// If we don't have pipeline creation cache control feature,
	// we must assume compilation can be synchronous.
	if (mode == CompileMode::FailOnCompileRequired &&
	    (device->get_workarounds().broken_pipeline_cache_control ||
	     !device->get_device_features().pipeline_creation_cache_control_features.pipelineCreationCacheControl))
	{
		return {};
	}

	// Unsupported. Gets pretty complicated since if any dependent pipeline fails, we have to abort.
	if (mode == CompileMode::FailOnCompileRequired && !compile.program_group.empty())
		return {};

	// Viewport state
	VkPipelineViewportStateCreateInfo vp = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	vp.viewportCount = 1;
	vp.scissorCount = 1;

	// Dynamic state
	VkPipelineDynamicStateCreateInfo dyn = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dyn.dynamicStateCount = 2;
	VkDynamicState states[7] = {
		VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT,
	};
	dyn.pDynamicStates = states;

	uint32_t dynamic_mask = COMMAND_BUFFER_DIRTY_VIEWPORT_BIT | COMMAND_BUFFER_DIRTY_SCISSOR_BIT;

	if (compile.static_state.state.depth_bias_enable)
	{
		states[dyn.dynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
		dynamic_mask |= COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT;
	}

	if (compile.static_state.state.stencil_test)
	{
		states[dyn.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
		states[dyn.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
		states[dyn.dynamicStateCount++] = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
		dynamic_mask |= COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT;
	}

	// Blend state
	VkPipelineColorBlendAttachmentState blend_attachments[VULKAN_NUM_ATTACHMENTS];
	VkPipelineColorBlendStateCreateInfo blend = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	blend.attachmentCount = compile.compatible_render_pass->get_num_color_attachments(compile.subpass_index);
	blend.pAttachments = blend_attachments;
	for (unsigned i = 0; i < blend.attachmentCount; i++)
	{
		auto &att = blend_attachments[i];
		att = {};

		if (compile.compatible_render_pass->get_color_attachment(compile.subpass_index, i).attachment != VK_ATTACHMENT_UNUSED &&
			(compile.layout->get_resource_layout().render_target_mask & (1u << i)))
		{
			att.colorWriteMask = (compile.static_state.state.write_mask >> (4 * i)) & 0xf;
			att.blendEnable = compile.static_state.state.blend_enable;
			if (att.blendEnable)
			{
				att.alphaBlendOp = static_cast<VkBlendOp>(compile.static_state.state.alpha_blend_op);
				att.colorBlendOp = static_cast<VkBlendOp>(compile.static_state.state.color_blend_op);
				att.dstAlphaBlendFactor = static_cast<VkBlendFactor>(compile.static_state.state.dst_alpha_blend);
				att.srcAlphaBlendFactor = static_cast<VkBlendFactor>(compile.static_state.state.src_alpha_blend);
				att.dstColorBlendFactor = static_cast<VkBlendFactor>(compile.static_state.state.dst_color_blend);
				att.srcColorBlendFactor = static_cast<VkBlendFactor>(compile.static_state.state.src_color_blend);
			}
		}
	}
	memcpy(blend.blendConstants, compile.potential_static_state.blend_constants, sizeof(blend.blendConstants));

	// Depth state
	VkPipelineDepthStencilStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	ds.stencilTestEnable = compile.compatible_render_pass->has_stencil(compile.subpass_index) && compile.static_state.state.stencil_test;
	ds.depthTestEnable = compile.compatible_render_pass->has_depth(compile.subpass_index) && compile.static_state.state.depth_test;
	ds.depthWriteEnable = compile.compatible_render_pass->has_depth(compile.subpass_index) && compile.static_state.state.depth_write;

	if (ds.depthTestEnable)
		ds.depthCompareOp = static_cast<VkCompareOp>(compile.static_state.state.depth_compare);

	if (ds.stencilTestEnable)
	{
		ds.front.compareOp = static_cast<VkCompareOp>(compile.static_state.state.stencil_front_compare_op);
		ds.front.passOp = static_cast<VkStencilOp>(compile.static_state.state.stencil_front_pass);
		ds.front.failOp = static_cast<VkStencilOp>(compile.static_state.state.stencil_front_fail);
		ds.front.depthFailOp = static_cast<VkStencilOp>(compile.static_state.state.stencil_front_depth_fail);
		ds.back.compareOp = static_cast<VkCompareOp>(compile.static_state.state.stencil_back_compare_op);
		ds.back.passOp = static_cast<VkStencilOp>(compile.static_state.state.stencil_back_pass);
		ds.back.failOp = static_cast<VkStencilOp>(compile.static_state.state.stencil_back_fail);
		ds.back.depthFailOp = static_cast<VkStencilOp>(compile.static_state.state.stencil_back_depth_fail);
	}

	// Vertex input
	VkPipelineVertexInputStateCreateInfo vi = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	VkVertexInputAttributeDescription vi_attribs[VULKAN_NUM_VERTEX_ATTRIBS];
	VkVertexInputBindingDescription vi_bindings[VULKAN_NUM_VERTEX_BUFFERS];

	if (compile.program->get_shader(ShaderStage::Vertex))
	{
		vi.pVertexAttributeDescriptions = vi_attribs;
		uint32_t attr_mask = compile.layout->get_resource_layout().attribute_mask;
		uint32_t binding_mask = 0;
		for_each_bit(attr_mask, [&](uint32_t bit) {
			auto &attr = vi_attribs[vi.vertexAttributeDescriptionCount++];
			attr.location = bit;
			attr.binding = compile.attribs[bit].binding;
			attr.format = compile.attribs[bit].format;
			attr.offset = compile.attribs[bit].offset;
			binding_mask |= 1u << attr.binding;
		});

		vi.pVertexBindingDescriptions = vi_bindings;
		for_each_bit(binding_mask, [&](uint32_t bit) {
			auto &bind = vi_bindings[vi.vertexBindingDescriptionCount++];
			bind.binding = bit;
			bind.inputRate = compile.input_rates[bit];
			bind.stride = compile.strides[bit];
		});
	}

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo ia = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	ia.primitiveRestartEnable = compile.static_state.state.primitive_restart;
	ia.topology = static_cast<VkPrimitiveTopology>(compile.static_state.state.topology);

	// Multisample
	VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	ms.rasterizationSamples = static_cast<VkSampleCountFlagBits>(compile.compatible_render_pass->get_sample_count(compile.subpass_index));

	if (compile.compatible_render_pass->get_sample_count(compile.subpass_index) > 1)
	{
		ms.alphaToCoverageEnable = compile.static_state.state.alpha_to_coverage;
		ms.alphaToOneEnable = compile.static_state.state.alpha_to_one;
		ms.sampleShadingEnable = compile.static_state.state.sample_shading;
		ms.minSampleShading = 1.0f;
	}

	// Raster
	VkPipelineRasterizationStateCreateInfo raster = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	raster.cullMode = static_cast<VkCullModeFlags>(compile.static_state.state.cull_mode);
	raster.frontFace = static_cast<VkFrontFace>(compile.static_state.state.front_face);
	raster.lineWidth = 1.0f;
	raster.polygonMode = compile.static_state.state.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	raster.depthBiasEnable = compile.static_state.state.depth_bias_enable != 0;

	VkPipelineRasterizationConservativeStateCreateInfoEXT conservative_raster = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT
	};
	if (compile.static_state.state.conservative_raster)
	{
		if (device->get_device_features().supports_conservative_rasterization)
		{
			raster.pNext = &conservative_raster;
			conservative_raster.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
		}
		else
		{
			LOGE("Conservative rasterization is not supported on this device.\n");
			return {};
		}
	}

	// Stages
	VkPipelineShaderStageCreateInfo stages[Util::ecast(ShaderStage::Count)];
	unsigned num_stages = 0;

	VkSpecializationInfo spec_info[ecast(ShaderStage::Count)] = {};
	VkSpecializationMapEntry spec_entries[ecast(ShaderStage::Count)][VULKAN_NUM_TOTAL_SPEC_CONSTANTS];
	uint32_t spec_constants[Util::ecast(ShaderStage::Count)][VULKAN_NUM_TOTAL_SPEC_CONSTANTS];

	VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_info_task;
	VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_info_mesh;

	for (unsigned i = 0; i < Util::ecast(ShaderStage::Count); i++)
	{
		auto mask = compile.layout->get_resource_layout().spec_constant_mask[i] &
		            get_combined_spec_constant_mask(compile);

		if (mask)
		{
			spec_info[i].pData = spec_constants[i];
			spec_info[i].pMapEntries = spec_entries[i];

			for_each_bit(mask, [&](uint32_t bit)
			{
				auto &entry = spec_entries[i][spec_info[i].mapEntryCount];
				entry.offset = sizeof(uint32_t) * spec_info[i].mapEntryCount;
				entry.size = sizeof(uint32_t);
				entry.constantID = bit;
				spec_constants[i][spec_info[i].mapEntryCount] = compile.potential_static_state.spec_constants[bit];
				spec_info[i].mapEntryCount++;
			});
			spec_info[i].dataSize = spec_info[i].mapEntryCount * sizeof(uint32_t);
		}
	}

	for (unsigned i = 0; i < Util::ecast(ShaderStage::Count); i++)
	{
		auto stage = static_cast<ShaderStage>(i);
		if (compile.program->get_shader(stage))
		{
			auto &s = stages[num_stages++];
			s = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			s.module = compile.program->get_shader(stage)->get_module();
			s.pName = "main";
			s.stage = static_cast<VkShaderStageFlagBits>(1u << i);
			if (spec_info[i].mapEntryCount)
				s.pSpecializationInfo = &spec_info[i];

			if (stage == ShaderStage::Mesh || stage == ShaderStage::Task)
			{
				VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT *required_info;
				unsigned min_size_log2, max_size_log2;
				bool size_enabled, full_group;

				if (stage == ShaderStage::Mesh)
				{
					size_enabled = compile.static_state.state.subgroup_control_size;
					full_group = compile.static_state.state.subgroup_full_group;
					min_size_log2 = compile.static_state.state.subgroup_minimum_size_log2;
					max_size_log2 = compile.static_state.state.subgroup_maximum_size_log2;
					required_info = &subgroup_size_info_mesh;
				}
				else
				{
					size_enabled = compile.static_state.state.subgroup_control_size_task;
					full_group = compile.static_state.state.subgroup_full_group_task;
					min_size_log2 = compile.static_state.state.subgroup_minimum_size_log2_task;
					max_size_log2 = compile.static_state.state.subgroup_maximum_size_log2_task;
					required_info = &subgroup_size_info_task;
				}

				if (size_enabled)
				{
					if (!setup_subgroup_size_control(
							*device, s, *required_info, s.stage,
							full_group, min_size_log2, max_size_log2))
					{
						LOGE("Subgroup size configuration not supported.\n");
						return {};
					}
				}
			}
		}
	}

	VkGraphicsPipelineCreateInfo pipe = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipe.layout = compile.layout->get_layout();
	pipe.renderPass = compile.compatible_render_pass->get_render_pass();
	pipe.subpass = compile.subpass_index;

	pipe.pViewportState = &vp;
	pipe.pDynamicState = &dyn;
	pipe.pColorBlendState = &blend;
	pipe.pDepthStencilState = &ds;
	if (compile.program->get_shader(ShaderStage::Vertex))
	{
		pipe.pVertexInputState = &vi;
		pipe.pInputAssemblyState = &ia;
	}
	pipe.pMultisampleState = &ms;
	pipe.pRasterizationState = &raster;
	pipe.pStages = stages;
	pipe.stageCount = num_stages;

	VkGraphicsPipelineShaderGroupsCreateInfoNV groups_info =
			{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_SHADER_GROUPS_CREATE_INFO_NV };
	VkGraphicsShaderGroupCreateInfoNV self_group =
			{ VK_STRUCTURE_TYPE_GRAPHICS_SHADER_GROUP_CREATE_INFO_NV };
	Util::SmallVector<VkPipeline, 64> pipelines;

	if (mode == CompileMode::IndirectBindable)
		pipe.flags |= VK_PIPELINE_CREATE_INDIRECT_BINDABLE_BIT_NV;

	if (!compile.program_group.empty())
	{
		DeferredPipelineCompile tmp_compile = compile;
		tmp_compile.program_group.clear();
		pipelines.reserve(compile.program_group.size());

		for (auto *p : compile.program_group)
		{
			tmp_compile.program = p;
			update_hash_graphics_pipeline(tmp_compile, CompileMode::IndirectBindable, nullptr);
			auto group_pipeline = p->get_pipeline(tmp_compile.hash);
			if (group_pipeline.pipeline == VK_NULL_HANDLE)
				group_pipeline = build_graphics_pipeline(device, tmp_compile, CompileMode::IndirectBindable);

			if (group_pipeline.pipeline == VK_NULL_HANDLE)
			{
				LOGE("Failed to compile group pipeline.\n");
				return {};
			}

			pipelines.push_back(group_pipeline.pipeline);
		}

		self_group.stageCount = pipe.stageCount;
		self_group.pStages = pipe.pStages;
		self_group.pVertexInputState = pipe.pVertexInputState;

		// Compile each program individually. Then we just link them.
		pipe.flags |= VK_PIPELINE_CREATE_INDIRECT_BINDABLE_BIT_NV;

		groups_info.pNext = pipe.pNext;
		pipe.pNext = &groups_info;
		// Trying to use pGroups[0] through self pPipeline reference crashes NV driver.
		groups_info.pPipelines = pipelines.data() + 1;
		groups_info.pipelineCount = uint32_t(pipelines.size() - 1);
		groups_info.pGroups = &self_group;
		groups_info.groupCount = 1;
	}

	VkPipeline pipeline = VK_NULL_HANDLE;
#ifdef GRANITE_VULKAN_FOSSILIZE
	device->register_graphics_pipeline(compile.hash, pipe);
#endif

	auto &table = device->get_device_table();

	if (mode == CompileMode::FailOnCompileRequired)
		pipe.flags |= VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT;

	auto start_ts = Util::get_current_time_nsecs();
	VkResult res = table.vkCreateGraphicsPipelines(device->get_device(), compile.cache, 1, &pipe, nullptr, &pipeline);
	auto end_ts = Util::get_current_time_nsecs();
	log_compile_time("graphics", compile.hash, end_ts - start_ts, res, mode);

	if (res != VK_SUCCESS || pipeline == VK_NULL_HANDLE)
	{
		if (res < 0)
			LOGE("Failed to create graphics pipeline!\n");
		return {};
	}

	auto returned_pipeline = compile.program->add_pipeline(compile.hash, { pipeline, dynamic_mask });
	if (returned_pipeline.pipeline != pipeline)
		table.vkDestroyPipeline(device->get_device(), pipeline, nullptr);
	return returned_pipeline;
}

bool CommandBuffer::flush_compute_pipeline(bool synchronous)
{
	update_hash_compute_pipeline(pipeline_state);
	current_pipeline = pipeline_state.program->get_pipeline(pipeline_state.hash);
	if (current_pipeline.pipeline == VK_NULL_HANDLE)
	{
		current_pipeline = build_compute_pipeline(
			device, pipeline_state,
			synchronous ? CompileMode::Sync : CompileMode::FailOnCompileRequired);
	}
	return current_pipeline.pipeline != VK_NULL_HANDLE;
}

void CommandBuffer::update_hash_compute_pipeline(DeferredPipelineCompile &compile)
{
	Hasher h;
	h.u64(compile.program->get_hash());
	h.u64(compile.layout->get_hash());

	// Spec constants.
	auto &layout = compile.layout->get_resource_layout();
	uint32_t combined_spec_constant = layout.combined_spec_constant_mask;
	combined_spec_constant &= get_combined_spec_constant_mask(compile);
	h.u32(combined_spec_constant);
	for_each_bit(combined_spec_constant, [&](uint32_t bit) {
		h.u32(compile.potential_static_state.spec_constants[bit]);
	});

	if (compile.static_state.state.subgroup_control_size)
	{
		h.s32(1);
		h.u32(compile.static_state.state.subgroup_minimum_size_log2);
		h.u32(compile.static_state.state.subgroup_maximum_size_log2);
		h.u32(compile.static_state.state.subgroup_full_group);
		// Required for Fossilize since we don't know exactly how to lower these requirements to a PSO
		// without knowing some device state.
		h.u32(compile.subgroup_size_tag);
	}
	else
		h.s32(0);

	compile.hash = h.get();
}

void CommandBuffer::update_hash_graphics_pipeline(DeferredPipelineCompile &compile,
                                                  CompileMode mode, uint32_t *out_active_vbos)
{
	Hasher h;
	uint32_t active_vbos = 0;
	auto &layout = compile.layout->get_resource_layout();
	for_each_bit(layout.attribute_mask, [&](uint32_t bit) {
		h.u32(bit);
		active_vbos |= 1u << compile.attribs[bit].binding;
		h.u32(compile.attribs[bit].binding);
		h.u32(compile.attribs[bit].format);
		h.u32(compile.attribs[bit].offset);
	});

	for_each_bit(active_vbos, [&](uint32_t bit) {
		h.u32(compile.input_rates[bit]);
		h.u32(compile.strides[bit]);
	});

	if (out_active_vbos)
		*out_active_vbos = active_vbos;

	h.u64(compile.compatible_render_pass->get_hash());
	h.u32(compile.subpass_index);
	h.u64(compile.program->get_hash());
	for (auto *p : compile.program_group)
		h.u64(p->get_hash());
	h.u64(compile.layout->get_hash());
	h.data(compile.static_state.words, sizeof(compile.static_state.words));

	if (compile.static_state.state.blend_enable)
	{
		const auto needs_blend_constant = [](VkBlendFactor factor) {
			return factor == VK_BLEND_FACTOR_CONSTANT_COLOR || factor == VK_BLEND_FACTOR_CONSTANT_ALPHA;
		};
		bool b0 = needs_blend_constant(static_cast<VkBlendFactor>(compile.static_state.state.src_color_blend));
		bool b1 = needs_blend_constant(static_cast<VkBlendFactor>(compile.static_state.state.src_alpha_blend));
		bool b2 = needs_blend_constant(static_cast<VkBlendFactor>(compile.static_state.state.dst_color_blend));
		bool b3 = needs_blend_constant(static_cast<VkBlendFactor>(compile.static_state.state.dst_alpha_blend));
		if (b0 || b1 || b2 || b3)
			h.data(reinterpret_cast<const uint32_t *>(compile.potential_static_state.blend_constants),
			       sizeof(compile.potential_static_state.blend_constants));
	}

	// Spec constants.
	uint32_t combined_spec_constant = layout.combined_spec_constant_mask;
	combined_spec_constant &= get_combined_spec_constant_mask(compile);
	h.u32(combined_spec_constant);
	for_each_bit(combined_spec_constant, [&](uint32_t bit) {
		h.u32(compile.potential_static_state.spec_constants[bit]);
	});
	h.s32(mode == CompileMode::IndirectBindable);

	if (compile.program->get_shader(ShaderStage::Task))
	{
		if (compile.static_state.state.subgroup_control_size_task)
		{
			h.s32(1);
			h.u32(compile.static_state.state.subgroup_minimum_size_log2_task);
			h.u32(compile.static_state.state.subgroup_maximum_size_log2_task);
			h.u32(compile.static_state.state.subgroup_full_group_task);
			// Required for Fossilize since we don't know exactly how to lower these requirements to a PSO
			// without knowing some device state.
			h.u32(compile.subgroup_size_tag);
		}
		else
			h.s32(0);
	}

	if (compile.program->get_shader(ShaderStage::Mesh))
	{
		if (compile.static_state.state.subgroup_control_size)
		{
			h.s32(1);
			h.u32(compile.static_state.state.subgroup_minimum_size_log2);
			h.u32(compile.static_state.state.subgroup_maximum_size_log2);
			h.u32(compile.static_state.state.subgroup_full_group);
			// Required for Fossilize since we don't know exactly how to lower these requirements to a PSO
			// without knowing some device state.
			h.u32(compile.subgroup_size_tag);
		}
		else
			h.s32(0);
	}

	compile.hash = h.get();
}

bool CommandBuffer::flush_graphics_pipeline(bool synchronous)
{
	auto mode = synchronous ? CompileMode::Sync : CompileMode::FailOnCompileRequired;
	update_hash_graphics_pipeline(pipeline_state, mode, &active_vbos);
	current_pipeline = pipeline_state.program->get_pipeline(pipeline_state.hash);
	if (current_pipeline.pipeline == VK_NULL_HANDLE)
		current_pipeline = build_graphics_pipeline(device, pipeline_state, mode);
	return current_pipeline.pipeline != VK_NULL_HANDLE;
}

void CommandBuffer::bind_pipeline(VkPipelineBindPoint bind_point, VkPipeline pipeline, uint32_t active_dynamic_state)
{
	table.vkCmdBindPipeline(cmd, bind_point, pipeline);

	// If some dynamic state is static in the pipeline it clobbers the dynamic state.
	// As a performance optimization don't clobber everything.
	uint32_t static_state_clobber = ~active_dynamic_state & COMMAND_BUFFER_DYNAMIC_BITS;
	set_dirty(static_state_clobber);
}

VkPipeline CommandBuffer::flush_compute_state(bool synchronous)
{
	if (!pipeline_state.program)
		return VK_NULL_HANDLE;
	VK_ASSERT(pipeline_state.layout);

	if (current_pipeline.pipeline == VK_NULL_HANDLE)
		set_dirty(COMMAND_BUFFER_DIRTY_PIPELINE_BIT);

	if (get_and_clear(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT | COMMAND_BUFFER_DIRTY_PIPELINE_BIT))
	{
		VkPipeline old_pipe = current_pipeline.pipeline;
		if (!flush_compute_pipeline(synchronous))
			return VK_NULL_HANDLE;

		if (old_pipe != current_pipeline.pipeline)
			bind_pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, current_pipeline.pipeline, current_pipeline.dynamic_mask);
	}

	if (current_pipeline.pipeline == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	flush_descriptor_sets();

	if (get_and_clear(COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT))
	{
		auto &range = pipeline_state.layout->get_resource_layout().push_constant_range;
		if (range.stageFlags != 0)
		{
			VK_ASSERT(range.offset == 0);
			table.vkCmdPushConstants(cmd, current_pipeline_layout, range.stageFlags,
			                         0, range.size,
			                         bindings.push_constant_data);
		}
	}

	return current_pipeline.pipeline;
}

VkPipeline CommandBuffer::flush_render_state(bool synchronous)
{
	if (!pipeline_state.program)
		return VK_NULL_HANDLE;
	VK_ASSERT(pipeline_state.layout);

	if (current_pipeline.pipeline == VK_NULL_HANDLE)
		set_dirty(COMMAND_BUFFER_DIRTY_PIPELINE_BIT);

	// We've invalidated pipeline state, update the VkPipeline.
	if (get_and_clear(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT | COMMAND_BUFFER_DIRTY_PIPELINE_BIT |
	                  COMMAND_BUFFER_DIRTY_STATIC_VERTEX_BIT))
	{
		VkPipeline old_pipe = current_pipeline.pipeline;
		if (!flush_graphics_pipeline(synchronous))
			return VK_NULL_HANDLE;

		if (old_pipe != current_pipeline.pipeline)
			bind_pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline.pipeline, current_pipeline.dynamic_mask);

#ifdef VULKAN_DEBUG
		if (current_framebuffer_surface_transform != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			// Make sure that if we're using prerotate, our vertex shaders have prerotate.
			auto spec_constant_mask = pipeline_state.layout->get_resource_layout().combined_spec_constant_mask;
			constexpr uint32_t expected_mask = 0xfu << VULKAN_NUM_USER_SPEC_CONSTANTS;
			VK_ASSERT((spec_constant_mask & expected_mask) == expected_mask);
		}
#endif
	}

	if (current_pipeline.pipeline == VK_NULL_HANDLE)
		return VK_NULL_HANDLE;

	flush_descriptor_sets();

	if (get_and_clear(COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT))
	{
		auto &range = pipeline_state.layout->get_resource_layout().push_constant_range;
		if (range.stageFlags != 0)
		{
			VK_ASSERT(range.offset == 0);
			table.vkCmdPushConstants(cmd, current_pipeline_layout, range.stageFlags,
			                         0, range.size,
			                         bindings.push_constant_data);
		}
	}

	if (get_and_clear(COMMAND_BUFFER_DIRTY_VIEWPORT_BIT))
	{
		if (current_framebuffer_surface_transform != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			auto tmp_viewport = viewport;
			viewport_transform_xy(tmp_viewport, current_framebuffer_surface_transform,
			                      framebuffer->get_width(), framebuffer->get_height());
			table.vkCmdSetViewport(cmd, 0, 1, &tmp_viewport);
		}
		else
			table.vkCmdSetViewport(cmd, 0, 1, &viewport);
	}

	if (get_and_clear(COMMAND_BUFFER_DIRTY_SCISSOR_BIT))
	{
		auto tmp_scissor = scissor;
		rect2d_transform_xy(tmp_scissor, current_framebuffer_surface_transform,
							framebuffer->get_width(), framebuffer->get_height());
		rect2d_clip(tmp_scissor);
		table.vkCmdSetScissor(cmd, 0, 1, &tmp_scissor);
	}

	if (pipeline_state.static_state.state.depth_bias_enable && get_and_clear(COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT))
		table.vkCmdSetDepthBias(cmd, dynamic_state.depth_bias_constant, 0.0f, dynamic_state.depth_bias_slope);
	if (pipeline_state.static_state.state.stencil_test && get_and_clear(COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT))
	{
		table.vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_FRONT_BIT, dynamic_state.front_compare_mask);
		table.vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_BIT, dynamic_state.front_reference);
		table.vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_FRONT_BIT, dynamic_state.front_write_mask);
		table.vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_BACK_BIT, dynamic_state.back_compare_mask);
		table.vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_BACK_BIT, dynamic_state.back_reference);
		table.vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_BACK_BIT, dynamic_state.back_write_mask);
	}

	uint32_t update_vbo_mask = dirty_vbos & active_vbos;
	for_each_bit_range(update_vbo_mask, [&](uint32_t binding, uint32_t binding_count) {
#ifdef VULKAN_DEBUG
		for (unsigned i = binding; i < binding + binding_count; i++)
			VK_ASSERT(vbo.buffers[i] != VK_NULL_HANDLE);
#endif
		table.vkCmdBindVertexBuffers(cmd, binding, binding_count, vbo.buffers + binding, vbo.offsets + binding);
	});
	dirty_vbos &= ~update_vbo_mask;

	return current_pipeline.pipeline;
}

bool CommandBuffer::flush_pipeline_state_without_blocking()
{
	if (is_compute)
		return flush_compute_state(false) != VK_NULL_HANDLE;
	else
		return flush_render_state(false) != VK_NULL_HANDLE;
}

VkPipeline CommandBuffer::get_current_compute_pipeline()
{
	return flush_compute_state(true);
}

VkPipeline CommandBuffer::get_current_graphics_pipeline()
{
	return flush_render_state(true);
}

void CommandBuffer::wait_events(uint32_t count, const PipelineEvent *events, const VkDependencyInfo *deps)
{
	VK_ASSERT(!framebuffer);
	VK_ASSERT(!actual_render_pass);

	Util::SmallVector<VkEvent> vk_events;
	vk_events.reserve(count);
	for (uint32_t i = 0; i < count; i++)
		vk_events.push_back(events[i]->get_event());

	if (device->get_workarounds().emulate_event_as_pipeline_barrier)
	{
		for (uint32_t i = 0; i < count; i++)
			barrier(deps[i]);
	}
	else if (device->get_device_features().sync2_features.synchronization2)
	{
		table.vkCmdWaitEvents2KHR(cmd, count, vk_events.data(), deps);
	}
	else
	{
		Sync1CompatData sync1;
		for (uint32_t i = 0; i < count; i++)
			convert_vk_dependency_info(deps[i], sync1);
		table.vkCmdWaitEvents(cmd, count, vk_events.data(),
		                      sync1.src_stages, sync1.dst_stages,
		                      uint32_t(sync1.mem_barriers.size()), sync1.mem_barriers.data(),
		                      uint32_t(sync1.buf_barriers.size()), sync1.buf_barriers.data(),
		                      uint32_t(sync1.img_barriers.size()), sync1.img_barriers.data());
	}
}

PipelineEvent CommandBuffer::signal_event(const VkDependencyInfo &dep)
{
	VK_ASSERT(!framebuffer);
	VK_ASSERT(!actual_render_pass);
	auto event = device->begin_signal_event();

	if (!device->get_workarounds().emulate_event_as_pipeline_barrier)
	{
		if (device->get_device_features().sync2_features.synchronization2)
		{
			table.vkCmdSetEvent2KHR(cmd, event->get_event(), &dep);
		}
		else
		{
			Sync1CompatData sync1;
			convert_vk_dependency_info(dep, sync1);
			table.vkCmdSetEvent(cmd, event->get_event(), sync1.src_stages);
		}
	}
	return event;
}

void CommandBuffer::set_vertex_attrib(uint32_t attrib, uint32_t binding, VkFormat format, VkDeviceSize offset)
{
	VK_ASSERT(attrib < VULKAN_NUM_VERTEX_ATTRIBS);
	VK_ASSERT(framebuffer);

	auto &attr = pipeline_state.attribs[attrib];

	if (attr.binding != binding || attr.format != format || attr.offset != offset)
		set_dirty(COMMAND_BUFFER_DIRTY_STATIC_VERTEX_BIT);

	VK_ASSERT(binding < VULKAN_NUM_VERTEX_BUFFERS);

	attr.binding = binding;
	attr.format = format;
	attr.offset = offset;
}

void CommandBuffer::set_index_buffer(const Buffer &buffer, VkDeviceSize offset, VkIndexType index_type)
{
	if (index_state.buffer == buffer.get_buffer() &&
	    index_state.offset == offset &&
	    index_state.index_type == index_type)
	{
		return;
	}

	index_state.buffer = buffer.get_buffer();
	index_state.offset = offset;
	index_state.index_type = index_type;
	table.vkCmdBindIndexBuffer(cmd, buffer.get_buffer(), offset, index_type);
}

void CommandBuffer::set_vertex_binding(uint32_t binding, const Buffer &buffer, VkDeviceSize offset, VkDeviceSize stride,
                                       VkVertexInputRate step_rate)
{
	VK_ASSERT(binding < VULKAN_NUM_VERTEX_BUFFERS);
	VK_ASSERT(framebuffer);

	VkBuffer vkbuffer = buffer.get_buffer();
	if (vbo.buffers[binding] != vkbuffer || vbo.offsets[binding] != offset)
		dirty_vbos |= 1u << binding;
	if (pipeline_state.strides[binding] != stride || pipeline_state.input_rates[binding] != step_rate)
		set_dirty(COMMAND_BUFFER_DIRTY_STATIC_VERTEX_BIT);

	vbo.buffers[binding] = vkbuffer;
	vbo.offsets[binding] = offset;
	pipeline_state.strides[binding] = stride;
	pipeline_state.input_rates[binding] = step_rate;
}

void CommandBuffer::set_viewport(const VkViewport &viewport_)
{
	VK_ASSERT(framebuffer);
	viewport = viewport_;
	set_dirty(COMMAND_BUFFER_DIRTY_VIEWPORT_BIT);
}

const VkViewport &CommandBuffer::get_viewport() const
{
	return viewport;
}

void CommandBuffer::set_scissor(const VkRect2D &rect)
{
	VK_ASSERT(framebuffer);
	VK_ASSERT(rect.offset.x >= 0);
	VK_ASSERT(rect.offset.y >= 0);
	scissor = rect;
	set_dirty(COMMAND_BUFFER_DIRTY_SCISSOR_BIT);
}

void CommandBuffer::push_constants(const void *data, VkDeviceSize offset, VkDeviceSize range)
{
	VK_ASSERT(offset + range <= VULKAN_PUSH_CONSTANT_SIZE);
	memcpy(bindings.push_constant_data + offset, data, range);
	set_dirty(COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT);
}

#ifdef GRANITE_VULKAN_SYSTEM_HANDLES
void CommandBuffer::set_program(const std::string &compute, const std::vector<std::pair<std::string, int>> &defines)
{
	auto *p = device->get_shader_manager().register_compute(compute);
	if (p)
	{
		auto *variant = p->register_variant(defines);
		set_program(variant->get_program());
	}
	else
		set_program(nullptr);
}

void CommandBuffer::set_program(const std::string &vertex, const std::string &fragment,
                                const std::vector<std::pair<std::string, int>> &defines)
{
	auto *p = device->get_shader_manager().register_graphics(vertex, fragment);
	if (p)
	{
		auto *variant = p->register_variant(defines);
		set_program(variant->get_program());
	}
	else
		set_program(nullptr);
}

void CommandBuffer::set_program(const std::string &task, const std::string &mesh, const std::string &fragment,
                                const std::vector<std::pair<std::string, int>> &defines)
{
	auto *p = device->get_shader_manager().register_graphics(task, mesh, fragment);
	if (p)
	{
		auto *variant = p->register_variant(defines);
		set_program(variant->get_program());
	}
	else
		set_program(nullptr);
}
#endif

void CommandBuffer::set_program_group(Program *const *programs, unsigned num_programs,
                                      const PipelineLayout *layout)
{
	pipeline_state.program = num_programs ? programs[0] : nullptr;
	pipeline_state.program_group = { programs, programs + num_programs };
	current_pipeline = {};
	set_dirty(COMMAND_BUFFER_DIRTY_PIPELINE_BIT);

	VK_ASSERT(device->get_device_features().device_generated_commands_features.deviceGeneratedCommands);

	if (!num_programs)
		return;

	VK_ASSERT(framebuffer && pipeline_state.program->get_shader(ShaderStage::Fragment));
#ifdef VULKAN_DEBUG
	for (unsigned i = 0; i < num_programs; i++)
		VK_ASSERT(pipeline_state.program_group[i]->get_shader(ShaderStage::Fragment));
#endif

	if (!layout && pipeline_state.program)
	{
		CombinedResourceLayout combined_layout = programs[0]->get_pipeline_layout()->get_resource_layout();
		for (unsigned i = 1; i < num_programs; i++)
			device->merge_combined_resource_layout(combined_layout, *programs[i]);
		layout = device->request_pipeline_layout(combined_layout, nullptr);
	}

	set_program_layout(layout);
}

void CommandBuffer::set_program(Program *program)
{
	if (pipeline_state.program == program)
		return;

	pipeline_state.program = program;
	pipeline_state.program_group.clear();
	current_pipeline = {};

	set_dirty(COMMAND_BUFFER_DIRTY_PIPELINE_BIT);
	if (!program)
		return;

	VK_ASSERT((framebuffer && pipeline_state.program->get_shader(ShaderStage::Fragment)) ||
	          (!framebuffer && pipeline_state.program->get_shader(ShaderStage::Compute)));

	set_program_layout(program->get_pipeline_layout());
}

void CommandBuffer::set_program_layout(const PipelineLayout *layout)
{
	VK_ASSERT(layout);
	if (!pipeline_state.layout)
	{
		dirty_sets = ~0u;
		set_dirty(COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT);
	}
	else if (layout->get_hash() != pipeline_state.layout->get_hash())
	{
		auto &new_layout = layout->get_resource_layout();
		auto &old_layout = pipeline_state.layout->get_resource_layout();

		// If the push constant layout changes, all descriptor sets
		// are invalidated.
		if (new_layout.push_constant_layout_hash != old_layout.push_constant_layout_hash)
		{
			dirty_sets = ~0u;
			set_dirty(COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT);
		}
		else
		{
			// Find the first set whose descriptor set layout differs.
			for (unsigned set = 0; set < VULKAN_NUM_DESCRIPTOR_SETS; set++)
			{
				if (layout->get_allocator(set) != pipeline_state.layout->get_allocator(set))
				{
					dirty_sets |= ~((1u << set) - 1);
					break;
				}
			}
		}
	}

	pipeline_state.layout = layout;
	current_pipeline_layout = pipeline_state.layout->get_layout();
}

void *CommandBuffer::allocate_constant_data(unsigned set, unsigned binding, VkDeviceSize size)
{
	VK_ASSERT(size <= VULKAN_MAX_UBO_SIZE);
	auto data = ubo_block.allocate(size);
	if (!data.host)
	{
		device->request_uniform_block(ubo_block, size);
		data = ubo_block.allocate(size);
	}
	set_uniform_buffer(set, binding, *ubo_block.gpu, data.offset, data.padded_size);
	return data.host;
}

void *CommandBuffer::allocate_index_data(VkDeviceSize size, VkIndexType index_type)
{
	auto data = ibo_block.allocate(size);
	if (!data.host)
	{
		device->request_index_block(ibo_block, size);
		data = ibo_block.allocate(size);
	}
	set_index_buffer(*ibo_block.gpu, data.offset, index_type);
	return data.host;
}

void *CommandBuffer::update_buffer(const Buffer &buffer, VkDeviceSize offset, VkDeviceSize size)
{
	if (size == 0)
		return nullptr;

	auto data = staging_block.allocate(size);
	if (!data.host)
	{
		device->request_staging_block(staging_block, size);
		data = staging_block.allocate(size);
	}
	copy_buffer(buffer, offset, *staging_block.cpu, data.offset, size);
	return data.host;
}

void *CommandBuffer::update_image(const Image &image, const VkOffset3D &offset, const VkExtent3D &extent,
                                  uint32_t row_length, uint32_t image_height,
                                  const VkImageSubresourceLayers &subresource)
{
	auto &create_info = image.get_create_info();
	uint32_t width = image.get_width(subresource.mipLevel);
	uint32_t height = image.get_height(subresource.mipLevel);
	uint32_t depth = image.get_depth(subresource.mipLevel);

	if (!row_length)
		row_length = width;

	if (!image_height)
		image_height = height;

	if ((subresource.aspectMask & (VK_IMAGE_ASPECT_PLANE_0_BIT |
	                               VK_IMAGE_ASPECT_PLANE_1_BIT |
	                               VK_IMAGE_ASPECT_PLANE_2_BIT)) != 0)
	{
		format_ycbcr_downsample_dimensions(create_info.format, subresource.aspectMask, row_length, image_height);
	}

	uint32_t blocks_x = row_length;
	uint32_t blocks_y = image_height;
	format_num_blocks(create_info.format, blocks_x, blocks_y);

	VkDeviceSize size =
	    TextureFormatLayout::format_block_size(create_info.format, subresource.aspectMask) * subresource.layerCount * depth * blocks_x * blocks_y;

	auto data = staging_block.allocate(size);
	if (!data.host)
	{
		device->request_staging_block(staging_block, size);
		data = staging_block.allocate(size);
	}

	copy_buffer_to_image(image, *staging_block.cpu, data.offset, offset, extent, row_length, image_height, subresource);
	return data.host;
}

void *CommandBuffer::update_image(const Image &image, uint32_t row_length, uint32_t image_height)
{
	const VkImageSubresourceLayers subresource = {
		format_to_aspect_mask(image.get_format()), 0, 0, 1,
	};
	return update_image(image, { 0, 0, 0 }, { image.get_width(), image.get_height(), image.get_depth() }, row_length,
	                    image_height, subresource);
}

void *CommandBuffer::allocate_vertex_data(unsigned binding, VkDeviceSize size, VkDeviceSize stride,
                                          VkVertexInputRate step_rate)
{
	auto data = vbo_block.allocate(size);
	if (!data.host)
	{
		device->request_vertex_block(vbo_block, size);
		data = vbo_block.allocate(size);
	}

	set_vertex_binding(binding, *vbo_block.gpu, data.offset, stride, step_rate);
	return data.host;
}

void CommandBuffer::set_uniform_buffer(unsigned set, unsigned binding, const Buffer &buffer, VkDeviceSize offset,
                                       VkDeviceSize range)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(binding < VULKAN_NUM_BINDINGS);
	VK_ASSERT(buffer.get_create_info().usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	auto &b = bindings.bindings[set][binding];

	if (buffer.get_cookie() == bindings.cookies[set][binding] && b.buffer.range == range)
	{
		if (b.dynamic_offset != offset)
		{
			dirty_sets_dynamic |= 1u << set;
			b.dynamic_offset = offset;
		}
	}
	else
	{
		b.buffer = { buffer.get_buffer(), 0, range };
		b.dynamic_offset = offset;
		bindings.cookies[set][binding] = buffer.get_cookie();
		bindings.secondary_cookies[set][binding] = 0;
		dirty_sets |= 1u << set;
	}
}

void CommandBuffer::set_storage_buffer(unsigned set, unsigned binding, const Buffer &buffer, VkDeviceSize offset,
                                       VkDeviceSize range)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(binding < VULKAN_NUM_BINDINGS);
	VK_ASSERT(buffer.get_create_info().usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	auto &b = bindings.bindings[set][binding];

	if (buffer.get_cookie() == bindings.cookies[set][binding] && b.buffer.offset == offset && b.buffer.range == range)
		return;

	b.buffer = { buffer.get_buffer(), offset, range };
	b.dynamic_offset = 0;
	bindings.cookies[set][binding] = buffer.get_cookie();
	bindings.secondary_cookies[set][binding] = 0;
	dirty_sets |= 1u << set;
}

void CommandBuffer::set_uniform_buffer(unsigned set, unsigned binding, const Buffer &buffer)
{
	set_uniform_buffer(set, binding, buffer, 0, buffer.get_create_info().size);
}

void CommandBuffer::set_storage_buffer(unsigned set, unsigned binding, const Buffer &buffer)
{
	set_storage_buffer(set, binding, buffer, 0, buffer.get_create_info().size);
}

void CommandBuffer::set_sampler(unsigned set, unsigned binding, const Sampler &sampler)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(binding < VULKAN_NUM_BINDINGS);
	if (sampler.get_cookie() == bindings.secondary_cookies[set][binding])
		return;

	auto &b = bindings.bindings[set][binding];
	b.image.fp.sampler = sampler.get_sampler();
	b.image.integer.sampler = sampler.get_sampler();
	dirty_sets |= 1u << set;
	bindings.secondary_cookies[set][binding] = sampler.get_cookie();
}

void CommandBuffer::set_buffer_view_common(unsigned set, unsigned binding, const BufferView &view)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(binding < VULKAN_NUM_BINDINGS);
	if (view.get_cookie() == bindings.cookies[set][binding])
		return;
	auto &b = bindings.bindings[set][binding];
	b.buffer_view = view.get_view();
	bindings.cookies[set][binding] = view.get_cookie();
	bindings.secondary_cookies[set][binding] = 0;
	dirty_sets |= 1u << set;
}

void CommandBuffer::set_buffer_view(unsigned set, unsigned binding, const BufferView &view)
{
	VK_ASSERT(view.get_buffer().get_create_info().usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
	set_buffer_view_common(set, binding, view);
}

void CommandBuffer::set_storage_buffer_view(unsigned set, unsigned binding, const BufferView &view)
{
	VK_ASSERT(view.get_buffer().get_create_info().usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
	set_buffer_view_common(set, binding, view);
}

void CommandBuffer::set_input_attachments(unsigned set, unsigned start_binding)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(start_binding + actual_render_pass->get_num_input_attachments(pipeline_state.subpass_index) <= VULKAN_NUM_BINDINGS);
	unsigned num_input_attachments = actual_render_pass->get_num_input_attachments(pipeline_state.subpass_index);
	for (unsigned i = 0; i < num_input_attachments; i++)
	{
		auto &ref = actual_render_pass->get_input_attachment(pipeline_state.subpass_index, i);
		if (ref.attachment == VK_ATTACHMENT_UNUSED)
			continue;

		const ImageView *view = framebuffer_attachments[ref.attachment];
		VK_ASSERT(view);
		VK_ASSERT(view->get_image().get_create_info().usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

		if (view->get_cookie() == bindings.cookies[set][start_binding + i] &&
		    bindings.bindings[set][start_binding + i].image.fp.imageLayout == ref.layout)
		{
			continue;
		}

		auto &b = bindings.bindings[set][start_binding + i];
		b.image.fp.imageLayout = ref.layout;
		b.image.integer.imageLayout = ref.layout;
		b.image.fp.imageView = view->get_float_view();
		b.image.integer.imageView = view->get_integer_view();
		bindings.cookies[set][start_binding + i] = view->get_cookie();
		dirty_sets |= 1u << set;
	}
}

void CommandBuffer::set_texture(unsigned set, unsigned binding,
                                VkImageView float_view, VkImageView integer_view,
                                VkImageLayout layout,
                                uint64_t cookie)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(binding < VULKAN_NUM_BINDINGS);

	if (cookie == bindings.cookies[set][binding] && bindings.bindings[set][binding].image.fp.imageLayout == layout)
		return;

	auto &b = bindings.bindings[set][binding];
	b.image.fp.imageLayout = layout;
	b.image.fp.imageView = float_view;
	b.image.integer.imageLayout = layout;
	b.image.integer.imageView = integer_view;
	bindings.cookies[set][binding] = cookie;
	dirty_sets |= 1u << set;
}

void CommandBuffer::set_bindless(unsigned set, VkDescriptorSet desc_set)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	bindless_sets[set] = desc_set;
	dirty_sets |= 1u << set;
}

void CommandBuffer::set_texture(unsigned set, unsigned binding, const ImageView &view)
{
	VK_ASSERT(view.get_image().get_create_info().usage & VK_IMAGE_USAGE_SAMPLED_BIT);
	set_texture(set, binding, view.get_float_view(), view.get_integer_view(),
	            view.get_image().get_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), view.get_cookie());
}

enum CookieBits
{
	COOKIE_BIT_UNORM = 1 << 0,
	COOKIE_BIT_SRGB = 1 << 1
};

void CommandBuffer::set_unorm_texture(unsigned set, unsigned binding, const ImageView &view)
{
	VK_ASSERT(view.get_image().get_create_info().usage & VK_IMAGE_USAGE_SAMPLED_BIT);
	auto unorm_view = view.get_unorm_view();
	VK_ASSERT(unorm_view != VK_NULL_HANDLE);
	set_texture(set, binding, unorm_view, unorm_view,
	            view.get_image().get_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), view.get_cookie() | COOKIE_BIT_UNORM);
}

void CommandBuffer::set_srgb_texture(unsigned set, unsigned binding, const ImageView &view)
{
	VK_ASSERT(view.get_image().get_create_info().usage & VK_IMAGE_USAGE_SAMPLED_BIT);
	auto srgb_view = view.get_srgb_view();
	VK_ASSERT(srgb_view != VK_NULL_HANDLE);
	set_texture(set, binding, srgb_view, srgb_view,
	            view.get_image().get_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), view.get_cookie() | COOKIE_BIT_SRGB);
}

void CommandBuffer::set_texture(unsigned set, unsigned binding, const ImageView &view, const Sampler &sampler)
{
	set_sampler(set, binding, sampler);
	set_texture(set, binding, view);
}

void CommandBuffer::set_texture(unsigned set, unsigned binding, const ImageView &view, StockSampler stock)
{
	VK_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
	VK_ASSERT(binding < VULKAN_NUM_BINDINGS);
	VK_ASSERT(view.get_image().get_create_info().usage & VK_IMAGE_USAGE_SAMPLED_BIT);
	const auto &sampler = device->get_stock_sampler(stock);
	set_texture(set, binding, view, sampler);
}

void CommandBuffer::set_sampler(unsigned set, unsigned binding, StockSampler stock)
{
	const auto &sampler = device->get_stock_sampler(stock);
	set_sampler(set, binding, sampler);
}

void CommandBuffer::set_storage_texture(unsigned set, unsigned binding, const ImageView &view)
{
	VK_ASSERT(view.get_image().get_create_info().usage & VK_IMAGE_USAGE_STORAGE_BIT);
	set_texture(set, binding, view.get_float_view(), view.get_integer_view(),
	            view.get_image().get_layout(VK_IMAGE_LAYOUT_GENERAL), view.get_cookie());
}

void CommandBuffer::set_unorm_storage_texture(unsigned set, unsigned binding, const ImageView &view)
{
	VK_ASSERT(view.get_image().get_create_info().usage & VK_IMAGE_USAGE_STORAGE_BIT);
	auto unorm_view = view.get_unorm_view();
	VK_ASSERT(unorm_view != VK_NULL_HANDLE);
	set_texture(set, binding, unorm_view, unorm_view,
	            view.get_image().get_layout(VK_IMAGE_LAYOUT_GENERAL), view.get_cookie() | COOKIE_BIT_UNORM);
}

void CommandBuffer::rebind_descriptor_set(uint32_t set)
{
	auto &layout = pipeline_state.layout->get_resource_layout();
	if (layout.bindless_descriptor_set_mask & (1u << set))
	{
		VK_ASSERT(bindless_sets[set]);
		table.vkCmdBindDescriptorSets(cmd, actual_render_pass ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		                              current_pipeline_layout, set, 1, &bindless_sets[set], 0, nullptr);
		return;
	}

	auto &set_layout = layout.sets[set];
	uint32_t num_dynamic_offsets = 0;
	uint32_t dynamic_offsets[VULKAN_NUM_BINDINGS];

	// UBOs
	for_each_bit(set_layout.uniform_buffer_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			VK_ASSERT(num_dynamic_offsets < VULKAN_NUM_BINDINGS);
			dynamic_offsets[num_dynamic_offsets++] = bindings.bindings[set][binding + i].dynamic_offset;
		}
	});

	table.vkCmdBindDescriptorSets(cmd, actual_render_pass ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
	                              current_pipeline_layout, set, 1, &allocated_sets[set], num_dynamic_offsets, dynamic_offsets);
}

void CommandBuffer::flush_descriptor_set(uint32_t set)
{
	auto &layout = pipeline_state.layout->get_resource_layout();
	if (layout.bindless_descriptor_set_mask & (1u << set))
	{
		VK_ASSERT(bindless_sets[set]);
		table.vkCmdBindDescriptorSets(cmd, actual_render_pass ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		                              current_pipeline_layout, set, 1, &bindless_sets[set], 0, nullptr);
		return;
	}

	auto &set_layout = layout.sets[set];
	uint32_t num_dynamic_offsets = 0;
	uint32_t dynamic_offsets[VULKAN_NUM_BINDINGS];
	Hasher h;

	h.u32(set_layout.fp_mask);

	// UBOs
	for_each_bit(set_layout.uniform_buffer_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			h.u32(bindings.bindings[set][binding + i].buffer.range);
			VK_ASSERT(bindings.bindings[set][binding + i].buffer.buffer != VK_NULL_HANDLE);

			VK_ASSERT(num_dynamic_offsets < VULKAN_NUM_BINDINGS);
			dynamic_offsets[num_dynamic_offsets++] = bindings.bindings[set][binding + i].dynamic_offset;
		}
	});

	// SSBOs
	for_each_bit(set_layout.storage_buffer_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			h.u32(bindings.bindings[set][binding + i].buffer.offset);
			h.u32(bindings.bindings[set][binding + i].buffer.range);
			VK_ASSERT(bindings.bindings[set][binding + i].buffer.buffer != VK_NULL_HANDLE);
		}
	});

	// Texel buffers
	for_each_bit(set_layout.sampled_texel_buffer_mask | set_layout.storage_texel_buffer_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			VK_ASSERT(bindings.bindings[set][binding + i].buffer_view != VK_NULL_HANDLE);
		}
	});

	// Sampled images
	for_each_bit(set_layout.sampled_image_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			if ((set_layout.immutable_sampler_mask & (1u << (binding + i))) == 0)
			{
				h.u64(bindings.secondary_cookies[set][binding + i]);
				VK_ASSERT(bindings.bindings[set][binding + i].image.fp.sampler != VK_NULL_HANDLE);
			}
			h.u32(bindings.bindings[set][binding + i].image.fp.imageLayout);
			VK_ASSERT(bindings.bindings[set][binding + i].image.fp.imageView != VK_NULL_HANDLE);
		}
	});

	// Separate images
	for_each_bit(set_layout.separate_image_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			h.u32(bindings.bindings[set][binding + i].image.fp.imageLayout);
			VK_ASSERT(bindings.bindings[set][binding + i].image.fp.imageView != VK_NULL_HANDLE);
		}
	});

	// Separate samplers
	for_each_bit(set_layout.sampler_mask & ~set_layout.immutable_sampler_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.secondary_cookies[set][binding + i]);
			VK_ASSERT(bindings.bindings[set][binding + i].image.fp.sampler != VK_NULL_HANDLE);
		}
	});

	// Storage images
	for_each_bit(set_layout.storage_image_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			h.u32(bindings.bindings[set][binding + i].image.fp.imageLayout);
			VK_ASSERT(bindings.bindings[set][binding + i].image.fp.imageView != VK_NULL_HANDLE);
		}
	});

	// Input attachments
	for_each_bit(set_layout.input_attachment_mask, [&](uint32_t binding) {
		unsigned array_size = set_layout.array_size[binding];
		for (unsigned i = 0; i < array_size; i++)
		{
			h.u64(bindings.cookies[set][binding + i]);
			h.u32(bindings.bindings[set][binding + i].image.fp.imageLayout);
			VK_ASSERT(bindings.bindings[set][binding + i].image.fp.imageView != VK_NULL_HANDLE);
		}
	});

	Hash hash = h.get();
	auto allocated = pipeline_state.layout->get_allocator(set)->find(thread_index, hash);

	// The descriptor set was not successfully cached, rebuild.
	if (!allocated.second)
	{
		auto update_template = pipeline_state.layout->get_update_template(set);
		VK_ASSERT(update_template);
		table.vkUpdateDescriptorSetWithTemplate(device->get_device(), allocated.first,
		                                        update_template, bindings.bindings[set]);
	}

	table.vkCmdBindDescriptorSets(cmd, actual_render_pass ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
	                              current_pipeline_layout, set, 1, &allocated.first, num_dynamic_offsets, dynamic_offsets);
	allocated_sets[set] = allocated.first;
}

void CommandBuffer::flush_descriptor_sets()
{
	auto &layout = pipeline_state.layout->get_resource_layout();

	uint32_t set_update = layout.descriptor_set_mask & dirty_sets;
	for_each_bit(set_update, [&](uint32_t set) { flush_descriptor_set(set); });
	dirty_sets &= ~set_update;

	// If we update a set, we also bind dynamically.
	dirty_sets_dynamic &= ~set_update;

	// If we only rebound UBOs, we might get away with just rebinding descriptor sets, no hashing and lookup required.
	uint32_t dynamic_set_update = layout.descriptor_set_mask & dirty_sets_dynamic;
	for_each_bit(dynamic_set_update, [&](uint32_t set) { rebind_descriptor_set(set); });
	dirty_sets_dynamic &= ~dynamic_set_update;
}

void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	VK_ASSERT(!is_compute);
	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Vertex) != nullptr);
		table.vkCmdDraw(cmd, vertex_count, instance_count, first_vertex, first_instance);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index,
                                 int32_t vertex_offset, uint32_t first_instance)
{
	VK_ASSERT(!is_compute);
	VK_ASSERT(index_state.buffer != VK_NULL_HANDLE);
	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Vertex) != nullptr);
		table.vkCmdDrawIndexed(cmd, index_count, instance_count, first_index, vertex_offset, first_instance);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_mesh_tasks(uint32_t tasks_x, uint32_t tasks_y, uint32_t tasks_z)
{
	VK_ASSERT(!is_compute);

	if (framebuffer_is_multiview && !get_device().get_device_features().mesh_shader_features.multiviewMeshShader)
	{
		LOGE("meshShader not supported in multiview, dropping draw call.\n");
		return;
	}

	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Mesh) != nullptr);
		table.vkCmdDrawMeshTasksEXT(cmd, tasks_x, tasks_y, tasks_z);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_mesh_tasks_indirect(const Buffer &buffer, VkDeviceSize offset,
                                             uint32_t draw_count, uint32_t stride)
{
	VK_ASSERT(!is_compute);

	if (framebuffer_is_multiview && !get_device().get_device_features().mesh_shader_features.multiviewMeshShader)
	{
		LOGE("meshShader not supported in multiview, dropping draw call.\n");
		return;
	}

	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Mesh) != nullptr);
		table.vkCmdDrawMeshTasksIndirectEXT(cmd, buffer.get_buffer(), offset, draw_count, stride);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_mesh_tasks_multi_indirect(const Buffer &buffer, VkDeviceSize offset,
                                                   uint32_t draw_count, uint32_t stride,
                                                   const Buffer &count, VkDeviceSize count_offset)
{
	VK_ASSERT(!is_compute);

	if (framebuffer_is_multiview && !get_device().get_device_features().mesh_shader_features.multiviewMeshShader)
	{
		LOGE("meshShader not supported in multiview, dropping draw call.\n");
		return;
	}

	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Mesh) != nullptr);
		table.vkCmdDrawMeshTasksIndirectCountEXT(cmd, buffer.get_buffer(), offset,
		                                         count.get_buffer(), count_offset,
		                                         draw_count, stride);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_indirect(const Vulkan::Buffer &buffer,
                                  VkDeviceSize offset, uint32_t draw_count, uint32_t stride)
{
	VK_ASSERT(!is_compute);
	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Vertex) != nullptr);
		table.vkCmdDrawIndirect(cmd, buffer.get_buffer(), offset, draw_count, stride);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_multi_indirect(const Buffer &buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride,
                                        const Buffer &count, VkDeviceSize count_offset)
{
	VK_ASSERT(!is_compute);
	if (!get_device().get_device_features().supports_draw_indirect_count)
	{
		LOGE("VK_KHR_draw_indirect_count not supported, dropping draw call.\n");
		return;
	}

	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Vertex) != nullptr);
		table.vkCmdDrawIndirectCountKHR(cmd, buffer.get_buffer(), offset,
		                                count.get_buffer(), count_offset,
		                                draw_count, stride);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_indexed_multi_indirect(const Buffer &buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride,
                                                const Buffer &count, VkDeviceSize count_offset)
{
	VK_ASSERT(!is_compute);
	if (!get_device().get_device_features().supports_draw_indirect_count)
	{
		LOGE("VK_KHR_draw_indirect_count not supported, dropping draw call.\n");
		return;
	}

	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Vertex) != nullptr);
		table.vkCmdDrawIndexedIndirectCountKHR(cmd, buffer.get_buffer(), offset,
		                                       count.get_buffer(), count_offset,
		                                       draw_count, stride);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::draw_indexed_indirect(const Vulkan::Buffer &buffer,
                                          VkDeviceSize offset, uint32_t draw_count, uint32_t stride)
{
	VK_ASSERT(!is_compute);
	if (flush_render_state(true) != VK_NULL_HANDLE)
	{
		VK_ASSERT(pipeline_state.program->get_shader(ShaderStage::Vertex) != nullptr);
		table.vkCmdDrawIndexedIndirect(cmd, buffer.get_buffer(), offset, draw_count, stride);
	}
	else
		LOGE("Failed to flush render state, draw call will be dropped.\n");
}

void CommandBuffer::dispatch_indirect(const Buffer &buffer, VkDeviceSize offset)
{
	VK_ASSERT(is_compute);
	if (flush_compute_state(true) != VK_NULL_HANDLE)
	{
		table.vkCmdDispatchIndirect(cmd, buffer.get_buffer(), offset);
	}
	else
		LOGE("Failed to flush render state, dispatch will be dropped.\n");
}

void CommandBuffer::execute_indirect_commands(
		const IndirectLayout *indirect_layout, uint32_t sequences,
		const Vulkan::Buffer &indirect, VkDeviceSize offset,
		const Vulkan::Buffer *count, size_t count_offset)
{
	VK_ASSERT(!is_compute);
	VK_ASSERT(device->get_device_features().device_generated_commands_features.deviceGeneratedCommands);

	if (flush_render_state(true) == VK_NULL_HANDLE)
	{
		LOGE("Failed to flush render state, draw call will be dropped.\n");
		return;
	}

	// TODO: Linearly allocate these, but big indirect commands like these
	// should only be done once per render pass anyways.
	VkGeneratedCommandsMemoryRequirementsInfoNV generated =
			{ VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_NV };
	VkMemoryRequirements2 reqs = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };

	generated.pipeline = current_pipeline.pipeline;
	generated.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	generated.indirectCommandsLayout = indirect_layout->get_layout();
	generated.maxSequencesCount = sequences;

	table.vkGetGeneratedCommandsMemoryRequirementsNV(device->get_device(), &generated, &reqs);

	BufferCreateInfo bufinfo = {};
	bufinfo.size = reqs.memoryRequirements.size;
	bufinfo.domain = BufferDomain::Device;
	bufinfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	bufinfo.allocation_requirements = reqs.memoryRequirements;
	auto preprocess_buffer = device->create_buffer(bufinfo);

	VkIndirectCommandsStreamNV stream = {};
	stream.buffer = indirect.get_buffer();
	stream.offset = offset;

	VkGeneratedCommandsInfoNV exec_info = { VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_NV };
	exec_info.indirectCommandsLayout = indirect_layout->get_layout();
	exec_info.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	exec_info.streamCount = 1;
	exec_info.pStreams = &stream;
	exec_info.preprocessSize = reqs.memoryRequirements.size;
	exec_info.preprocessBuffer = preprocess_buffer->get_buffer();
	exec_info.sequencesCount = sequences;
	exec_info.pipeline = current_pipeline.pipeline;
	if (count)
	{
		exec_info.sequencesCountBuffer = count->get_buffer();
		exec_info.sequencesCountOffset = count_offset;
	}
	table.vkCmdExecuteGeneratedCommandsNV(cmd, VK_FALSE, &exec_info);

	// Everything is nuked after execute generated commands.
	set_dirty(COMMAND_BUFFER_DYNAMIC_BITS |
	          COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT |
	          COMMAND_BUFFER_DIRTY_PIPELINE_BIT);
}

void CommandBuffer::dispatch(uint32_t groups_x, uint32_t groups_y, uint32_t groups_z)
{
	VK_ASSERT(is_compute);
	if (flush_compute_state(true) != VK_NULL_HANDLE)
		table.vkCmdDispatch(cmd, groups_x, groups_y, groups_z);
	else
		LOGE("Failed to flush render state, dispatch will be dropped.\n");
}

void CommandBuffer::clear_render_state()
{
	// Preserve spec constant mask.
	auto &state = pipeline_state.static_state.state;
	memset(&state, 0, sizeof(state));
}

void CommandBuffer::set_opaque_state()
{
	clear_render_state();
	auto &state = pipeline_state.static_state.state;
	state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	state.cull_mode = VK_CULL_MODE_BACK_BIT;
	state.blend_enable = false;
	state.depth_test = true;
	state.depth_compare = VK_COMPARE_OP_LESS_OR_EQUAL;
	state.depth_write = true;
	state.depth_bias_enable = false;
	state.primitive_restart = false;
	state.stencil_test = false;
	state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	state.write_mask = ~0u;
	set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
}

void CommandBuffer::set_quad_state()
{
	clear_render_state();
	auto &state = pipeline_state.static_state.state;
	state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	state.cull_mode = VK_CULL_MODE_NONE;
	state.blend_enable = false;
	state.depth_test = false;
	state.depth_write = false;
	state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	state.write_mask = ~0u;
	set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
}

void CommandBuffer::set_opaque_sprite_state()
{
	clear_render_state();
	auto &state = pipeline_state.static_state.state;
	state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	state.cull_mode = VK_CULL_MODE_NONE;
	state.blend_enable = false;
	state.depth_compare = VK_COMPARE_OP_LESS;
	state.depth_test = true;
	state.depth_write = true;
	state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	state.write_mask = ~0u;
	set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
}

void CommandBuffer::set_transparent_sprite_state()
{
	clear_render_state();
	auto &state = pipeline_state.static_state.state;
	state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	state.cull_mode = VK_CULL_MODE_NONE;
	state.blend_enable = true;
	state.depth_test = true;
	state.depth_compare = VK_COMPARE_OP_LESS;
	state.depth_write = false;
	state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	state.write_mask = ~0u;

	// The alpha layer should start at 1 (fully transparent).
	// As layers are blended in, the transparency is multiplied with other transparencies (1 - alpha).
	set_blend_factors(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ZERO,
	                  VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	set_blend_op(VK_BLEND_OP_ADD);

	set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
}

void CommandBuffer::restore_state(const CommandBufferSavedState &state)
{
	auto &static_state = pipeline_state.static_state;
	auto &potential_static_state = pipeline_state.potential_static_state;

	for (unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
	{
		if (state.flags & (COMMAND_BUFFER_SAVED_BINDINGS_0_BIT << i))
		{
			if (memcmp(state.bindings.bindings[i], bindings.bindings[i], sizeof(bindings.bindings[i])))
			{
				memcpy(bindings.bindings[i], state.bindings.bindings[i], sizeof(bindings.bindings[i]));
				memcpy(bindings.cookies[i], state.bindings.cookies[i], sizeof(bindings.cookies[i]));
				memcpy(bindings.secondary_cookies[i], state.bindings.secondary_cookies[i], sizeof(bindings.secondary_cookies[i]));
				dirty_sets |= 1u << i;
			}
		}
	}

	if (state.flags & COMMAND_BUFFER_SAVED_PUSH_CONSTANT_BIT)
	{
		if (memcmp(state.bindings.push_constant_data, bindings.push_constant_data, sizeof(bindings.push_constant_data)) != 0)
		{
			memcpy(bindings.push_constant_data, state.bindings.push_constant_data, sizeof(bindings.push_constant_data));
			set_dirty(COMMAND_BUFFER_DIRTY_PUSH_CONSTANTS_BIT);
		}
	}

	if ((state.flags & COMMAND_BUFFER_SAVED_VIEWPORT_BIT) && memcmp(&state.viewport, &viewport, sizeof(viewport)) != 0)
	{
		viewport = state.viewport;
		set_dirty(COMMAND_BUFFER_DIRTY_VIEWPORT_BIT);
	}

	if ((state.flags & COMMAND_BUFFER_SAVED_SCISSOR_BIT) && memcmp(&state.scissor, &scissor, sizeof(scissor)) != 0)
	{
		scissor = state.scissor;
		set_dirty(COMMAND_BUFFER_DIRTY_SCISSOR_BIT);
	}

	if (state.flags & COMMAND_BUFFER_SAVED_RENDER_STATE_BIT)
	{
		if (memcmp(&state.static_state, &static_state, sizeof(static_state)) != 0)
		{
			memcpy(&static_state, &state.static_state, sizeof(static_state));
			set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
		}

		if (memcmp(&state.potential_static_state, &potential_static_state, sizeof(potential_static_state)) != 0)
		{
			memcpy(&potential_static_state, &state.potential_static_state, sizeof(potential_static_state));
			set_dirty(COMMAND_BUFFER_DIRTY_STATIC_STATE_BIT);
		}

		if (memcmp(&state.dynamic_state, &dynamic_state, sizeof(dynamic_state)) != 0)
		{
			memcpy(&dynamic_state, &state.dynamic_state, sizeof(dynamic_state));
			set_dirty(COMMAND_BUFFER_DIRTY_STENCIL_REFERENCE_BIT | COMMAND_BUFFER_DIRTY_DEPTH_BIAS_BIT);
		}
	}
}

void CommandBuffer::save_state(CommandBufferSaveStateFlags flags, CommandBufferSavedState &state)
{
	for (unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
	{
		if (flags & (COMMAND_BUFFER_SAVED_BINDINGS_0_BIT << i))
		{
			memcpy(state.bindings.bindings[i], bindings.bindings[i], sizeof(bindings.bindings[i]));
			memcpy(state.bindings.cookies[i], bindings.cookies[i], sizeof(bindings.cookies[i]));
			memcpy(state.bindings.secondary_cookies[i], bindings.secondary_cookies[i],
			       sizeof(bindings.secondary_cookies[i]));
		}
	}

	if (flags & COMMAND_BUFFER_SAVED_VIEWPORT_BIT)
		state.viewport = viewport;
	if (flags & COMMAND_BUFFER_SAVED_SCISSOR_BIT)
		state.scissor = scissor;
	if (flags & COMMAND_BUFFER_SAVED_RENDER_STATE_BIT)
	{
		memcpy(&state.static_state, &pipeline_state.static_state, sizeof(pipeline_state.static_state));
		state.potential_static_state = pipeline_state.potential_static_state;
		state.dynamic_state = dynamic_state;
	}

	if (flags & COMMAND_BUFFER_SAVED_PUSH_CONSTANT_BIT)
		memcpy(state.bindings.push_constant_data, bindings.push_constant_data, sizeof(bindings.push_constant_data));

	state.flags = flags;
}

QueryPoolHandle CommandBuffer::write_timestamp(VkPipelineStageFlags2 stage)
{
	return device->write_timestamp(cmd, stage);
}

void CommandBuffer::end_threaded_recording()
{
	VK_ASSERT(!debug_channel_buffer);

	if (is_ended)
		return;

	is_ended = true;

	// We must end a command buffer on the same thread index we started it on.
	VK_ASSERT(get_current_thread_index() == thread_index);

	if (has_profiling())
	{
		auto &query_pool = device->get_performance_query_pool(device->get_physical_queue_type(type));
		query_pool.end_command_buffer(cmd);
	}

	if (table.vkEndCommandBuffer(cmd) != VK_SUCCESS)
		LOGE("Failed to end command buffer.\n");
}

void CommandBuffer::end()
{
	end_threaded_recording();

	if (vbo_block.mapped)
		device->request_vertex_block_nolock(vbo_block, 0);
	if (ibo_block.mapped)
		device->request_index_block_nolock(ibo_block, 0);
	if (ubo_block.mapped)
		device->request_uniform_block_nolock(ubo_block, 0);
	if (staging_block.mapped)
		device->request_staging_block_nolock(staging_block, 0);
}

void CommandBuffer::begin_region(const char *name, const float *color)
{
	if (device->ext.supports_debug_utils)
	{
		VkDebugUtilsLabelEXT info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		if (color)
		{
			for (unsigned i = 0; i < 4; i++)
				info.color[i] = color[i];
		}
		else
		{
			for (unsigned i = 0; i < 4; i++)
				info.color[i] = 1.0f;
		}

		info.pLabelName = name;
		if (vkCmdBeginDebugUtilsLabelEXT)
			vkCmdBeginDebugUtilsLabelEXT(cmd, &info);
	}
}

void CommandBuffer::end_region()
{
	if (device->ext.supports_debug_utils)
	{
		if (vkCmdEndDebugUtilsLabelEXT)
			vkCmdEndDebugUtilsLabelEXT(cmd);
	}
}

void CommandBuffer::enable_profiling()
{
	profiling = true;
}

bool CommandBuffer::has_profiling() const
{
	return profiling;
}

void CommandBuffer::begin_debug_channel(DebugChannelInterface *iface, const char *tag, VkDeviceSize size)
{
	if (debug_channel_buffer)
		end_debug_channel();

	debug_channel_tag = tag;
	debug_channel_interface = iface;

	BufferCreateInfo info = {};
	info.size = size;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	info.domain = BufferDomain::Device;
	debug_channel_buffer = device->create_buffer(info);

	fill_buffer(*debug_channel_buffer, 0);
	buffer_barrier(*debug_channel_buffer,
	               VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
	               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	               VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT);

	set_storage_buffer(VULKAN_NUM_DESCRIPTOR_SETS - 1, VULKAN_NUM_BINDINGS - 1, *debug_channel_buffer);
}

void CommandBuffer::end_debug_channel()
{
	if (!debug_channel_buffer)
		return;

	BufferCreateInfo info = {};
	info.size = debug_channel_buffer->get_create_info().size;
	info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.domain = BufferDomain::CachedHost;
	auto debug_channel_readback = device->create_buffer(info);
	barrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_SHADER_WRITE_BIT,
	        VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_TRANSFER_READ_BIT);
	copy_buffer(*debug_channel_readback, *debug_channel_buffer);
	barrier(VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
	        VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT);

	debug_channel_buffer.reset();
	device->add_debug_channel_buffer(debug_channel_interface, std::move(debug_channel_tag), std::move(debug_channel_readback));
	debug_channel_readback = {};
	debug_channel_tag = {};
	debug_channel_interface = nullptr;
}

#ifdef GRANITE_VULKAN_SYSTEM_HANDLES
void CommandBufferUtil::set_quad_vertex_state(CommandBuffer &cmd)
{
#ifdef __APPLE__
	// For *some* reason, Metal does not support tightly packed R8G8 ...
	// Have to use RGBA8 <_<.
	auto *data = static_cast<int8_t *>(cmd.allocate_vertex_data(0, 16, 4));
	*data++ = -127;
	*data++ = +127;
	*data++ = 0;
	*data++ = +127;
	*data++ = +127;
	*data++ = +127;
	*data++ = 0;
	*data++ = +127;
	*data++ = -127;
	*data++ = -127;
	*data++ = 0;
	*data++ = +127;
	*data++ = +127;
	*data++ = -127;
	*data++ = 0;
	*data++ = +127;

	cmd.set_vertex_attrib(0, 0, VK_FORMAT_R8G8B8A8_SNORM, 0);
#else
	auto *data = static_cast<int8_t *>(cmd.allocate_vertex_data(0, 8, 2));
	*data++ = -127;
	*data++ = +127;
	*data++ = +127;
	*data++ = +127;
	*data++ = -127;
	*data++ = -127;
	*data++ = +127;
	*data++ = -127;

	cmd.set_vertex_attrib(0, 0, VK_FORMAT_R8G8_SNORM, 0);
#endif
}

void CommandBufferUtil::set_fullscreen_quad_vertex_state(CommandBuffer &cmd)
{
	auto *data = static_cast<float *>(cmd.allocate_vertex_data(0, 6 * sizeof(float), 2 * sizeof(float)));
	*data++ = -1.0f;
	*data++ = -3.0f;
	*data++ = -1.0f;
	*data++ = +1.0f;
	*data++ = +3.0f;
	*data++ = +1.0f;

	cmd.set_vertex_attrib(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
}

void CommandBufferUtil::draw_fullscreen_quad(CommandBuffer &cmd, unsigned instances)
{
	cmd.set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	cmd.draw(3, instances);
}

void CommandBufferUtil::draw_quad(CommandBuffer &cmd, unsigned instances)
{
	cmd.set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	cmd.draw(4, instances);
}

void CommandBufferUtil::draw_fullscreen_quad(CommandBuffer &cmd, const std::string &vertex, const std::string &fragment,
                                             const std::vector<std::pair<std::string, int>> &defines)
{
	draw_fullscreen_quad_depth(cmd, vertex, fragment, false, false, VK_COMPARE_OP_ALWAYS, defines);
}

void CommandBufferUtil::draw_fullscreen_quad_depth(CommandBuffer &cmd, const std::string &vertex,
                                                   const std::string &fragment,
                                                   bool depth_test, bool depth_write, VkCompareOp depth_compare,
                                                   const std::vector<std::pair<std::string, int>> &defines)
{
	setup_fullscreen_quad(cmd, vertex, fragment, defines, depth_test, depth_write, depth_compare);
	draw_fullscreen_quad(cmd);
}

void CommandBufferUtil::setup_fullscreen_quad(Vulkan::CommandBuffer &cmd, const std::string &vertex,
                                              const std::string &fragment,
                                              const std::vector<std::pair<std::string, int>> &defines, bool depth_test,
                                              bool depth_write, VkCompareOp depth_compare)
{
	cmd.set_program(vertex, fragment, defines);
	cmd.set_quad_state();
	set_fullscreen_quad_vertex_state(cmd);
	cmd.set_depth_test(depth_test, depth_write);
	cmd.set_depth_compare(depth_compare);
	cmd.set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
}
#endif

void CommandBufferDeleter::operator()(Vulkan::CommandBuffer *cmd)
{
	cmd->device->handle_pool.command_buffers.free(cmd);
}
}
