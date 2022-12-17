#pragma once

#include <nall/platform.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>
#include <nall/windows/utf8.hpp>

namespace nall {

#define Copy    0
#define RelNear 1

struct detour {
  static auto insert(const string& moduleName, const string& functionName, void*& source, void* target) -> bool;
  static auto remove(const string& moduleName, const string& functionName, void*& source) -> bool;

protected:
  static auto length(const u8* function) -> u32;
  static auto mirror(u8* target, const u8* source) -> u32;

  struct opcode {
    u16 prefix;
    u32 length;
    u32 mode;
    u16 modify;
  };
  static constexpr opcode opcodes[] = {
    //TODO:
    //* fs:, gs: should force another opcode copy
    //* conditional branches within +5-byte range should fail

      {0x50, 1},                   //push eax
      {0x51, 1},                   //push ecx
      {0x52, 1},                   //push edx
      {0x53, 1},                   //push ebx
      {0x54, 1},                   //push esp
      {0x55, 1},                   //push ebp
      {0x56, 1},                   //push esi
      {0x57, 1},                   //push edi
      {0x58, 1},                   //pop eax
      {0x59, 1},                   //pop ecx
      {0x5a, 1},                   //pop edx
      {0x5b, 1},                   //pop ebx
      {0x5c, 1},                   //pop esp
      {0x5d, 1},                   //pop ebp
      {0x5e, 1},                   //pop esi
      {0x5f, 1},                   //pop edi
      {0x64, 1},                   //fs:
      {0x65, 1},                   //gs:
      {0x68, 5},                   //push dword
      {0x6a, 2},                   //push byte
      {0x74, 2, RelNear, 0x0f84},  //je near      -> je far
      {0x75, 2, RelNear, 0x0f85},  //jne near     -> jne far
      {0x89, 2},                   //mov reg,reg
      {0x8b, 2},                   //mov reg,reg
      {0x90, 1},                   //nop
      {0xa1, 5},                   //mov eax,[dword]
      {0xeb, 2, RelNear,   0xe9},  //jmp near     -> jmp far
  };
};

inline auto detour::length(const u8* function) -> u32 {
  u32 length = 0;
  while(length < 5) {
    const detour::opcode *opcode = 0;
    for(auto& op : detour::opcodes) {
      if(function[length] == op.prefix) {
        opcode = &op;
        break;
      }
    }
    if(opcode == 0) break;
    length += opcode->length;
  }
  return length;
}

inline auto detour::mirror(u8* target, const u8* source) -> u32 {
  const u8* entryPoint = source;
  for(u32 n = 0; n < 256; n++) target[256 + n] = source[n];

  u32 size = detour::length(source);
  while(size) {
    const detour::opcode* opcode = nullptr;
    for(auto& op : detour::opcodes) {
      if(*source == op.prefix) {
        opcode = &op;
        break;
      }
    }

    switch(opcode->mode) {
    case Copy:
      for(u32 n = 0; n < opcode->length; n++) *target++ = *source++;
      break;
    case RelNear: {
      source++;
      u64 sourceAddress = (u64)source + 1 + (s8)*source;
      *target++ = opcode->modify;
      if(opcode->modify >> 8) *target++ = opcode->modify >> 8;
      u64 targetAddress = (u64)target + 4;
      u64 address = sourceAddress - targetAddress;
      *target++ = address >>  0;
      *target++ = address >>  8;
      *target++ = address >> 16;
      *target++ = address >> 24;
      source += 2;
    } break;
    }

    size -= opcode->length;
  }

  u64 address = (entryPoint + detour::length(entryPoint)) - (target + 5);
  *target++ = 0xe9;  //jmp entryPoint
  *target++ = address >>  0;
  *target++ = address >>  8;
  *target++ = address >> 16;
  *target++ = address >> 24;

  return source - entryPoint;
}

#undef Copy
#undef RelNear

}

#if defined(NALL_HEADER_ONLY)
  #include <nall/windows/detour.cpp>
#endif
