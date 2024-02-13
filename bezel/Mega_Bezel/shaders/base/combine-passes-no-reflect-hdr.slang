#version 450

#define IS_NO_REFLECT_PRESET
#define IS_HDR_PRESET

#include "../megatron/include/parameters-hdr-color.h"
#include "../megatron/include/gamma_correct.h"
#include "../megatron/include/inverse_tonemap.h"

#pragma format A2B10G10R10_UNORM_PACK32

#include "combine-passes.inc"