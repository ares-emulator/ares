/* Copyright (c) 2020 Themaister
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

#include "video_interface.hpp"
#include "rdp_renderer.hpp"
#include "luts.hpp"
#include "bitops.hpp"
#include <cmath>

#ifndef PARALLEL_RDP_SHADER_DIR
#include "shaders/slangmosh.hpp"
#endif

namespace RDP
{
void VideoInterface::set_device(Vulkan::Device *device_)
{
	device = device_;
	init_gamma_table();

	if (const char *env = getenv("VI_DEBUG"))
		debug_channel = strtol(env, nullptr, 0) != 0;
	if (const char *env = getenv("VI_DEBUG_X"))
		filter_debug_channel_x = strtol(env, nullptr, 0);
	if (const char *env = getenv("VI_DEBUG_Y"))
		filter_debug_channel_y = strtol(env, nullptr, 0);

	if (const char *timestamp_env = getenv("PARALLEL_RDP_BENCH"))
		timestamp = strtol(timestamp_env, nullptr, 0) > 0;
}

void VideoInterface::set_renderer(Renderer *renderer_)
{
	renderer = renderer_;
}

int VideoInterface::resolve_shader_define(const char *name, const char *define) const
{
	if (strcmp(define, "DEBUG_ENABLE") == 0)
		return int(debug_channel);
	else
		return 0;
}

void VideoInterface::message(const std::string &tag, uint32_t code, uint32_t x, uint32_t y, uint32_t, uint32_t num_words,
                             const Vulkan::DebugChannelInterface::Word *words)
{
	if (filter_debug_channel_x >= 0 && x != uint32_t(filter_debug_channel_x))
		return;
	if (filter_debug_channel_y >= 0 && y != uint32_t(filter_debug_channel_y))
		return;

	switch (num_words)
	{
	case 1:
		LOGI("(%u, %u), line %d.\n", x, y, words[0].s32);
		break;

	case 2:
		LOGI("(%u, %u), line %d: (%d).\n", x, y, words[0].s32, words[1].s32);
		break;

	case 3:
		LOGI("(%u, %u), line %d: (%d, %d).\n", x, y, words[0].s32, words[1].s32, words[2].s32);
		break;

	case 4:
		LOGI("(%u, %u), line %d: (%d, %d, %d).\n", x, y,
		     words[0].s32, words[1].s32, words[2].s32, words[3].s32);
		break;

	default:
		LOGE("Unknown number of generic parameters: %u\n", num_words);
		break;
	}
}

void VideoInterface::init_gamma_table()
{
	Vulkan::BufferCreateInfo info = {};
	info.domain = Vulkan::BufferDomain::Device;
	info.size = sizeof(gamma_table);
	info.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;

	gamma_lut = device->create_buffer(info, gamma_table);

	Vulkan::BufferViewCreateInfo view = {};
	view.buffer = gamma_lut.get();
	view.range = sizeof(gamma_table);
	view.format = VK_FORMAT_R8_UINT;
	gamma_lut_view = device->create_buffer_view(view);
}

void VideoInterface::set_vi_register(VIRegister reg, uint32_t value)
{
	vi_registers[unsigned(reg)] = value;
}

void VideoInterface::set_rdram(const Vulkan::Buffer *rdram_, size_t offset, size_t size)
{
	rdram = rdram_;
	rdram_offset = offset;
	rdram_size = size;
}

void VideoInterface::set_hidden_rdram(const Vulkan::Buffer *hidden_rdram_)
{
	hidden_rdram = hidden_rdram_;
}

void VideoInterface::set_shader_bank(const ShaderBank *bank)
{
	shader_bank = bank;
}

static VkPipelineStageFlagBits layout_to_stage(VkImageLayout layout)
{
	switch (layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	default:
		return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
}

static VkAccessFlags layout_to_access(VkImageLayout layout)
{
	switch (layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return VK_ACCESS_TRANSFER_READ_BIT;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return VK_ACCESS_TRANSFER_WRITE_BIT;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return VK_ACCESS_SHADER_READ_BIT;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	default:
		return 0;
	}
}

VideoInterface::Registers VideoInterface::decode_vi_registers(HorizontalInfoLines *lines) const
{
	Registers reg = {};

	// Definitely should not change per scanline ...
	reg.status = vi_registers[unsigned(VIRegister::Control)];
	reg.vi_width = vi_registers[unsigned(VIRegister::Width)] & 0xfff;
	reg.vi_offset = vi_registers[unsigned(VIRegister::Origin)] & 0xffffff;
	reg.v_current_line = vi_registers[unsigned(VIRegister::VCurrentLine)] & 1;
	reg.v_start = (vi_registers[unsigned(VIRegister::VStart)] >> 16) & 0x3ff;

	// It might be possible to change YStart/YAdd per scanline, but it's dubious ...
	int y_start = (vi_registers[unsigned(VIRegister::YScale)] >> 16) & 0xfff;
	int y_add = vi_registers[unsigned(VIRegister::YScale)] & 0xfff;
	reg.init_y_add = y_add;

	// Must be constant for a frame, or things won't make any sense.
	int v_end = vi_registers[unsigned(VIRegister::VStart)] & 0x3ff;
	int v_sync = vi_registers[unsigned(VIRegister::VSync)] & 0x3ff;

	reg.is_pal = unsigned(v_sync) > (VI_V_SYNC_NTSC + 25);

	// Clamp vertical ranges.
	// Restrict the area we might have to scan out to AA buffers as a performance optimization
	// and safety since we iterate over v_start/v_end later.
	int v_end_max = reg.is_pal ? VI_V_END_PAL : VI_V_END_NTSC;
	if (v_end > v_end_max)
		v_end = v_end_max;
	if (reg.v_start > v_end_max)
		reg.v_start = v_end_max;

	int v_start_offset = reg.is_pal ? VI_V_OFFSET_PAL : VI_V_OFFSET_NTSC;
	reg.v_res = (v_end - reg.v_start) >> 1;
	reg.v_start = (reg.v_start - v_start_offset) / 2;

	if (reg.v_start < 0)
	{
		// If YAdd can change per scanline, this won't be correct.
		y_start -= y_add * reg.v_start;
		// v_res is not adjusted here for some reason, but h_res is?
		reg.v_start = 0;
	}

	// Clamp v_res to scanout range.
	reg.v_res = std::min(reg.v_res, int(VI_MAX_OUTPUT_SCANLINES) - reg.v_start);

	// Horizontal shenanigans.
	int h_start_clamp_lo = INT32_MAX;
	int h_end_clamp_hi = 0;
	int h_start_lo = INT32_MAX;
	int h_end_hi = 0;

	reg.max_x = 0;
	bool degenerate_y = reg.v_res <= 0;

	// Clear out degenerate lines.
	if (lines)
	{
		if (degenerate_y)
		{
			for (auto &line : lines->lines)
				line = {};
		}
		else
		{
			for (int line = 0; line < reg.v_start; line++)
				lines->lines[line] = {};
			for (int line = reg.v_start + reg.v_res; line < int(VI_MAX_OUTPUT_SCANLINES); line++)
				lines->lines[line] = {};
		}
	}

	const auto analyze_line = [&](int x_start, int x_add, int h_start, int h_end, HorizontalInfo *line) {
		// Clamp horizontal region to [0, 640].
		bool left_clamp = false;
		bool right_clamp = false;

		h_start -= reg.is_pal ? VI_H_OFFSET_PAL : VI_H_OFFSET_NTSC;
		h_end -= reg.is_pal ? VI_H_OFFSET_PAL : VI_H_OFFSET_NTSC;

		if (h_start < 0)
		{
			x_start -= x_add * h_start;
			h_start = 0;
			// Reference weirdness that doesn't really make sense.
			left_clamp = true;
		}

		if (h_end > VI_SCANOUT_WIDTH)
		{
			h_end = VI_SCANOUT_WIDTH;
			// Reference weirdness that doesn't really make sense.
			right_clamp = true;
		}

		int h_start_clamp = h_start + (left_clamp ? 0 : 8);
		int h_end_clamp = h_end - (right_clamp ? 0 : 7);

		// Effectively, these are bounding boxes.
		int h_res = h_end - h_start;
		int max_x = (x_start + h_res * x_add) >> 10;
		reg.max_x = std::max(reg.max_x, max_x);
		h_start_lo = std::min(h_start_lo, h_start);
		h_end_hi = std::max(h_end_hi, h_end);
		h_start_clamp_lo = std::min(h_start_clamp_lo, h_start_clamp);
		h_end_clamp_hi = std::max(h_end_clamp_hi, h_end_clamp);

		if (line)
		{
			auto &l = *line;
			l.h_start = h_start;
			l.h_start_clamp = h_start_clamp;
			l.h_end_clamp = h_end_clamp;
			l.x_start = x_start;
			l.x_add = x_add;
			l.y_start = y_start;
			l.y_add = y_add;
			l.y_base = 0; // TODO: If we start adjusting YAdd per scanline, we'll need to begin a new base region.
		}
	};

	if (degenerate_y || !per_line_state.ended)
	{
		int x_start = (vi_registers[unsigned(VIRegister::XScale)] >> 16) & 0xfff;
		int x_add = vi_registers[unsigned(VIRegister::XScale)] & 0xfff;
		int h_start = (vi_registers[unsigned(VIRegister::HStart)] >> 16) & 0x3ff;
		int h_end = vi_registers[unsigned(VIRegister::HStart)] & 0x3ff;

		HorizontalInfo line_info;

		// Need to make sure we update bounding box state for X at least.
		// This is treated as a degenerate frame, not necessarily invalid frame (null handle).
		// This is to have same behavior as reference.
		analyze_line(x_start, x_add, h_start, h_end, &line_info);

		if (lines)
			for (int line = reg.v_start; line < reg.v_start + reg.v_res; line++)
				lines->lines[line] = line_info;
	}
	else
	{
		for (int line = reg.v_start; line < reg.v_start + reg.v_res; line++)
		{
			// TODO: No idea if this is correct. This intuitively makes sense, but that's about it.
			int effective_line = 2 * line + v_start_offset + int(reg.v_current_line == 0);

			int x_start = (per_line_state.x_scale.line_state[effective_line] >> 16) & 0xfff;
			int x_add = per_line_state.x_scale.line_state[effective_line] & 0xfff;
			int h_start = (per_line_state.h_start.line_state[effective_line] >> 16) & 0x3ff;
			int h_end = per_line_state.h_start.line_state[effective_line] & 0x3ff;

			analyze_line(x_start, x_add, h_start, h_end, lines ? &lines->lines[line] : nullptr);
		}
	}

	// Effectively, these are bounding boxes.
	reg.max_y = (y_start + reg.v_res * y_add) >> 10;
	reg.h_start = h_start_lo;
	reg.h_res = h_end_hi - h_start_lo;
	reg.h_start_clamp = h_start_clamp_lo;
	reg.h_res_clamp = h_end_clamp_hi - h_start_clamp_lo;

	// The basic formula is that a frame is counted with an active horizontal range of
	// X(range) = [H_OFFSET, H_OFFSET + H_RES], giving 640 output pixels per line.
	// Similarly, vertical scanout has an active range of Y(range) = [V_OFFSET, V_OFFSET + V_RES].
	// Y is counted in terms of interlaced lines (i.e. 480 and 576).
	// We will scan out half of these per field (e.g. 240p or 480i for NTSC).
	// The HStart and VStart registers are used to signal where on screen we render.
	// HStart and VStart registers might carve out a portion of the screen, or use a larger one,
	// the active area on screen is an intersection of the VI register state and the X(range)/Y(range).
	// When the X counter hits HStart, we begin computing the X coordinate we want to sample based on
	// X(sample) = XStart + XAdd * (X - HStart).
	// Similarly, Y(sample) = YStart + YAdd * (Y - (VStart >> 1)), YAdd increments once per scanline.
	// We always normalize the interpolations to be progressive.
	// Interlacing just shifts positions on screen after the fact.
	//
	// VRAM(X, Y) is fetched with any post-processing required, looking at neighboring VRAM pixels.
	// For this reason, we compute the maximum X and Y we might access, and build an X x Y image
	// which is already preprocessed with AA, Divot filters, etc.
	// The final scaling pass interpolates that result.
	// The mental model here is that the VI could have a line buffer to keep some scanlines in cache to support the
	// processing. XStart/YStart registers just control how fast we iterate through these lines,
	// which implements scaling effects.
	//
	// As another weird quirk, it seems like we need to account for a
	// 8 pixel guard band horizontally (reg.left_clamp / reg.right_clamp)
	// if we begin scanout inside the active region for whatever reason.
	// This is to match reference.

	return reg;
}

void VideoInterface::scanout_memory_range(unsigned &offset, unsigned &length) const
{
	auto reg = decode_vi_registers(nullptr);

	bool divot = (reg.status & VI_CONTROL_DIVOT_ENABLE_BIT) != 0;

	// Need to sample a 2-pixel border to have room for AA filter and divot.
	int aa_width = reg.max_x + 2 + 4 + int(divot) * 2;
	// 1 pixel border on top and bottom.
	int aa_height = reg.max_y + 1 + 4;

	int x_off = divot ? -3 : -2;
	int y_off = -2;

	if (reg.vi_offset == 0 || reg.h_res <= 0 || reg.h_start >= VI_SCANOUT_WIDTH)
	{
		offset = 0;
		length = 0;
		return;
	}

	int pixel_size = ((reg.status & VI_CONTROL_TYPE_MASK) | VI_CONTROL_TYPE_RGBA5551_BIT) == VI_CONTROL_TYPE_RGBA8888_BIT ? 4 : 2;
	reg.vi_offset &= ~(pixel_size - 1);
	reg.vi_offset += (x_off + y_off * reg.vi_width) * pixel_size;

	offset = reg.vi_offset;
	length = (aa_height * reg.vi_width + aa_width) * pixel_size;
}

bool VideoInterface::need_fetch_bug_emulation(const Registers &regs, unsigned scaling_factor)
{
	// If we risk sampling same Y coordinate for two scanlines we can trigger this case,
	// so add workaround paths for it.
	return regs.init_y_add < 1024 && scaling_factor == 1;
}

Vulkan::ImageHandle VideoInterface::vram_fetch_stage(const Registers &regs, unsigned scaling_factor) const
{
	auto async_cmd = device->request_command_buffer(Vulkan::CommandBuffer::Type::AsyncCompute);
	Vulkan::ImageHandle vram_image;
	Vulkan::QueryPoolHandle start_ts, end_ts;
	bool divot = (regs.status & VI_CONTROL_DIVOT_ENABLE_BIT) != 0;

	if (scaling_factor > 1)
	{
		unsigned pixel_size_log2 = ((regs.status & VI_CONTROL_TYPE_MASK) == VI_CONTROL_TYPE_RGBA8888_BIT) ? 2 : 1;
		unsigned offset, length;
		scanout_memory_range(offset, length);
		renderer->submit_update_upscaled_domain_external(*async_cmd, offset, length, pixel_size_log2);
		async_cmd->barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT,
		                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	}

	if (timestamp)
		start_ts = async_cmd->write_timestamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	// Need to sample a 2-pixel border to have room for AA filter and divot.
	int extract_width = regs.max_x + 2 + 4 + int(divot) * 2;
	// 1 pixel border on top and bottom.
	int extract_height = regs.max_y + 1 + 4;

	Vulkan::ImageCreateInfo rt_info = Vulkan::ImageCreateInfo::render_target(
			extract_width,
			extract_height,
			VK_FORMAT_R8G8B8A8_UINT);

	rt_info.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	rt_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rt_info.misc = Vulkan::IMAGE_MISC_CONCURRENT_QUEUE_GRAPHICS_BIT |
	               Vulkan::IMAGE_MISC_CONCURRENT_QUEUE_ASYNC_COMPUTE_BIT;
	vram_image = device->create_image(rt_info);
	vram_image->set_layout(Vulkan::Layout::General);

	async_cmd->image_barrier(*vram_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
	                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
	                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT);

#ifdef PARALLEL_RDP_SHADER_DIR
	async_cmd->set_program("rdp://extract_vram.comp");
#else
	async_cmd->set_program(shader_bank->extract_vram);
#endif
	async_cmd->set_storage_texture(0, 0, vram_image->get_view());

	if (scaling_factor > 1)
	{
		async_cmd->set_storage_buffer(0, 1, *renderer->get_upscaled_rdram_buffer());
		async_cmd->set_storage_buffer(0, 2, *renderer->get_upscaled_hidden_rdram_buffer());
	}
	else
	{
		async_cmd->set_storage_buffer(0, 1, *rdram, rdram_offset, rdram_size);
		async_cmd->set_storage_buffer(0, 2, *hidden_rdram);
	}

	struct Push
	{
		uint32_t fb_offset;
		uint32_t fb_width;
		int32_t x_offset;
		int32_t y_offset;
		int32_t x_res;
		int32_t y_res;
	} push = {};

	if ((regs.status & VI_CONTROL_TYPE_MASK) == VI_CONTROL_TYPE_RGBA8888_BIT)
		push.fb_offset = regs.vi_offset >> 2;
	else
		push.fb_offset = regs.vi_offset >> 1;

	push.fb_width = regs.vi_width;
	push.x_offset = divot ? -3 : -2;
	push.y_offset = -2;
	push.x_res = extract_width;
	push.y_res = extract_height;

	async_cmd->set_specialization_constant_mask(7);
	async_cmd->set_specialization_constant(0, uint32_t(rdram_size));
	async_cmd->set_specialization_constant(1, regs.status & (VI_CONTROL_TYPE_MASK | VI_CONTROL_META_AA_BIT));
	async_cmd->set_specialization_constant(2, trailing_zeroes(scaling_factor));

	async_cmd->push_constants(&push, 0, sizeof(push));
	async_cmd->dispatch((extract_width + 15) / 16,
	                    (extract_height + 7) / 8,
	                    1);

	// Just enforce an execution barrier here for rendering work in next frame.
	async_cmd->barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
	                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0);

	if (timestamp)
	{
		end_ts = async_cmd->write_timestamp(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		device->register_time_interval("VI GPU", std::move(start_ts), std::move(end_ts), "extract-vram");
	}

	Vulkan::Semaphore sem;
	device->submit(async_cmd, nullptr, 1, &sem);
	device->add_wait_semaphore(Vulkan::CommandBuffer::Type::Generic, std::move(sem),
	                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, true);

	return vram_image;
}

Vulkan::ImageHandle VideoInterface::aa_fetch_stage(Vulkan::CommandBuffer &cmd, Vulkan::Image &vram_image,
                                                   const Registers &regs, unsigned scaling_factor) const
{
	Vulkan::ImageHandle aa_image;
	Vulkan::QueryPoolHandle start_ts, end_ts;
	bool fetch_bug = need_fetch_bug_emulation(regs, scaling_factor);
	bool divot = (regs.status & VI_CONTROL_DIVOT_ENABLE_BIT) != 0;

	// For the AA pass, we need to figure out how many pixels we might need to read.
	int aa_width = regs.max_x + 3 + int(divot) * 2;
	int aa_height = regs.max_y + 2;

	Vulkan::ImageCreateInfo rt_info = Vulkan::ImageCreateInfo::render_target(aa_width, aa_height,
	                                                                         VK_FORMAT_R8G8B8A8_UINT);
	rt_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	rt_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rt_info.layers = fetch_bug ? 2 : 1;
	rt_info.misc = Vulkan::IMAGE_MISC_FORCE_ARRAY_BIT;
	aa_image = device->create_image(rt_info);

	Vulkan::ImageViewCreateInfo view_info = {};
	view_info.image = aa_image.get();
	view_info.view_type = VK_IMAGE_VIEW_TYPE_2D;
	view_info.layers = 1;

	Vulkan::ImageViewHandle aa_primary, aa_secondary;
	view_info.base_layer = 0;
	aa_primary = device->create_image_view(view_info);

	if (fetch_bug)
	{
		view_info.base_layer = 1;
		aa_secondary = device->create_image_view(view_info);
	}

	Vulkan::RenderPassInfo rp;
	rp.color_attachments[0] = aa_primary.get();
	rp.clear_attachments = 0;

	if (fetch_bug)
	{
		rp.color_attachments[1] = aa_secondary.get();
		rp.num_color_attachments = 2;
		rp.store_attachments = 3;
	}
	else
	{
		rp.num_color_attachments = 1;
		rp.store_attachments = 1;
	}

	cmd.image_barrier(*aa_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
	                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	if (timestamp)
		start_ts = cmd.write_timestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	cmd.begin_render_pass(rp);
	cmd.set_opaque_state();

#ifdef PARALLEL_RDP_SHADER_DIR
	cmd.set_program("rdp://fullscreen.vert", "rdp://vi_fetch.frag",
	                {
			                { "DEBUG_ENABLE", debug_channel ? 1 : 0 },
			                { "FETCH_BUG", fetch_bug ? 1 : 0 },
	                });
#else
	cmd.set_program(device->request_program(shader_bank->fullscreen, shader_bank->vi_fetch[int(fetch_bug)]));
#endif

	struct Push
	{
		int32_t x_offset;
		int32_t y_offset;
	} push = {};

	push.x_offset = 2;
	push.y_offset = 2;

	cmd.push_constants(&push, 0, sizeof(push));

	cmd.set_specialization_constant_mask(3);
	cmd.set_specialization_constant(0, uint32_t(rdram_size));
	cmd.set_specialization_constant(1,
	                                regs.status & (VI_CONTROL_META_AA_BIT | VI_CONTROL_DITHER_FILTER_ENABLE_BIT));

	cmd.set_texture(0, 0, vram_image.get_view());
	cmd.draw(3);
	cmd.end_render_pass();

	if (timestamp)
	{
		end_ts = cmd.write_timestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		device->register_time_interval("VI GPU", std::move(start_ts), std::move(end_ts), "vi-fetch");
	}

	cmd.image_barrier(*aa_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

	return aa_image;
}

Vulkan::ImageHandle VideoInterface::divot_stage(Vulkan::CommandBuffer &cmd, Vulkan::Image &aa_image,
                                                const Registers &regs, unsigned scaling_factor) const
{
	Vulkan::ImageHandle divot_image;
	Vulkan::QueryPoolHandle start_ts, end_ts;
	bool fetch_bug = need_fetch_bug_emulation(regs, scaling_factor);

	// For the divot pass, we need to figure out how many pixels we might need to read.
	int divot_width = regs.max_x + 2;
	int divot_height = regs.max_y + 2;

	Vulkan::ImageCreateInfo rt_info = Vulkan::ImageCreateInfo::render_target(divot_width, divot_height,
	                                                                         VK_FORMAT_R8G8B8A8_UINT);
	rt_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	rt_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rt_info.layers = fetch_bug ? 2 : 1;
	rt_info.misc = Vulkan::IMAGE_MISC_FORCE_ARRAY_BIT;
	divot_image = device->create_image(rt_info);

	Vulkan::ImageViewCreateInfo view_info = {};
	view_info.image = divot_image.get();
	view_info.view_type = VK_IMAGE_VIEW_TYPE_2D;
	view_info.layers = 1;

	Vulkan::ImageViewHandle divot_primary, divot_secondary;
	view_info.base_layer = 0;
	divot_primary = device->create_image_view(view_info);

	if (fetch_bug)
	{
		view_info.base_layer = 1;
		divot_secondary = device->create_image_view(view_info);
	}

	Vulkan::RenderPassInfo rp;
	rp.color_attachments[0] = divot_primary.get();
	rp.clear_attachments = 0;

	if (fetch_bug)
	{
		rp.color_attachments[1] = divot_secondary.get();
		rp.num_color_attachments = 2;
		rp.store_attachments = 3;
	}
	else
	{
		rp.num_color_attachments = 1;
		rp.store_attachments = 1;
	}

	cmd.image_barrier(*divot_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
	                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	if (timestamp)
		start_ts = cmd.write_timestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	cmd.begin_render_pass(rp);
	cmd.set_opaque_state();

#ifdef PARALLEL_RDP_SHADER_DIR
	cmd.set_program("rdp://fullscreen.vert", "rdp://vi_divot.frag", {
			{ "DEBUG_ENABLE", debug_channel ? 1 : 0 },
			{ "FETCH_BUG", fetch_bug ? 1 : 0 },
	});
#else
	cmd.set_program(device->request_program(shader_bank->fullscreen, shader_bank->vi_divot[int(fetch_bug)]));
#endif

	cmd.set_texture(0, 0, aa_image.get_view());
	cmd.draw(3);
	cmd.end_render_pass();

	if (timestamp)
	{
		end_ts = cmd.write_timestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		device->register_time_interval("VI GPU", std::move(start_ts), std::move(end_ts), "vi-divot");
	}

	cmd.image_barrier(*divot_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

	return divot_image;
}

void VideoInterface::bind_horizontal_info_view(Vulkan::CommandBuffer &cmd, const HorizontalInfoLines &lines)
{
	auto &device = cmd.get_device();

	Vulkan::BufferCreateInfo horizontal_buffer_info = {};
	horizontal_buffer_info.size = sizeof(lines);
	horizontal_buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	horizontal_buffer_info.domain = Vulkan::BufferDomain::LinkedDeviceHost;
	auto scanout_parameters = device.create_buffer(horizontal_buffer_info, &lines);

	Vulkan::BufferViewCreateInfo horizontal_view_info = {};
	horizontal_view_info.format = VK_FORMAT_R32G32B32A32_SINT;
	horizontal_view_info.buffer = scanout_parameters.get();
	horizontal_view_info.range = sizeof(lines);

	auto scanout_parameters_view = device.create_buffer_view(horizontal_view_info);
	cmd.set_buffer_view(0, 1, *scanout_parameters_view);
}

Vulkan::ImageHandle VideoInterface::scale_stage(Vulkan::CommandBuffer &cmd, const Vulkan::Image *divot_image,
                                                Registers regs, const HorizontalInfoLines &lines,
                                                unsigned scaling_factor, bool degenerate,
                                                const ScanoutOptions &options, bool final_pass) const
{
	Vulkan::ImageHandle scale_image;
	Vulkan::QueryPoolHandle start_ts, end_ts;
	bool fetch_bug = need_fetch_bug_emulation(regs, scaling_factor);
	bool serrate = (regs.status & VI_CONTROL_SERRATE_BIT) != 0 && !options.upscale_deinterlacing;

	Vulkan::ImageCreateInfo rt_info = Vulkan::ImageCreateInfo::render_target(
			VI_SCANOUT_WIDTH * scaling_factor,
			((regs.is_pal ? VI_V_RES_PAL: VI_V_RES_NTSC) >> int(!serrate)) * scaling_factor,
			VK_FORMAT_R8G8B8A8_UNORM);

	unsigned crop_left = 0;
	unsigned crop_right = 0;
	unsigned crop_top = 0;
	unsigned crop_bottom = 0;

	if (options.crop_rect.enable)
	{
		crop_left = options.crop_rect.left;
		crop_right = options.crop_rect.right;
		crop_top = options.crop_rect.top;
		crop_bottom = options.crop_rect.bottom;

		if (serrate)
		{
			crop_top *= 2;
			crop_bottom *= 2;
		}
	}
	else
	{
		// Rescale crop pixels to preserve aspect ratio.
		auto crop_pixels_y = options.crop_overscan_pixels * (serrate ? 2 : 1);
		auto crop_pixels_x = unsigned(std::round(float(crop_pixels_y) * (float(rt_info.width) / float(rt_info.height))));

		crop_left = crop_right = crop_pixels_x;
		crop_top = crop_bottom = crop_pixels_y;
	}

	crop_left *= scaling_factor;
	crop_right *= scaling_factor;
	crop_top *= scaling_factor;
	crop_bottom *= scaling_factor;

	if (crop_left + crop_right < rt_info.width && crop_top + crop_bottom < rt_info.height)
	{
		rt_info.width -= crop_left + crop_right;
		rt_info.height -= crop_top + crop_bottom;
	}
	else
	{
		LOGE("Too large crop of %u x %u for RT %u x %u.\n",
			 crop_left + crop_right, crop_top + crop_bottom, rt_info.width, rt_info.height);
	}

	rt_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	rt_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rt_info.misc = Vulkan::IMAGE_MISC_MUTABLE_SRGB_BIT;

	if (options.export_scanout && final_pass)
	{
		rt_info.misc |= Vulkan::IMAGE_MISC_EXTERNAL_MEMORY_BIT;
		rt_info.external.memory_handle_type = options.export_handle_type;
	}

	scale_image = device->create_image(rt_info);

	if (!scale_image)
	{
		LOGE("Failed to allocate scale image.\n");
		return {};
	}

	Vulkan::RenderPassInfo rp;
	rp.color_attachments[0] = &scale_image->get_view();
	memset(&rp.clear_color[0], 0, sizeof(rp.clear_color[0]));
	rp.num_color_attachments = 1;
	rp.clear_attachments = 1;
	rp.store_attachments = 1;

	cmd.image_barrier(*scale_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
	                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	if (prev_scanout_image)
	{
		if (prev_image_is_external)
		{
			cmd.acquire_external_image_barrier(*prev_scanout_image, prev_image_layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
											   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
		}
		else if (prev_image_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			VK_ASSERT(prev_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			cmd.image_barrier(*prev_scanout_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			                  VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
		}
	}

	if (timestamp)
		start_ts = cmd.write_timestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	cmd.begin_render_pass(rp);

	cmd.set_specialization_constant_mask((1 << 1) | (1 << 2));
	cmd.set_specialization_constant(1,
	                                regs.status & (VI_CONTROL_GAMMA_ENABLE_BIT |
	                                               VI_CONTROL_GAMMA_DITHER_ENABLE_BIT |
	                                               VI_CONTROL_META_SCALE_BIT |
	                                               VI_CONTROL_META_AA_BIT));
	cmd.set_specialization_constant(2, uint32_t(fetch_bug));

	struct Push
	{
		int32_t h_offset, v_offset;
		int32_t v_start;
		uint32_t y_add;
		uint32_t frame_count;

		uint32_t serrate_shift;
		uint32_t serrate_mask;
		uint32_t serrate_select;

		uint32_t info_y_shift;
	} push = {};

	push.info_y_shift = Util::floor_log2(scaling_factor);

	if (serrate)
	{
		regs.v_start *= 2;
		regs.v_res *= 2;
		push.serrate_shift = 1;
		push.serrate_mask = 1;
		bool field_state = regs.v_current_line == 0;
		push.serrate_select = int(field_state);
		push.info_y_shift++;
	}

	push.h_offset = int(crop_left);
	push.v_offset = int(crop_top);
	push.v_start = regs.v_start;
	push.y_add = regs.init_y_add;
	push.frame_count = frame_count;

	cmd.set_opaque_state();
#ifdef PARALLEL_RDP_SHADER_DIR
	cmd.set_program("rdp://fullscreen.vert", "rdp://vi_scale.frag", {
			{ "DEBUG_ENABLE", debug_channel ? 1 : 0 },
	});
#else
	cmd.set_program(device->request_program(shader_bank->fullscreen, shader_bank->vi_scale));
#endif
	cmd.set_buffer_view(1, 0, *gamma_lut_view);
	bind_horizontal_info_view(cmd, lines);

	cmd.push_constants(&push, 0, sizeof(push));

	const auto shift_rect = [](VkRect2D &rect, int x, int y) {
		rect.offset.x += x;
		rect.offset.y += y;

		if (rect.offset.x < 0)
		{
			rect.extent.width += rect.offset.x;
			rect.offset.x = 0;
		}

		if (rect.offset.y < 0)
		{
			rect.extent.height += rect.offset.y;
			rect.offset.y = 0;
		}

		// Check for signed overflow without relying on -fwrapv.
		if (rect.extent.width & 0x80000000u)
			rect.extent.width = 0;
		if (rect.extent.height & 0x80000000u)
			rect.extent.height = 0;
	};

	if (!degenerate && divot_image && regs.h_res > 0 && regs.v_res > 0)
	{
		VkRect2D rect = {{ regs.h_start, regs.v_start }, { uint32_t(regs.h_res), uint32_t(regs.v_res) }};
		shift_rect(rect, -int(crop_left), -int(crop_top));

		if (rect.extent.width > 0 && rect.extent.height > 0)
		{
			cmd.set_texture(0, 0, divot_image->get_view());
			cmd.set_scissor(rect);
			cmd.draw(3);
		}
	}

	// To deal with weave interlacing and other "persistence effects", we blend in previous frame's result.
	// This is somewhat arbitrary, but seems to work well enough in practice.

	if (prev_scanout_image && options.blend_previous_frame)
	{
		cmd.set_blend_enable(true);
		cmd.set_blend_factors(VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_DST_ALPHA);
		// Don't overwrite alpha, it's already zero.
		cmd.set_color_write_mask(0x7);
		cmd.set_specialization_constant_mask(0);
		cmd.set_texture(0, 0, prev_scanout_image->get_view());
#ifdef PARALLEL_RDP_SHADER_DIR
		cmd.set_program("rdp://fullscreen.vert", "rdp://vi_blend_fields.frag", {
				{ "DEBUG_ENABLE", debug_channel ? 1 : 0 },
		});
#else
		cmd.set_program(device->request_program(shader_bank->fullscreen, shader_bank->vi_blend_fields));
#endif

		if (degenerate)
		{
			if (regs.h_res > 0)
			{
				VkRect2D rect = {{ regs.h_start, 0 }, { uint32_t(regs.h_res), prev_scanout_image->get_height() }};
				shift_rect(rect, -int(crop_left), -int(crop_top));
				if (rect.extent.width > 0 && rect.extent.height > 0)
				{
					cmd.set_scissor(rect);
					cmd.draw(3);
				}
			}
		}
		else
		{
			// Top part.
			if (regs.h_res > 0 && regs.v_start > 0)
			{
				VkRect2D rect = {{ regs.h_start, 0 }, { uint32_t(regs.h_res), uint32_t(regs.v_start) }};
				shift_rect(rect, -int(crop_left), -int(crop_top));
				if (rect.extent.width > 0 && rect.extent.height > 0)
				{
					cmd.set_scissor(rect);
					cmd.draw(3);
				}
			}

			// Middle part, don't overwrite the 8 pixel guard band.
			if (regs.h_res_clamp > 0 && regs.v_res > 0)
			{
				VkRect2D rect = {{ regs.h_start_clamp, regs.v_start }, { uint32_t(regs.h_res_clamp), uint32_t(regs.v_res) }};
				shift_rect(rect, -int(crop_left), -int(crop_top));
				if (rect.extent.width > 0 && rect.extent.height > 0)
				{
					cmd.set_scissor(rect);
					cmd.draw(3);
				}
			}

			// Bottom part.
			if (regs.h_res > 0 && prev_scanout_image->get_height() > uint32_t(regs.v_start + regs.v_res))
			{
				VkRect2D rect = {{ regs.h_start, regs.v_start + regs.v_res },
				                 { uint32_t(regs.h_res), prev_scanout_image->get_height() - uint32_t(regs.v_start + regs.v_res) }};
				shift_rect(rect, -int(crop_left), -int(crop_top));
				if (rect.extent.width > 0 && rect.extent.height > 0)
				{
					cmd.set_scissor(rect);
					cmd.draw(3);
				}
			}
		}
	}

	cmd.end_render_pass();

	if (timestamp)
	{
		end_ts = cmd.write_timestamp(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		device->register_time_interval("VI GPU", std::move(start_ts), std::move(end_ts), "vi-scale");
	}

	return scale_image;
}

Vulkan::ImageHandle VideoInterface::downscale_stage(Vulkan::CommandBuffer &cmd, Vulkan::Image &scale_image,
                                                    unsigned scaling_factor, unsigned downscale_steps,
                                                    const ScanoutOptions &options, bool final_pass) const
{
	Vulkan::ImageHandle downscale_image;
	const Vulkan::Image *input = &scale_image;
	Vulkan::ImageHandle holder;

	bool need_pass = scaling_factor > 1 && downscale_steps;

	// TODO: Could optimize this to happen in one pass, but ... eh.
	while (need_pass)
	{
		if (input != &scale_image)
		{
			cmd.image_barrier(*input, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
		}

		unsigned width = input->get_width();
		unsigned height = input->get_height();

		Vulkan::ImageCreateInfo rt_info = Vulkan::ImageCreateInfo::render_target(
				width / 2, height / 2,
				VK_FORMAT_R8G8B8A8_UNORM);

		rt_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		rt_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		rt_info.misc = Vulkan::IMAGE_MISC_MUTABLE_SRGB_BIT;

		scaling_factor /= 2;
		downscale_steps--;
		need_pass = scaling_factor > 1 && downscale_steps;

		if (options.export_scanout && final_pass && !need_pass)
		{
			rt_info.misc |= Vulkan::IMAGE_MISC_EXTERNAL_MEMORY_BIT;
			rt_info.external.memory_handle_type = options.export_handle_type;
		}

		downscale_image = device->create_image(rt_info);

		if (!downscale_image)
		{
			LOGE("Failed to allocate downscale image.\n");
			return {};
		}

		cmd.image_barrier(*downscale_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
		                  VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

		cmd.blit_image(*downscale_image, *input,
		               {}, {int(rt_info.width), int(rt_info.height), 1},
		               {}, {int(width), int(height), 1},
		               0, 0);

		input = downscale_image.get();
		holder = downscale_image;
	}

	return downscale_image;
}

Vulkan::ImageHandle VideoInterface::upscale_deinterlace(Vulkan::CommandBuffer &cmd, Vulkan::Image &scale_image,
                                                        unsigned scaling_factor, bool field_select,
                                                        const ScanoutOptions &options) const
{
	Vulkan::ImageHandle deinterlaced_image;

	// If we're running upscaled, upscaling Y further is somewhat meaningless and bandwidth intensive.
	Vulkan::ImageCreateInfo rt_info = Vulkan::ImageCreateInfo::render_target(
			scale_image.get_width(), scale_image.get_height() * (scaling_factor == 1 ? 2 : 1),
			VK_FORMAT_R8G8B8A8_UNORM);

	rt_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	rt_info.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	rt_info.misc = Vulkan::IMAGE_MISC_MUTABLE_SRGB_BIT;

	if (options.export_scanout)
	{
		rt_info.misc |= Vulkan::IMAGE_MISC_EXTERNAL_MEMORY_BIT;
		rt_info.external.memory_handle_type = options.export_handle_type;
	}

	deinterlaced_image = device->create_image(rt_info);

	if (!deinterlaced_image)
	{
		LOGE("Failed to allocate deinterlace image.\n");
		return {};
	}

	Vulkan::RenderPassInfo rp;
	rp.color_attachments[0] = &deinterlaced_image->get_view();
	rp.num_color_attachments = 1;
	rp.store_attachments = 1;

	cmd.image_barrier(*deinterlaced_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
	                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	cmd.begin_render_pass(rp);
	cmd.set_opaque_state();

	struct Push
	{
		float y_offset;
	} push = {};
	push.y_offset = (float(scaling_factor) * (field_select ? -0.25f : +0.25f)) / float(scale_image.get_height());
	cmd.push_constants(&push, 0, sizeof(push));

#ifdef PARALLEL_RDP_SHADER_DIR
	cmd.set_program("rdp://vi_deinterlace.vert", "rdp://vi_deinterlace.frag", {
		{ "DEBUG_ENABLE", debug_channel ? 1 : 0 },
	});
#else
	cmd.set_program(device->request_program(shader_bank->vi_deinterlace_vert, shader_bank->vi_deinterlace_frag));
#endif
	cmd.set_texture(0, 0, scale_image.get_view(), Vulkan::StockSampler::LinearClamp);
	cmd.draw(3);
	cmd.end_render_pass();
	return deinterlaced_image;
}

void VideoInterface::begin_vi_register_per_scanline(PerScanlineRegisterFlags flags)
{
	per_line_state.flags = flags;
	per_line_state.h_start.latched_state = vi_registers[unsigned(VIRegister::HStart)];
	per_line_state.x_scale.latched_state = vi_registers[unsigned(VIRegister::XScale)];
	per_line_state.h_start.line_state[0] = vi_registers[unsigned(VIRegister::HStart)];
	per_line_state.x_scale.line_state[0] = vi_registers[unsigned(VIRegister::XScale)];
	per_line_state.line = 0;
	per_line_state.ended = false;
}

void VideoInterface::set_vi_register_for_scanline(PerScanlineRegisterBits reg, uint32_t value)
{
	if ((per_line_state.flags & reg) == 0)
	{
		LOGW("Attempting to set VI register %u per scanline, "
		     "but was not flagged in begin_vi_register_per_scanline, ignoring.\n", reg);
		return;
	}

	switch (reg)
	{
	case PER_SCANLINE_HSTART_BIT:
		per_line_state.h_start.latched_state = value;
		break;

	case PER_SCANLINE_XSCALE_BIT:
		per_line_state.x_scale.latched_state = value;
		break;

	default:
		break;
	}
}

void VideoInterface::latch_vi_register_for_scanline(unsigned vi_line)
{
	vi_line = std::min(vi_line, VI_V_END_MAX - 1);

	if (vi_line <= per_line_state.line)
	{
		LOGW("Ignoring vi_line %u, current line is %u, not monotonically increasing, ignoring.\n",
			 vi_line, per_line_state.line);
		return;
	}

	unsigned new_counter = per_line_state.line;

	while (++new_counter < vi_line)
	{
		per_line_state.h_start.line_state[new_counter] = per_line_state.h_start.line_state[per_line_state.line];
		per_line_state.x_scale.line_state[new_counter] = per_line_state.x_scale.line_state[per_line_state.line];
	}

	per_line_state.h_start.line_state[new_counter] = per_line_state.h_start.latched_state;
	per_line_state.x_scale.line_state[new_counter] = per_line_state.x_scale.latched_state;
	per_line_state.line = new_counter;
}

void VideoInterface::clear_per_scanline_state()
{
	per_line_state.flags = 0;
	per_line_state.ended = false;
}

void VideoInterface::end_vi_register_per_scanline()
{
	if (per_line_state.flags == 0)
	{
		LOGW("Cannot end vi_register_per_scanline() with per line flags == 0, ignoring.\n");
		return;
	}

	if (per_line_state.ended)
	{
		LOGW("Already ended per line register state, ignoring.\n");
		return;
	}

	unsigned new_counter = per_line_state.line;
	while (++new_counter < VI_V_END_MAX)
	{
		per_line_state.h_start.line_state[new_counter] = per_line_state.h_start.line_state[per_line_state.line];
		per_line_state.x_scale.line_state[new_counter] = per_line_state.x_scale.line_state[per_line_state.line];
	}

	per_line_state.ended = true;
}

Vulkan::ImageHandle VideoInterface::scanout(VkImageLayout target_layout, const ScanoutOptions &options, unsigned scaling_factor_)
{
	unsigned downscale_steps = std::min(8u, options.downscale_steps);
	int scaling_factor = int(scaling_factor_);
	Vulkan::ImageHandle scanout;
	HorizontalInfoLines lines;

	auto regs = decode_vi_registers(&lines);
	clear_per_scanline_state();

	if (regs.vi_offset == 0)
	{
		prev_scanout_image.reset();
		return scanout;
	}

	if (!options.vi.serrate)
		regs.status &= ~VI_CONTROL_SERRATE_BIT;

	bool status_is_aa = (regs.status & VI_CONTROL_AA_MODE_MASK) < VI_CONTROL_AA_MODE_RESAMP_ONLY_BIT;
	bool status_is_bilinear = (regs.status & VI_CONTROL_AA_MODE_MASK) < VI_CONTROL_AA_MODE_RESAMP_REPLICATE_BIT;

	status_is_aa = status_is_aa && options.vi.aa;
	status_is_bilinear = status_is_bilinear && options.vi.scale;

	regs.status &= ~(VI_CONTROL_AA_MODE_MASK | VI_CONTROL_META_AA_BIT | VI_CONTROL_META_SCALE_BIT);
	if (status_is_aa)
		regs.status |= VI_CONTROL_META_AA_BIT;
	if (status_is_bilinear)
		regs.status |= VI_CONTROL_META_SCALE_BIT;

	if (!options.vi.gamma_dither)
		regs.status &= ~VI_CONTROL_GAMMA_DITHER_ENABLE_BIT;
	if (!options.vi.divot_filter)
		regs.status &= ~VI_CONTROL_DIVOT_ENABLE_BIT;
	if (!options.vi.dither_filter)
		regs.status &= ~VI_CONTROL_DITHER_FILTER_ENABLE_BIT;

	bool is_blank = (regs.status & VI_CONTROL_TYPE_RGBA5551_BIT) == 0;
	if (is_blank && previous_frame_blank)
	{
		frame_count++;
		prev_scanout_image.reset();
		return scanout;
	}

	if (is_blank)
		prev_scanout_image.reset();

	regs.status |= VI_CONTROL_TYPE_RGBA5551_BIT;
	previous_frame_blank = is_blank;

	bool divot = (regs.status & VI_CONTROL_DIVOT_ENABLE_BIT) != 0;

	if (regs.h_res <= 0 || regs.h_start >= VI_SCANOUT_WIDTH)
	{
		frame_count++;

		// A dirty hack to make it work for games which strobe the invalid state (but expect the image to persist),
		// and games which legitimately render invalid frames for long stretches where a black screen is expected.
		if (options.persist_frame_on_invalid_input && (frame_count - last_valid_frame_count < 4))
		{
			scanout = prev_scanout_image;

			if (scanout && prev_image_layout != target_layout)
			{
				auto cmd = device->request_command_buffer();
				cmd->image_barrier(*scanout, prev_image_layout, target_layout,
				                   layout_to_stage(prev_image_layout), 0,
				                   layout_to_stage(target_layout), layout_to_access(target_layout));
				prev_image_layout = target_layout;
				device->submit(cmd);
			}
		}
		else
			prev_scanout_image.reset();

		return scanout;
	}

	last_valid_frame_count = frame_count;

	bool degenerate = regs.h_res <= 0 || regs.v_res <= 0;

	regs.h_start *= scaling_factor;
	regs.h_start_clamp *= scaling_factor;
	regs.v_start *= scaling_factor;

	regs.h_res *= scaling_factor;
	regs.h_res_clamp *= scaling_factor;
	regs.v_res *= scaling_factor;

	regs.max_x = regs.max_x * scaling_factor + (scaling_factor - 1);
	regs.max_y = regs.max_y * scaling_factor + (scaling_factor - 1);

	for (auto &line : lines.lines)
	{
		line.h_start *= scaling_factor;
		line.h_start_clamp *= scaling_factor;
		line.h_end_clamp *= scaling_factor;
		line.x_start *= scaling_factor;
		line.y_start *= scaling_factor;
		line.y_base *= scaling_factor;
	}

	// First we copy data out of VRAM into a texture which we will then perform our post-AA on.
	// We do this on the async queue so we don't have to stall async queue on graphics work to deal with WAR hazards.
	// After the copy, we can immediately begin rendering new frames while we do post in parallel.
	Vulkan::ImageHandle vram_image;
	if (!degenerate)
		vram_image = vram_fetch_stage(regs, scaling_factor);

	auto cmd = device->request_command_buffer();

	if (debug_channel)
		cmd->begin_debug_channel(this, "VI", 32 * 1024 * 1024);

	// In the first pass, we need to read from VRAM and apply the fetch filter.
	// This is either the AA filter if coverage < 7, or the dither reconstruction filter if coverage == 7 and enabled.
	// Following that, post-AA filter, we have the divot filter.
	// In this filter, we need to find the median value of three horizontal pixels, post AA if any of them have coverage < 7.
	// Finally, we lerp the result based on x_add and y_add, and then, apply gamma/dither on top as desired.

	// AA -> divot could probably be done with compute and shared memory, but ideally this is done in fragment shaders in this implementation
	// so that we can run higher-priority compute shading workload async in the async queue.
	// We also get to take advantage of framebuffer compression FWIW.

	Vulkan::ImageHandle aa_image;
	if (!degenerate)
		aa_image = aa_fetch_stage(*cmd, *vram_image, regs, scaling_factor);

	// Divot pass
	Vulkan::ImageHandle divot_image;
	if (divot && !degenerate)
		divot_image = divot_stage(*cmd, *aa_image, regs, scaling_factor);
	else
		divot_image = std::move(aa_image);

	// Scale pass
	bool is_final_pass = !downscale_steps || scaling_factor <= 1;
	bool serrate = (regs.status & VI_CONTROL_SERRATE_BIT) != 0;

	auto scale_image = scale_stage(*cmd, divot_image.get(),
	                               regs, lines,
	                               scaling_factor, degenerate, options,
	                               is_final_pass);

	auto src_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (!is_final_pass && scale_image)
	{
		cmd->image_barrier(*scale_image, src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                   layout_to_stage(src_layout), layout_to_access(src_layout),
		                   VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);

		is_final_pass = !serrate || !options.upscale_deinterlacing;

		scale_image = downscale_stage(*cmd, *scale_image, scaling_factor, downscale_steps,
									  options, is_final_pass);

		src_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	if (!is_final_pass && scale_image)
	{
		cmd->image_barrier(*scale_image, src_layout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		                   layout_to_stage(src_layout), layout_to_access(src_layout),
		                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

		bool field_state = regs.v_current_line == 0;
		scale_image = upscale_deinterlace(*cmd, *scale_image,
		                                  std::max(1, scaling_factor >> downscale_steps),
		                                  field_state, options);
		src_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	if (scale_image)
	{
		if (options.export_scanout)
		{
			// Foreign handle types (e.g. D3D) must use GENERAL layouts.
			if (options.export_handle_type != Vulkan::ExternalHandle::get_opaque_memory_handle_type())
				target_layout = VK_IMAGE_LAYOUT_GENERAL;

			cmd->release_external_image_barrier(*scale_image, src_layout,
			                                    target_layout,
			                                    layout_to_stage(src_layout),
			                                    layout_to_access(src_layout));
		}
		else
		{
			cmd->image_barrier(*scale_image, src_layout, target_layout,
			                   layout_to_stage(src_layout), layout_to_access(src_layout),
			                   layout_to_stage(target_layout), layout_to_access(target_layout));
		}
	}

	prev_image_layout = target_layout;
	prev_scanout_image = scale_image;
	prev_image_is_external = options.export_scanout;

	if (options.persist_frame_on_invalid_input && options.export_scanout)
	{
		LOGE("persist_frame_on_invalid_input cannot be combined with export_scanout.\n");
		prev_scanout_image.reset();
	}

	device->submit(cmd);
	scanout = std::move(scale_image);
	frame_count++;
	return scanout;
}
}
