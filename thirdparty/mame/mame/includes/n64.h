// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
#ifndef MAME_INCLUDES_N64_H
#define MAME_INCLUDES_N64_H

#pragma once

#if defined(MAME_RDP)
#include "video/n64.h"

class running_machine
{
public:
	running_machine(n64_state* state)
		: m_state(state), m_rand_seed(0x9d14abd7)
	{ }

	template <class DriverClass> DriverClass *driver_data() const;

	u32 rand()
	{
		m_rand_seed = 1664525 * m_rand_seed + 1013904223;

		// return rotated by 16 bits; the low bits have a short period
		// and are frequently used
		return (m_rand_seed >> 16) | (m_rand_seed << 16);
	}

private:
	n64_state* m_state;
	u32 m_rand_seed; // current random number seed
};

template <> inline n64_state *running_machine::driver_data() const { return m_state; }

class n64_state
{
public:
	n64_state(uint32_t* rdram, uint32_t* rsp_dmem, n64_periphs* rcp_periphs)
		: m_machine(this), m_rdram(rdram), m_rsp_dmem(rsp_dmem), m_rcp_periphs(rcp_periphs)
	{ }

	void video_start();

	// Getters
	n64_rdp* rdp() { return m_rdp.get(); }

	running_machine& machine() { return m_machine; }

protected:
	running_machine m_machine;

	uint32_t* m_rdram;
	uint32_t* m_rsp_dmem;

	n64_periphs* m_rcp_periphs;

	/* video-related */
	std::unique_ptr<n64_rdp> m_rdp;
};

class n64_periphs
{
public:
	virtual void dp_full_sync() = 0;
};
#else
#include "cpu/rsp/rsp.h"
#include "cpu/mips/mips3.h"
#include "sound/dmadac.h"
#include "video/n64.h"

/*----------- driver state -----------*/

class n64_rdp;
class n64_periphs;

class n64_state : public driver_device
{
public:
	n64_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_vr4300(*this, "maincpu")
		, m_rsp(*this, "rsp")
		, m_sram(*this, "sram")
		, m_rdram(*this, "rdram")
		, m_rsp_imem(*this, "rsp_imem")
		, m_rsp_dmem(*this, "rsp_dmem")
		, m_rcp_periphs(*this, "rcp")
	{
	}

	virtual void machine_start() override;
	virtual void machine_reset() override;
	virtual void video_start() override;
	void n64_machine_stop();

	uint32_t screen_update_n64(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	DECLARE_WRITE_LINE_MEMBER(screen_vblank_n64);

	// Getters
	n64_rdp* rdp() { return m_rdp.get(); }
	uint32_t* rdram() { return m_rdram; }
	uint32_t* sram() { return m_sram; }

protected:
	required_device<mips3_device> m_vr4300;
	required_device<rsp_device> m_rsp;

	optional_shared_ptr<uint32_t> m_sram;
	required_shared_ptr<uint32_t> m_rdram;
	required_shared_ptr<uint32_t> m_rsp_imem;
	required_shared_ptr<uint32_t> m_rsp_dmem;

	required_device<n64_periphs> m_rcp_periphs;

	/* video-related */
	std::unique_ptr<n64_rdp> m_rdp;
};

/*----------- devices -----------*/

#define AUDIO_DMA_DEPTH     2

struct n64_savable_data_t
{
	uint8_t sram[0x20000];
	uint8_t eeprom[2048];
	uint8_t mempak[2][0x8000];
};

class n64_periphs : public device_t,
					public device_video_interface
{
private:
	struct AUDIO_DMA
	{
		uint32_t address;
		uint32_t length;
	};

public:
	// construction/destruction
	n64_periphs(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	uint32_t is64_r(offs_t offset);
	void is64_w(offs_t offset, uint32_t data);
	uint32_t open_r(offs_t offset);
	void open_w(uint32_t data);
	uint32_t rdram_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void rdram_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t mi_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void mi_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t vi_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void vi_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t ai_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void ai_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t pi_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void pi_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t ri_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void ri_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t si_reg_r(offs_t offset);
	void si_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t dd_reg_r(offs_t offset);
	void dd_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t pif_ram_r(offs_t offset, uint32_t mem_mask = ~0);
	void pif_ram_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	TIMER_CALLBACK_MEMBER(reset_timer_callback);
	TIMER_CALLBACK_MEMBER(vi_scanline_callback);
	TIMER_CALLBACK_MEMBER(dp_delay_callback);
	TIMER_CALLBACK_MEMBER(ai_timer_callback);
	TIMER_CALLBACK_MEMBER(pi_dma_callback);
	TIMER_CALLBACK_MEMBER(si_dma_callback);
	uint32_t dp_reg_r(offs_t offset, uint32_t mem_mask = ~0);
	void dp_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t sp_reg_r(offs_t offset);
	void sp_reg_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	void sp_set_status(uint32_t data);
	void signal_rcp_interrupt(int interrupt);
	void check_interrupts();

	void dp_full_sync();
	void ai_timer_tick();
	void pi_dma_tick();
	void si_dma_tick();
	void reset_tick();
	void video_update(bitmap_rgb32 &bitmap);

	// Video Interface (VI) registers
	uint32_t vi_width;
	uint32_t vi_origin;
	uint32_t vi_control;
	uint32_t vi_blank;
	uint32_t vi_hstart;
	uint32_t vi_vstart;
	uint32_t vi_xscale;
	uint32_t vi_yscale;
	uint32_t vi_burst;
	uint32_t vi_vsync;
	uint32_t vi_hsync;
	uint32_t vi_leap;
	uint32_t vi_intr;
	uint32_t vi_vburst;
	uint8_t field;

	// nvram-specific for the console
	device_t *m_nvram_image;

	n64_savable_data_t m_save_data;

	uint32_t cart_length;

	bool dd_present;
	bool disk_present;
	bool cart_present;

	// Mouse X2/Y2 for delta position
	int mouse_x2[4];
	int mouse_y2[4];

	void poll_reset_button(bool button);

	uint32_t dp_clock;

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

private:
	n64_state* m_n64;
	address_space *m_mem_map;
	required_device<mips3_device> m_vr4300;
	required_device<rsp_device> m_rsp;
	required_shared_ptr<uint32_t> m_rsp_imem;
	required_shared_ptr<uint32_t> m_rsp_dmem;

	uint32_t *m_rdram;
	uint32_t *m_sram;

	void clear_rcp_interrupt(int interrupt);

	bool reset_held;
	emu_timer *reset_timer;
	emu_timer *dp_delay_timer;

	uint8_t is64_buffer[0x10000];

	// Video interface (VI) registers and functions
	emu_timer *vi_scanline_timer;

	// Audio Interface (AI) registers and functions
	void ai_dma();
	AUDIO_DMA *ai_fifo_get_top();
	void ai_fifo_push(uint32_t address, uint32_t length);
	void ai_fifo_pop();
	bool ai_delayed_carry;

	required_device_array<dmadac_sound_device, 2> ai_dac;
	uint32_t ai_dram_addr;
	uint32_t ai_len;
	uint32_t ai_control;
	int ai_dacrate;
	int ai_bitrate;
	uint32_t ai_status;

	emu_timer *ai_timer;

	AUDIO_DMA ai_fifo[AUDIO_DMA_DEPTH];
	int ai_fifo_wpos;
	int ai_fifo_rpos;
	int ai_fifo_num;

	// Memory Interface (MI) registers
	uint32_t mi_version;
	uint32_t mi_interrupt;
	uint32_t mi_intr_mask;
	uint32_t mi_mode;

	// RDRAM Interface (RI) registers
	uint32_t rdram_regs[10];
	uint32_t ri_regs[8];

	// RSP Interface (SP) registers
	void sp_dma(int direction);

	uint32_t sp_mem_addr;
	uint32_t sp_dram_addr;
	uint32_t sp_mem_addr_start;
	uint32_t sp_dram_addr_start;
	int sp_dma_length;
	int sp_dma_count;
	int sp_dma_skip;
	uint32_t sp_semaphore;

	// Disk Drive (DD) registers and functions
	void dd_set_zone_and_track_offset();
	void dd_update_bm();
	void dd_write_sector();
	void dd_read_sector();
	void dd_read_C2();
	uint32_t dd_buffer[256];
	uint32_t dd_sector_data[64];
	uint32_t dd_ram_seq_data[16];
	uint32_t dd_data_reg;
	uint32_t dd_status_reg;
	uint32_t dd_track_reg;
	uint32_t dd_buf_status_reg;
	uint32_t dd_sector_err_reg;
	uint32_t dd_seq_status_reg;
	uint32_t dd_seq_ctrl_reg;
	uint32_t dd_sector_reg;
	uint32_t dd_reset_reg;
	uint32_t dd_current_reg;
	bool dd_bm_reset_held;
	bool dd_write;
	uint8_t dd_int;
	uint8_t dd_start_block;
	uint8_t dd_start_sector;
	uint8_t dd_sectors_per_block;
	uint8_t dd_sector_size;
	uint8_t dd_zone;
	uint32_t dd_track_offset;

	// Peripheral Interface (PI) registers and functions
	emu_timer *pi_dma_timer;
	uint32_t pi_dram_addr;
	uint32_t pi_cart_addr;
	uint32_t pi_rd_len;
	uint32_t pi_wr_len;
	uint32_t pi_status;
	uint32_t pi_bsd_dom1_lat;
	uint32_t pi_bsd_dom1_pwd;
	uint32_t pi_bsd_dom1_pgs;
	uint32_t pi_bsd_dom1_rls;
	uint32_t pi_bsd_dom2_lat;
	uint32_t pi_bsd_dom2_pwd;
	uint32_t pi_bsd_dom2_pgs;
	uint32_t pi_bsd_dom2_rls;
	uint32_t pi_dma_dir;

	// Serial Interface (SI) registers and functions
	emu_timer *si_dma_timer;
	void pif_dma(int direction);
	void handle_pif();
	int pif_channel_handle_command(int channel, int slength, uint8_t *sdata, int rlength, uint8_t *rdata);
	uint8_t calc_mempak_crc(uint8_t *buffer, int length);
	uint8_t pif_ram[0x40];
	uint8_t pif_cmd[0x40];
	uint32_t si_dram_addr;
	uint32_t si_pif_addr;
	uint32_t si_pif_addr_rd64b;
	uint32_t si_pif_addr_wr64b;
	uint32_t si_status_val;
	uint32_t si_dma_dir;
	uint32_t cic_status;
	int cic_type;

	n64_savable_data_t savable_data;

	// Video Interface (VI) functions
	void vi_recalculate_resolution();
	void video_update16(bitmap_rgb32 &bitmap);
	void video_update32(bitmap_rgb32 &bitmap);
	uint8_t random_seed;        // %HACK%, adds 19 each time it's read and is more or less random
	uint8_t get_random() { return random_seed += 0x13; }

	int32_t m_gamma_table[256];
	int32_t m_gamma_dither_table[0x4000];

};

// device type definition
DECLARE_DEVICE_TYPE(N64PERIPH, n64_periphs)
#endif

/*----------- defined in video/n64.c -----------*/

#define DACRATE_NTSC    (48681812)
#define DACRATE_PAL (49656530)
#define DACRATE_MPAL    (48628316)

/*----------- defined in machine/n64.c -----------*/

#define SP_INTERRUPT    0x1
#define SI_INTERRUPT    0x2
#define AI_INTERRUPT    0x4
#define VI_INTERRUPT    0x8
#define PI_INTERRUPT    0x10
#define DP_INTERRUPT    0x20

#define SP_STATUS_HALT          0x0001
#define SP_STATUS_BROKE         0x0002
#define SP_STATUS_DMABUSY       0x0004
#define SP_STATUS_DMAFULL       0x0008
#define SP_STATUS_IOFULL        0x0010
#define SP_STATUS_SSTEP         0x0020
#define SP_STATUS_INTR_BREAK    0x0040
#define SP_STATUS_SIGNAL0       0x0080
#define SP_STATUS_SIGNAL1       0x0100
#define SP_STATUS_SIGNAL2       0x0200
#define SP_STATUS_SIGNAL3       0x0400
#define SP_STATUS_SIGNAL4       0x0800
#define SP_STATUS_SIGNAL5       0x1000
#define SP_STATUS_SIGNAL6       0x2000
#define SP_STATUS_SIGNAL7       0x4000

#define DP_STATUS_XBUS_DMA      0x01
#define DP_STATUS_FREEZE        0x02
#define DP_STATUS_FLUSH         0x04
#define DP_STATUS_START_VALID   0x400

#define DD_ASIC_STATUS_DISK_CHANGE   0x00010000
#define DD_ASIC_STATUS_MECHA_ERR     0x00020000
#define DD_ASIC_STATUS_WRPROTECT_ERR 0x00040000
#define DD_ASIC_STATUS_HEAD_RETRACT  0x00080000
#define DD_ASIC_STATUS_MOTOR_OFF     0x00100000
#define DD_ASIC_STATUS_RESET         0x00400000
#define DD_ASIC_STATUS_BUSY          0x00800000
#define DD_ASIC_STATUS_DISK          0x01000000
#define DD_ASIC_STATUS_MECHA_INT     0x02000000
#define DD_ASIC_STATUS_BM_INT        0x04000000
#define DD_ASIC_STATUS_BM_ERROR      0x08000000
#define DD_ASIC_STATUS_C2_XFER       0x10000000
#define DD_ASIC_STATUS_DREQ          0x40000000

#define DD_TRACK_INDEX_LOCK        0x60000000

#define DD_BM_MECHA_INT_RESET 0x01000000
#define DD_BM_XFERBLOCKS      0x02000000
#define DD_BM_DISABLE_C1      0x04000000
#define DD_BM_DISABLE_OR_CHK  0x08000000
#define DD_BM_RESET           0x10000000
#define DD_BM_INT_MASK        0x20000000
#define DD_BM_MODE            0x40000000
#define DD_BM_START           0x80000000

#define DD_BMST_RUNNING       0x80000000
#define DD_BMST_ERROR         0x04000000
#define DD_BMST_MICRO_STATUS  0x02000000
#define DD_BMST_BLOCKS        0x01000000
#define DD_BMST_C1_CORRECT    0x00800000
#define DD_BMST_C1_DOUBLE     0x00400000
#define DD_BMST_C1_SINGLE     0x00200000
#define DD_BMST_C1_ERROR      0x00010000

#define DD_ASIC_ERR_AM_FAIL      0x80000000
#define DD_ASIC_ERR_MICRO_FAIL   0x40000000
#define DD_ASIC_ERR_SPINDLE_FAIL 0x20000000
#define DD_ASIC_ERR_OVER_RUN     0x10000000
#define DD_ASIC_ERR_OFFTRACK     0x08000000
#define DD_ASIC_ERR_NO_DISK      0x04000000
#define DD_ASIC_ERR_CLOCK_UNLOCK 0x02000000
#define DD_ASIC_ERR_SELF_STOP    0x01000000

#define DD_SEQ_MICRO_INT_MASK    0x80000000
#define DD_SEQ_MICRO_PC_ENABLE   0x40000000

#define SECTORS_PER_BLOCK   85
#define BLOCKS_PER_TRACK    2

const unsigned int ddZoneSecSize[16] = {232,216,208,192,176,160,144,128,
										216,208,192,176,160,144,128,112};
const unsigned int ddZoneTrackSize[16] = {158,158,149,149,149,149,149,114,
											158,158,149,149,149,149,149,114};
const unsigned int ddStartOffset[16] =
	{0x0,0x5F15E0,0xB79D00,0x10801A0,0x1523720,0x1963D80,0x1D414C0,0x20BBCE0,
		0x23196E0,0x28A1E00,0x2DF5DC0,0x3299340,0x36D99A0,0x3AB70E0,0x3E31900,0x4149200};

#endif // MAME_INCLUDES_N64_H
