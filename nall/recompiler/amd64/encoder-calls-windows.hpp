#pragma once

//{
  //virtual instructions to call member functions
  template<typename C, typename R, typename... P>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object) {
    sub(rsp, imm8{0x28});
    mov(rcx, imm64{object});
    call(imm64{function}, rax);
    add(rsp, imm8{0x28});
  }

  template<typename C, typename R, typename... P, typename P0>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0) {
    sub(rsp, imm8{0x28});
    mov(rcx, imm64{object});
    mov(rdx, imm64{p0});
    call(imm64{function}, rax);
    add(rsp, imm8{0x28});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1) {
    sub(rsp, imm8{0x28});
    mov(rcx, imm64{object});
    mov(rdx, imm64{p0});
    mov(r8, imm64{p1});
    call(imm64{function}, rax);
    add(rsp, imm8{0x28});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2) {
    sub(rsp, imm8{0x28});
    mov(rcx, imm64{object});
    mov(rdx, imm64{p0});
    mov(r8, imm64{p1});
    mov(r9, imm64{p2});
    call(imm64{function}, rax);
    add(rsp, imm8{0x28});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2, typename P3>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2, P3 p3) {
    sub(rsp, imm8{0x38});
    mov(rcx, imm64{object});
    mov(rdx, imm64{p0});
    mov(r8, imm64{p1});
    mov(r9, imm64{p2});
    mov(rax, imm64{p3});
    mov(dis8{rsp, 0x20}, rax);
    call(imm64{function}, rax);
    add(rsp, imm8{0x38});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2, typename P3, typename P4>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2, P3 p3, P4 p4) {
    sub(rsp, imm8{0x38});
    mov(rcx, imm64{object});
    mov(rdx, imm64{p0});
    mov(r8, imm64{p1});
    mov(r9, imm64{p2});
    mov(rax, imm64{p3});
    mov(dis8{rsp, 0x20}, rax);
    mov(rax, imm64{p4});
    mov(dis8{rsp, 0x28}, rax);
    call(imm64{function}, rax);
    add(rsp, imm8{0x38});
  }
//};
