#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do a 1:1 copy of a texture

#include "slang_params.inc"
#include "cathode-retro-util-lanczos.inc"
#define inTexCoord vTexCoord

//DECLARE_TEXTURE2D(g_sourceTexture, g_sampler);
layout(set = 0, binding = 2) uniform sampler2D Source;
#define g_sourceTexture Source

void main()
{
  FragColor = texture(g_sourceTexture, inTexCoord);
}
