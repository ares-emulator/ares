#version 450

#define IS_HDR_PRESET
#pragma format A2B10G10R10_UNORM_PACK32
#include "../megatron/include/parameters-hdr-color.h"
#include "../megatron/include/gamma_correct.h"
#include "../megatron/include/inverse_tonemap.h"

#include "combine-passes.inc"