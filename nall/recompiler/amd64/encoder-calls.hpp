#pragma once

//{
  //virtual instructions to call member functions
  template<typename C, typename R, typename... P>
  auto call(auto (C::*function)(P...) -> R, C* object) {
    mov(rax, imm64{function});
    mov(rdi, imm64{object});
    sub(rsp, imm8{0x08});
    call(rax);
    add(rsp, imm8{0x08});
  }

  template<typename C, typename R, typename... P, typename P0>
  auto call(auto (C::*function)(P...) -> R, C* object, P0 p0) {
    mov(rax, imm64{function});
    mov(rdi, imm64{object});
    mov(rsi, imm64{p0});
    sub(rsp, imm8{0x08});
    call(rax);
    add(rsp, imm8{0x08});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1>
  auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1) {
    mov(rax, imm64{function});
    mov(rdi, imm64{object});
    mov(rsi, imm64{p0});
    mov(rdx, imm64{p1});
    sub(rsp, imm8{0x08});
    call(rax);
    add(rsp, imm8{0x08});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2>
  auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2) {
    mov(rax, imm64{function});
    mov(rdi, imm64{object});
    mov(rsi, imm64{p0});
    mov(rdx, imm64{p1});
    mov(rcx, imm64{p2});
    sub(rsp, imm8{0x08});
    call(rax);
    add(rsp, imm8{0x08});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2, typename P3>
  auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2, P3 p3) {
    mov(rax, imm64{function});
    mov(rdi, imm64{object});
    mov(rsi, imm64{p0});
    mov(rdx, imm64{p1});
    mov(rcx, imm64{p2});
    mov(r8, imm64{p3});
    sub(rsp, imm8{0x08});
    call(rax);
    add(rsp, imm8{0x08});
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2, typename P3, typename P4>
  auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2, P3 p3, P4 p4) {
    mov(rax, imm64{function});
    mov(rdi, imm64{object});
    mov(rsi, imm64{p0});
    mov(rdx, imm64{p1});
    mov(rcx, imm64{p2});
    mov(r8, imm64{p3});
    mov(r9, imm64{p4});
    sub(rsp, imm8{0x08});
    call(rax);
    add(rsp, imm8{0x08});
  }
//};
