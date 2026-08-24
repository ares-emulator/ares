/*
 *    Stack-less Just-In-Time compiler
 *
 *    Copyright Zoltan Herczeg (hzmester@freemail.hu). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER(S) OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static void test_buffer1(void)
{
	/* Test aligned labels. */
	executable_code code;
	struct sljit_compiler *compiler = sljit_create_compiler(NULL);
	struct sljit_label *label[12];
	struct sljit_jump *jump[3];
	sljit_s32 i;
	sljit_uw buf[8];
	sljit_uw addr[2];

	if (verbose)
		printf("Run test_buffer1\n");

	FAILED(!compiler, "cannot create compiler\n");

	for (i = 0; i < 8; i++)
		buf[i] = ~(sljit_uw)0;

	sljit_emit_enter(compiler, 0, SLJIT_ARGS1V(P), 5, 5, 2 * sizeof(sljit_sw));

	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_S0, 0);
	jump[0] = sljit_emit_op_addr(compiler, SLJIT_MOV_ADDR, SLJIT_MEM1(SLJIT_R1), 0);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 0);
	/* buf[1] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_IMM, 85603);

	label[0] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_16, NULL);
	label[1] = sljit_emit_label(compiler);
	/* buf[2] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 2 * sizeof(sljit_sw), SLJIT_R0, 0);

	label[2] = sljit_emit_label(compiler);
	label[3] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_16, NULL);
	sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 1000);
	sljit_set_label(sljit_emit_cmp(compiler, SLJIT_LESS_EQUAL, SLJIT_R0, 0, SLJIT_IMM, 17000), label[1]);

	label[4] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_2, NULL);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R4, 0, SLJIT_IMM, 46789);

	/* The SLJIT_JUMP_IF_NON_ZERO is the default. */
	jump[1] = sljit_emit_op2cmpz(compiler, SLJIT_SUB, SLJIT_S1, 0, SLJIT_R4, 0, SLJIT_IMM, 578);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM0(), 0);

	label[5] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_8, NULL);
	label[6] = sljit_emit_label(compiler);
	sljit_set_label(jump[1], label[6]);

	/* buf[3] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 3 * sizeof(sljit_sw), SLJIT_R4, 0);
	/* buf[4] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 4 * sizeof(sljit_sw), SLJIT_S1, 0);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_S0, 0);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_S1, 0, SLJIT_IMM, (5 * sizeof(sljit_sw)) >> 1);
	/* buf[5] */
	sljit_set_label(sljit_emit_op_addr(compiler, SLJIT_MOV_ADDR, SLJIT_MEM2(SLJIT_R1, SLJIT_S1), 1), label[3]);

	label[7] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_4, NULL);
	sljit_set_label(jump[0], label[7]);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, 69743);

	label[8] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_1, NULL);
	/* buf[6] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 6 * sizeof(sljit_sw), SLJIT_R1, 0);

	label[9] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_16, NULL);
	jump[2] = sljit_emit_jump(compiler, SLJIT_JUMP);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM0(), 0);
	sljit_set_label(jump[2], sljit_emit_label(compiler));

	/* buf[7] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 7 * sizeof(sljit_sw), SLJIT_IMM, 50923);

	sljit_emit_return_void(compiler);

	label[10] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_8, NULL);
	sljit_emit_op0(compiler, SLJIT_NOP);
	sljit_emit_op0(compiler, SLJIT_NOP);
	label[11] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_8, NULL);

	code.code = sljit_generate_code(compiler, 0, NULL);
	CHECK(compiler);

	FAILED((sljit_get_label_abs_addr(label[0]) & 0xf) != 0, "test_buffer1 case 1 failed\n");
	FAILED(label[2] == label[3], "test_buffer1 case 2 failed\n");
	FAILED((sljit_get_label_abs_addr(label[3]) & 0xf) != 0, "test_buffer1 case 3 failed\n");
	FAILED((sljit_get_label_abs_addr(label[4]) & 0x1) != 0, "test_buffer1 case 4 failed\n");
	FAILED((sljit_get_label_abs_addr(label[5]) & 0x7) != 0, "test_buffer1 case 5 failed\n");
	FAILED(label[5] != label[6], "test_buffer1 case 6 failed\n");
	FAILED((sljit_get_label_abs_addr(label[7]) & 0x3) != 0, "test_buffer1 case 7 failed\n");
	FAILED(label[7] == label[8], "test_buffer1 case 8 failed\n");
	FAILED((sljit_get_label_abs_addr(label[9]) & 0xf) != 0, "test_buffer1 case 9 failed\n");
	FAILED((sljit_get_label_abs_addr(label[10]) & 0x7) != 0, "test_buffer1 case 10 failed\n");
	FAILED((sljit_get_label_abs_addr(label[11]) & 0x7) != 0, "test_buffer1 case 11 failed\n");

	addr[0] = sljit_get_label_addr(label[7]);
	addr[1] = sljit_get_label_addr(label[3]);

	sljit_free_compiler(compiler);

	code.func1((sljit_sw)&buf);

	FAILED(buf[0] != addr[0], "test_buffer1 case 12 failed\n");
	FAILED(buf[1] != 85603, "test_buffer1 case 13 failed\n");
	FAILED(buf[2] != 17000, "test_buffer1 case 14 failed\n");
	FAILED(buf[3] != 46789, "test_buffer1 case 15 failed\n");
	FAILED(buf[4] != 46211, "test_buffer1 case 16 failed\n");
	FAILED(buf[5] != addr[1], "test_buffer1 case 17 failed\n");
	FAILED(buf[6] != 69743, "test_buffer1 case 18 failed\n");
	FAILED(buf[7] != 50923, "test_buffer1 case 19 failed\n");

	sljit_free_code(code.code, NULL);
	successful_tests++;
}

static void test_buffer2(void)
{
	/* Test read-only buffers. */
	executable_code code;
	struct sljit_compiler *compiler = sljit_create_compiler(NULL);
	struct sljit_read_only_buffer ro_buffers[4];
	struct sljit_jump *jump[4];
	struct sljit_label *label;
	sljit_sw executable_offset;
	sljit_sw buf[3];
	sljit_sw *ptr_sw;
	sljit_u8 *ptr_u8;
	sljit_s32 *ptr_s32;

	if (verbose)
		printf("Run test_buffer2\n");

	FAILED(!compiler, "cannot create compiler\n");

	buf[0] = -1;
	buf[1] = -1;
	buf[2] = -1;

	ro_buffers[0].next = ro_buffers + 1;
	ro_buffers[0].size = 8 * sizeof(sljit_sw);
	ro_buffers[1].next = ro_buffers + 2;
	ro_buffers[1].size = 0;
	ro_buffers[2].next = NULL;
	ro_buffers[2].size = 1;
	ro_buffers[3].next = NULL;
	ro_buffers[3].size = 5 * 4;

	sljit_emit_enter(compiler, 0, SLJIT_ARGS3V(P, P, P), 5, 5, 2 * sizeof(sljit_sw));

	jump[0] = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_S1, 0, SLJIT_IMM, -1);

	jump[1] = sljit_emit_op_addr(compiler, SLJIT_MOV_ABS_ADDR, SLJIT_R0, 0);
	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_MEM2(SLJIT_R0, SLJIT_S1), SLJIT_WORD_SHIFT);

	jump[2] = sljit_emit_op_addr(compiler, SLJIT_MOV_ABS_ADDR, SLJIT_R1, 0);
	sljit_emit_op1(compiler, SLJIT_MOV_U8, SLJIT_TMP_DEST_REG, 0, SLJIT_MEM1(SLJIT_R1), 0);
	/* buf[1] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_TMP_DEST_REG, 0);

	jump[3] = sljit_emit_jump(compiler, SLJIT_JUMP);
	sljit_set_label(jump[0], sljit_emit_label(compiler));

	label = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_W, ro_buffers);
	sljit_set_label(jump[1], ro_buffers[0].u.label);
	sljit_set_label(jump[2], ro_buffers[2].u.label);

	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_IMM, 58961);

	sljit_set_label(jump[3], sljit_emit_label(compiler));

	jump[0] = sljit_emit_cmp(compiler, SLJIT_EQUAL, SLJIT_S2, 0, SLJIT_IMM, -1);

	jump[1] = sljit_emit_op_addr(compiler, SLJIT_MOV_ABS_ADDR, SLJIT_R0, 0);
	sljit_emit_op1(compiler, SLJIT_MOV_S32, SLJIT_R1, 0, SLJIT_MEM2(SLJIT_R0, SLJIT_S2), 2);
	/* buf[2] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 2 * sizeof(sljit_sw), SLJIT_R1, 0);

	jump[2] = sljit_emit_jump(compiler, SLJIT_JUMP);
	sljit_set_label(jump[0], sljit_emit_label(compiler));

	sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_4, ro_buffers + 3);
	sljit_set_label(jump[1], ro_buffers[3].u.label);

	/* buf[2] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 2 * sizeof(sljit_sw), SLJIT_IMM, 63901);

	sljit_set_label(jump[2], sljit_emit_label(compiler));

	sljit_emit_return_void(compiler);

	code.code = sljit_generate_code(compiler, 0, NULL);
	CHECK(compiler);

	executable_offset = sljit_get_executable_offset(compiler);
	FAILED(label != ro_buffers[0].u.label, "test_buffer2 case 1 failed\n");
	FAILED(ro_buffers[1].u.label != ro_buffers[2].u.label, "test_buffer2 case 2 failed\n");
	ro_buffers[0].u.addr = sljit_get_label_abs_addr(ro_buffers[0].u.label);
	ro_buffers[2].u.addr = sljit_get_label_abs_addr(ro_buffers[2].u.label);
	ro_buffers[3].u.addr = sljit_get_label_abs_addr(ro_buffers[3].u.label);
	FAILED((ro_buffers[0].u.addr & ((1 << SLJIT_LABEL_ALIGN_W) - 1)) != 0, "test_buffer2 case 3 failed\n");
	FAILED((ro_buffers[3].u.addr & 0x3) != 0, "test_buffer2 case 4 failed\n");

	sljit_free_compiler(compiler);

	code.func3((sljit_sw)&buf, -1, -1);

	FAILED(buf[0] != 58961, "test_buffer2 case 4 failed\n");
	FAILED(buf[1] != -1, "test_buffer2 case 5 failed\n");
	FAILED(buf[2] != 63901, "test_buffer2 case 6 failed\n");

	ptr_sw = (sljit_sw*)sljit_read_only_buffer_start_writing(ro_buffers[0].u.addr, ro_buffers[0].size, executable_offset);
	ptr_sw[0] = 85372;
	ptr_sw[4] = -92714;
	ptr_sw[7] = 60824;
	sljit_read_only_buffer_end_writing(ro_buffers[0].u.addr, ro_buffers[0].size, executable_offset);

	ptr_u8 = (sljit_u8*)sljit_read_only_buffer_start_writing(ro_buffers[2].u.addr, ro_buffers[2].size, executable_offset);
	ptr_u8[0] = 174;
	sljit_read_only_buffer_end_writing(ro_buffers[2].u.addr, ro_buffers[2].size, executable_offset);

	ptr_s32 = (sljit_s32*)sljit_read_only_buffer_start_writing(ro_buffers[3].u.addr, ro_buffers[3].size, executable_offset);
	ptr_s32[0] = -82561;
	ptr_s32[1] = 73610;
	ptr_s32[4] = -38219;
	sljit_read_only_buffer_end_writing(ro_buffers[3].u.addr, ro_buffers[3].size, executable_offset);

	code.func3((sljit_sw)&buf, 0, 0);

	FAILED(buf[0] != 85372, "test_buffer2 case 7 failed\n");
	FAILED(buf[1] != 174, "test_buffer2 case 8 failed\n");
	FAILED(buf[2] != -82561, "test_buffer2 case 9 failed\n");

	code.func3((sljit_sw)&buf, 4, 1);

	FAILED(buf[0] != -92714, "test_buffer2 case 10 failed\n");
	FAILED(buf[1] != 174, "test_buffer2 case 11 failed\n");
	FAILED(buf[2] != 73610, "test_buffer2 case 12 failed\n");

	code.func3((sljit_sw)&buf, 7, 4);

	FAILED(buf[0] != 60824, "test_buffer2 case 13 failed\n");
	FAILED(buf[1] != 174, "test_buffer2 case 14 failed\n");
	FAILED(buf[2] != -38219, "test_buffer2 case 15 failed\n");

	sljit_free_code(code.code, NULL);
	successful_tests++;
}

static void test_buffer3(void)
{
	/* Test read-only buffers. */
	executable_code code;
	struct sljit_compiler *compiler = sljit_create_compiler(NULL);
	struct sljit_read_only_buffer ro_buffers[2];
	struct sljit_jump *jump[4];
	struct sljit_label *label[8];
	sljit_sw executable_offset;
	sljit_sw buf[2];
	sljit_uw *ptr_uw;

	if (verbose)
		printf("Run test_buffer3\n");

	FAILED(!compiler, "cannot create compiler\n");

	ro_buffers[0].next = ro_buffers + 1;
	ro_buffers[0].size = 4 * sizeof(sljit_sw);
	ro_buffers[1].next = NULL;
	ro_buffers[1].size = 3 * sizeof(sljit_sw);

	sljit_emit_enter(compiler, 0, SLJIT_ARGS3V(P, P, P), 5, 5, 2 * sizeof(sljit_sw));

	jump[0] = sljit_emit_op_addr(compiler, SLJIT_MOV_ABS_ADDR, SLJIT_TMP_DEST_REG, 0);
	sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_MEM2(SLJIT_TMP_DEST_REG, SLJIT_S1), SLJIT_WORD_SHIFT);

	label[0] = sljit_emit_label(compiler);
	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_IMM, 83974);
	jump[1] = sljit_emit_jump(compiler, SLJIT_JUMP);

	label[1] = sljit_emit_label(compiler);
	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_IMM, 65803);
	jump[2] = sljit_emit_jump(compiler, SLJIT_JUMP);

	label[2] = sljit_emit_label(compiler);
	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_IMM, -38405);
	jump[3] = sljit_emit_jump(compiler, SLJIT_JUMP);

	label[3] = sljit_emit_label(compiler);
	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_IMM, 74198);

	label[4] = sljit_emit_label(compiler);
	sljit_set_label(jump[1], label[4]);
	sljit_set_label(jump[2], label[4]);
	sljit_set_label(jump[3], label[4]);

	jump[1] = sljit_emit_op_addr(compiler, SLJIT_MOV_ABS_ADDR, SLJIT_TMP_DEST_REG, 0);
	sljit_emit_ijump(compiler, SLJIT_FAST_CALL, SLJIT_MEM2(SLJIT_TMP_DEST_REG, SLJIT_S2), 0);

	sljit_emit_return_void(compiler);

	label[4] = sljit_emit_label(compiler);
	sljit_emit_op_dst(compiler, SLJIT_FAST_ENTER, SLJIT_TMP_DEST_REG, 0);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 63891);
	/* buf[1] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_R0, 0);
	sljit_emit_op_src(compiler, SLJIT_FAST_RETURN, SLJIT_TMP_DEST_REG, 0);

	label[5] = sljit_emit_label(compiler);
	sljit_emit_op_dst(compiler, SLJIT_FAST_ENTER, SLJIT_TMP_DEST_REG, 0);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, -27019);
	/* buf[1] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_R1, 0);
	sljit_emit_op_src(compiler, SLJIT_FAST_RETURN, SLJIT_TMP_DEST_REG, 0);

	label[6] = sljit_emit_label(compiler);
	sljit_emit_op_dst(compiler, SLJIT_FAST_ENTER, SLJIT_TMP_DEST_REG, 0);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, 95486);
	/* buf[1] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_R2, 0);
	sljit_emit_op_src(compiler, SLJIT_FAST_RETURN, SLJIT_TMP_DEST_REG, 0);

	label[7] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_W, ro_buffers);
	sljit_set_label(jump[0], ro_buffers[0].u.label);
	sljit_set_label(jump[1], ro_buffers[1].u.label);

	code.code = sljit_generate_code(compiler, 0, NULL);
	CHECK(compiler);

	executable_offset = sljit_get_executable_offset(compiler);
	ro_buffers[0].u.addr = sljit_get_label_abs_addr(ro_buffers[0].u.label);
	ro_buffers[1].u.addr = sljit_get_label_abs_addr(ro_buffers[1].u.label);

	ptr_uw = (sljit_uw*)sljit_read_only_buffer_start_writing(ro_buffers[0].u.addr, ro_buffers[0].size, executable_offset);
	ptr_uw[0] = sljit_get_label_addr(label[0]);
	ptr_uw[1] = sljit_get_label_addr(label[1]);
	ptr_uw[2] = sljit_get_label_addr(label[2]);
	ptr_uw[3] = sljit_get_label_addr(label[3]);
	sljit_read_only_buffer_end_writing(ro_buffers[0].u.addr, ro_buffers[0].size, executable_offset);

	ptr_uw = (sljit_uw*)sljit_read_only_buffer_start_writing(ro_buffers[1].u.addr, ro_buffers[1].size, executable_offset);
	ptr_uw[0] = sljit_get_label_addr(label[4]);
	ptr_uw[1] = sljit_get_label_addr(label[5]);
	ptr_uw[2] = sljit_get_label_addr(label[6]);
	sljit_read_only_buffer_end_writing(ro_buffers[1].u.addr, ro_buffers[1].size, executable_offset);

	sljit_free_compiler(compiler);

	code.func3((sljit_sw)&buf, 0, 0);

	FAILED(buf[0] != 83974, "test_buffer3 case 1 failed\n");
	FAILED(buf[1] != 63891, "test_buffer3 case 2 failed\n");

	code.func3((sljit_sw)&buf, 3, sizeof(sljit_uw));

	FAILED(buf[0] != 74198, "test_buffer3 case 3 failed\n");
	FAILED(buf[1] != -27019, "test_buffer3 case 4 failed\n");

	code.func3((sljit_sw)&buf, 1, 2 * sizeof(sljit_uw));

	FAILED(buf[0] != 65803, "test_buffer3 case 5 failed\n");
	FAILED(buf[1] != 95486, "test_buffer3 case 6 failed\n");

	code.func3((sljit_sw)&buf, 2, 0);

	FAILED(buf[0] != -38405, "test_buffer3 case 7 failed\n");
	FAILED(buf[1] != 63891, "test_buffer3 case 8 failed\n");

	sljit_free_code(code.code, NULL);
	successful_tests++;
}

static void test_buffer4(void)
{
	/* Test read-only buffers. */
	executable_code code;
	struct sljit_compiler *compiler = sljit_create_compiler(NULL);
	struct sljit_read_only_buffer ro_buffers[2];
	struct sljit_jump *jump[7];
	struct sljit_label *label;
	sljit_sw executable_offset;
	sljit_uw buf[8];
	sljit_uw addr[2];
	sljit_s32 i;
	sljit_uw *ptr_uw;

	if (verbose)
		printf("Run test_buffer4\n");

	FAILED(!compiler, "cannot create compiler\n");

	for (i = 0; i < 8; i++)
		buf[i] = ~(sljit_uw)0;
	buf[3] = 60439;
	buf[5] = 74901;

	ro_buffers[0].next = ro_buffers + 1;
	ro_buffers[0].size = 3 * sizeof(sljit_sw);
	ro_buffers[1].next = NULL;
	ro_buffers[1].size = sizeof(sljit_sw);

	sljit_emit_enter(compiler, 0, SLJIT_ARGS1V(P), 5, 5, 8 * sizeof(sljit_sw));

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 58321);
	jump[0] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_R0, 0);
	/* buf[0] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_R0, 0);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_S1, 0, SLJIT_IMM, -45209);
	jump[1] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_S1, 0);
	/* buf[1] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_S1, 0);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 3 * sizeof(sljit_sw), SLJIT_IMM, 29614);
	jump[2] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_MEM1(SLJIT_SP), 3 * sizeof(sljit_sw));
	/* buf[2] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 2 * sizeof(sljit_sw), SLJIT_MEM1(SLJIT_SP), 3 * sizeof(sljit_sw));

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_S0, 0);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_S1, 0, SLJIT_IMM, 3);
	/* buf[3] */
	jump[3] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_MEM2(SLJIT_R1, SLJIT_S1), SLJIT_WORD_SHIFT);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, -71694);
	jump[4] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_TMP_DEST_REG, 0);
	/* buf[4] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 4 * sizeof(sljit_sw), SLJIT_TMP_DEST_REG, 0);

	/* buf[5] */
	jump[5] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_MEM0(), (sljit_sw)(buf + 5));

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_TMP_DEST_REG, 0, SLJIT_IMM, 68531);
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, -84012);
	jump[6] = sljit_emit_op_addr(compiler, SLJIT_ADD_ABS_ADDR, SLJIT_R1, 0);
	/* buf[6] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 6 * sizeof(sljit_sw), SLJIT_R1, 0);
	/* buf[7] */
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 7 * sizeof(sljit_sw), SLJIT_TMP_DEST_REG, 0);

	sljit_emit_return_void(compiler);

	label = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_W, ro_buffers);
	sljit_set_label(jump[0], label);
	sljit_set_label(jump[1], label);
	sljit_set_label(jump[2], label);
	sljit_set_label(jump[3], label);
	sljit_set_label(jump[4], label);

	label = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_W, ro_buffers + 1);
	sljit_set_label(jump[5], label);
	sljit_set_label(jump[6], label);

	code.code = sljit_generate_code(compiler, 0, NULL);
	CHECK(compiler);

	executable_offset = sljit_get_executable_offset(compiler);
	addr[0] = sljit_get_label_abs_addr(ro_buffers[0].u.label);
	addr[1] = sljit_get_label_abs_addr(ro_buffers[1].u.label);

	sljit_free_compiler(compiler);

	ptr_uw = (sljit_uw*)sljit_read_only_buffer_start_writing(addr[0], ro_buffers[0].size, executable_offset);
	ptr_uw[0] = 0;
	sljit_read_only_buffer_end_writing(addr[0], ro_buffers[0].size, executable_offset);

	ptr_uw = (sljit_uw*)sljit_read_only_buffer_start_writing(addr[1], ro_buffers[1].size, executable_offset);
	ptr_uw[0] = 0;
	sljit_read_only_buffer_end_writing(addr[1], ro_buffers[1].size, executable_offset);

	code.func1((sljit_sw)&buf);

	FAILED((addr[0] & (sizeof(sljit_sw) - 1)) != 0, "test_buffer4 case 1 failed\n");
	FAILED((addr[1] & (sizeof(sljit_sw) - 1)) != 0, "test_buffer4 case 2 failed\n");
	FAILED(buf[0] != addr[0] + 58321, "test_buffer4 case 3 failed\n");
	FAILED(buf[1] != addr[0] - 45209, "test_buffer4 case 4 failed\n");
	FAILED(buf[2] != addr[0] + 29614, "test_buffer4 case 5 failed\n");
	FAILED(buf[3] != addr[0] + 60439, "test_buffer4 case 6 failed\n");
	FAILED(buf[4] != addr[0] - 71694, "test_buffer4 case 7 failed\n");
	FAILED(buf[5] != addr[1] + 74901, "test_buffer4 case 8 failed\n");
	FAILED(buf[6] != addr[1] - 84012, "test_buffer4 case 9 failed\n");
	FAILED(buf[7] != 68531, "test_buffer4 case 10 failed\n");

	sljit_free_code(code.code, NULL);
	successful_tests++;
}

static void test_buffer5(void)
{
	/* Test code rewriting in buffers. */
	executable_code code;
	struct sljit_compiler *compiler = sljit_create_compiler(NULL);
	struct sljit_read_only_buffer ro_buffers[1];
	struct sljit_generate_code_buffer code_buffer;
	struct sljit_label *label[2];
	sljit_sw executable_offset;
	sljit_sw buf[3];
	sljit_uw addr[2];
	sljit_s32 i;
	void *dyn_code;

	if (verbose)
		printf("Run test_buffer5\n");

	FAILED(!compiler, "cannot create compiler\n");

	ro_buffers[0].next = NULL;
	ro_buffers[0].size = 127;

	sljit_emit_enter(compiler, 0, SLJIT_ARGS1V(P), 5, 5, 8 * sizeof(sljit_sw));

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 0, SLJIT_IMM, -598461);
	label[0] = sljit_emit_aligned_label(compiler, SLJIT_LABEL_ALIGN_1, ro_buffers);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_MEM0(), 0);
	label[1] = sljit_emit_label(compiler);

	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), sizeof(sljit_sw), SLJIT_IMM, 613802);
	sljit_emit_return_void(compiler);

	/* Initial code. */
	code.code = sljit_generate_code(compiler, 0, NULL);
	CHECK(compiler);

	executable_offset = sljit_get_executable_offset(compiler);
	addr[0] = sljit_get_label_abs_addr(label[0]);
	addr[1] = sljit_get_label_addr(label[1]);

	sljit_free_compiler(compiler);

	compiler = sljit_create_compiler(NULL);
	FAILED(!compiler, "cannot create compiler\n");

	sljit_set_context(compiler, 0, SLJIT_ARGS1V(P), 5, 5, 8 * sizeof(sljit_sw));
	sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_MEM1(SLJIT_S0), 2 * sizeof(sljit_sw), SLJIT_IMM, -305814);
	sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_IMM, (sljit_sw)addr[1]);

	code_buffer.buffer = sljit_read_only_buffer_start_writing(addr[0], ro_buffers[0].size, executable_offset);
	code_buffer.size = ro_buffers[0].size;
	code_buffer.executable_offset = executable_offset;

	dyn_code = sljit_generate_code(compiler, SLJIT_GENERATE_CODE_BUFFER | SLJIT_GENERATE_CODE_NO_CONTEXT, &code_buffer);
	CHECK(compiler);
#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
	FAILED((sljit_uw)dyn_code != addr[0] + 1, "test_buffer5 case 1 failed\n");
#else /* !SLJIT_CONFIG_ARM_THUMB2 */
	FAILED((sljit_uw)dyn_code != addr[0], "test_buffer5 case 1 failed\n");
#endif

	sljit_read_only_buffer_end_writing(addr[0], ro_buffers[0].size, executable_offset);
	sljit_free_compiler(compiler);

	for (i = 0; i < 3; i++)
		buf[i] = -1;

	code.func1((sljit_sw)&buf);
	FAILED(buf[0] != -598461, "test_buffer5 case 2 failed\n");
	FAILED(buf[1] != 613802, "test_buffer5 case 3 failed\n");
	FAILED(buf[2] != -305814, "test_buffer5 case 4 failed\n");

	/* Code modification. */
	compiler = sljit_create_compiler(NULL);
	FAILED(!compiler, "cannot create compiler\n");

	sljit_set_context(compiler, 0, SLJIT_ARGS1V(P), 5, 5, 8 * sizeof(sljit_sw));
	sljit_emit_op2(compiler, SLJIT_ADD, SLJIT_MEM1(SLJIT_S0), 2 * sizeof(sljit_sw), SLJIT_S0, 0, SLJIT_IMM, 1);
	sljit_emit_ijump(compiler, SLJIT_JUMP, SLJIT_IMM, (sljit_sw)addr[1]);

	code_buffer.buffer = sljit_read_only_buffer_start_writing(addr[0], ro_buffers[0].size, executable_offset);
	code_buffer.size = ro_buffers[0].size;
	code_buffer.executable_offset = executable_offset;

	dyn_code = sljit_generate_code(compiler, SLJIT_GENERATE_CODE_BUFFER | SLJIT_GENERATE_CODE_NO_CONTEXT, &code_buffer);
	CHECK(compiler);
#if (defined SLJIT_CONFIG_ARM_THUMB2 && SLJIT_CONFIG_ARM_THUMB2)
	FAILED((sljit_uw)dyn_code != addr[0] + 1, "test_buffer5 case 5 failed\n");
#else /* !SLJIT_CONFIG_ARM_THUMB2 */
	FAILED((sljit_uw)dyn_code != addr[0], "test_buffer5 case 5 failed\n");
#endif

	sljit_read_only_buffer_end_writing(addr[0], ro_buffers[0].size, executable_offset);
	sljit_free_compiler(compiler);

	for (i = 0; i < 3; i++)
		buf[i] = -1;

	code.func1((sljit_sw)&buf);
	FAILED(buf[0] != -598461, "test_buffer5 case 6 failed\n");
	FAILED(buf[1] != 613802, "test_buffer5 case 7 failed\n");
	FAILED(buf[2] != (sljit_sw)buf + 1, "test_buffer5 case 8 failed\n");

	sljit_free_code(code.code, NULL);
	successful_tests++;
}
