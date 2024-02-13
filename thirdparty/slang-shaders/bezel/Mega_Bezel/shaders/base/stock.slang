#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#include "stock.inc"