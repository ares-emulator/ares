#pragma once

//{
  struct imm64 {
    explicit imm64(u64 data) : data(data) {}
    template<typename T> explicit imm64(T* pointer) : data((u64)pointer) {}
    template<typename C, typename R, typename... P> explicit imm64(auto (C::*function)(P...) -> R) {
      union force_cast_ub {
        auto (C::*function)(P...) -> R;
        u64 pointer;
      } cast{function};
      data = cast.pointer;
    }
    template<typename C, typename R, typename... P> explicit imm64(auto (C::*function)(P...) const -> R) {
      union force_cast_ub {
        auto (C::*function)(P...) const -> R;
        u64 pointer;
      } cast{function};
      data = cast.pointer;
    }
    u64 data;
  };

  template<typename C, typename V, typename... P>
  alwaysinline auto call(V (C::*function)(P...)) {
    static_assert(sizeof...(P) <= 3);
    sljit_s32 type = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1);
    if constexpr(sizeof...(P) >= 1) type |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2);
    if constexpr(sizeof...(P) >= 2) type |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 3);
    if constexpr(sizeof...(P) >= 3) type |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 4);
    if constexpr(!std::is_void_v<V>) type |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_S0, 0);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, SLJIT_FUNC_ADDR(imm64{function}.data));
  }

  template<typename C, typename R, typename... P>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, imm64{object}.data);
    sljit_s32 type = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1);
    if constexpr(!std::is_void_v<R>) type |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, SLJIT_FUNC_ADDR(imm64{function}.data));
  }

  template<typename C, typename R, typename... P, typename P0>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, imm64{object}.data);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, imm64(p0).data);
    sljit_s32 type = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1)
                   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2);
    if constexpr(!std::is_void_v<R>) type |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, SLJIT_FUNC_ADDR(imm64{function}.data));
  }

  template<typename C, typename R, typename... P, typename P0, typename P1>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, imm64{object}.data);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, imm64(p0).data);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, imm64(p1).data);
    sljit_s32 type = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1)
                   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2)
                   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 3);
    if constexpr(!std::is_void_v<R>) type |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, SLJIT_FUNC_ADDR(imm64{function}.data));
  }

  template<typename C, typename R, typename... P, typename P0, typename P1, typename P2>
  alwaysinline auto call(auto (C::*function)(P...) -> R, C* object, P0 p0, P1 p1, P2 p2) {
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, imm64{object}.data);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R1, 0, SLJIT_IMM, imm64(p0).data);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R2, 0, SLJIT_IMM, imm64(p1).data);
    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R3, 0, SLJIT_IMM, imm64(p2).data);
    sljit_s32 type = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1)
                   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2)
                   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 3)
                   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 4);
    if constexpr(!std::is_void_v<R>) type |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);
    sljit_emit_icall(compiler, SLJIT_CALL, type, SLJIT_IMM, SLJIT_FUNC_ADDR(imm64{function}.data));
  }

  #if defined(ABI_SYSTEMV) and defined(ARCHITECTURE_AMD64)
  // System V AMD64 ABI compatible function calls.

  // If we use SLJIT's regular SLJIT_CALL, function arguments are expected to be
  // in registers R0, R1, R2, and R3. The concrete x64 registers chosen in SLJIT
  // for R0 (RAX) and R2 (RDI) aren't compatible with the Sys V calling convention.
  // Therefore SLJIT always generates extra instructions to move arguments to the
  // to the registers expected by the callee.

  // We can avoid that by placing the arguments in the correct registers directly,
  // and setting the SLJIT_CALL_REG_ARG flag.
  // Note that this is actually misuse of the API, since the flag is meant for
  // functions compiled with SLJIT's custom ABI. Nevertheless, it allows us to
  // skip the two unneeded mov instructions and works just fine on x64.
  //
  // Argument placement in the two calling conventions:
  //
  //    arg#    0    1    2    3
  //    Sys V: RDI, RSI, RDX, RCX
  //    SLJIT: RAX, RSI, RDI, RCX
  //

  // Note that arg2_reg refers to TMP_REG1 since it maps to RDX, but we store
  // TMP_REG1 - 1 instead so that it works when passed through SLJIT_R(i) when
  // setup_arg() calls the reg(TMP_REG1 - 1) constructor.

  static constexpr int call_type = SLJIT_CALL_REG_ARG;
  static constexpr int arg0_reg = 2;                              // RDI
  static constexpr int arg1_reg = 1;                              // RSI
  static constexpr int arg2_reg = SLJIT_NUMBER_OF_REGISTERS + 1;  // RDX
  static constexpr int arg3_reg = 3;                              // RCX
  #else
  // SLJIT's generic SLJIT_CALL convention, compatible with any ABI.

  static constexpr int call_type = SLJIT_CALL;
  static constexpr int arg0_reg = 0;
  static constexpr int arg1_reg = 1;
  static constexpr int arg2_reg = 2;
  static constexpr int arg3_reg = 3;
  #endif

  template<typename T>
  void setup_arg(const reg& dst, const T& src) {
    if constexpr (std::is_same_v<T, mem>) {
      // The 'mem' type represents a (base register, offset) pair
      // Extract the original saved register index from the encoded first operand of mem.
      sljit_s32 S = SLJIT_EXTRACT_REG(src.fst);
      sljit_s32 i = SLJIT_NUMBER_OF_REGISTERS - S; // inverse of SLJIT_S(i)
      lea(dst, sreg(i), src.snd);
    } else if constexpr (std::is_same_v<T, reg>) {
      sljit_emit_op1(compiler, SLJIT_MOV, dst.fst, dst.snd, src.fst, src.snd);
    } else if constexpr (std::is_same_v<T, sreg>) {
      sljit_emit_op1(compiler, SLJIT_MOV, dst.fst, dst.snd, src.fst, src.snd);
    } else if constexpr (std::is_same_v<T, imm>) {
      sljit_emit_op1(compiler, SLJIT_MOV32, dst.fst, dst.snd, src.fst, src.snd);
    } else {
      static_assert(std::is_same_v<T, imm64>, "unknown function argument type");
      sljit_emit_op1(compiler, SLJIT_MOV, dst.fst, dst.snd, SLJIT_IMM, src.data);
    }
  }

  template<typename C, typename V, typename... P, typename... Q>
  void callf(V (C::*function)(P...), const Q&... args) {
    static_assert(sizeof...(P) <= 3);
    static_assert(sizeof...(P) == sizeof...(Q));

    // 1) Collect register-backed arguments and their sources.
    //    Problem: argument setup can clobber values when one argument source
    //    lives in a register that is also a destination for another argument.
    //    We first capture all register->register dependencies before emitting
    //    any move, so we can schedule them safely.
    auto tuple = std::forward_as_tuple(args...);
    reg destinations[3] = {reg(arg1_reg), reg(arg2_reg), reg(arg3_reg)};
    bool registerArg[3] = {0, 0, 0};
    bool pending[3]     = {0, 0, 0};
    sljit_s32 srcfst[3] = {0, 0, 0};
    sljit_sw srcsnd[3]  = {0, 0, 0};

    if constexpr(sizeof...(P) > 0) {
      using A0 = std::remove_cvref_t<std::tuple_element_t<0, decltype(tuple)>>;
      if constexpr(std::is_same_v<A0, reg> || std::is_same_v<A0, sreg>) {
        registerArg[0] = 1;
        pending[0] = 1;
        srcfst[0] = std::get<0>(tuple).fst;
        srcsnd[0] = std::get<0>(tuple).snd;
      }
    }
    if constexpr(sizeof...(P) > 1) {
      using A1 = std::remove_cvref_t<std::tuple_element_t<1, decltype(tuple)>>;
      if constexpr(std::is_same_v<A1, reg> || std::is_same_v<A1, sreg>) {
        registerArg[1] = 1;
        pending[1] = 1;
        srcfst[1] = std::get<1>(tuple).fst;
        srcsnd[1] = std::get<1>(tuple).snd;
      }
    }
    if constexpr(sizeof...(P) > 2) {
      using A2 = std::remove_cvref_t<std::tuple_element_t<2, decltype(tuple)>>;
      if constexpr(std::is_same_v<A2, reg> || std::is_same_v<A2, sreg>) {
        registerArg[2] = 1;
        pending[2] = 1;
        srcfst[2] = std::get<2>(tuple).fst;
        srcsnd[2] = std::get<2>(tuple).snd;
      }
    }

    // Helpers to track unresolved register moves.
    auto sourceUsedPending = [&](sljit_s32 fst, sljit_sw snd) -> bool {
      for(int i = 0; i < 3; i++) {
        if(!pending[i]) continue;
        if(srcfst[i] == fst && srcsnd[i] == snd) return 1;
      }
      return 0;
    };
    auto moveRegister = [&](const reg& dst, sljit_s32 srcFst, sljit_sw srcSnd) -> void {
      sljit_emit_op1(compiler, SLJIT_MOV, dst.fst, dst.snd, srcFst, srcSnd);
    };
    auto resolve = [&](int index) -> void {
      pending[index] = 0;
    };

    // 2) Emit non-conflicting moves first, skipping no-op moves.
    //    A move is blocked if its destination is still needed as a source by
    //    another pending move. Emitting only unblocked moves guarantees we
    //    never overwrite a value that has not been consumed yet.
    for(int step = 0; step < 6; step++) {
      bool hasPending = 0;
      for(int i = 0; i < 3; i++) { if(pending[i]) { hasPending = 1; break; } }
      if(!hasPending) break;

      bool progressed = 0;
      for(int i = 0; i < 3; i++) {
        if(!pending[i]) continue;
        auto dst = destinations[i];
        if(srcfst[i] == dst.fst && srcsnd[i] == dst.snd) {
          resolve(i);
          progressed = 1;
          continue;
        }
        bool blocked = 0;
        for(int j = 0; j < 3; j++) {
          if(i == j || !pending[j]) continue;
          if(srcfst[j] == dst.fst && srcsnd[j] == dst.snd) { blocked = 1; break; }
        }
        if(blocked) continue;
        moveRegister(dst, srcfst[i], srcsnd[i]);
        resolve(i);
        progressed = 1;
      }
      if(progressed) continue;

      // 3) Cycle detected: move one source to a safe temporary register.
      //    If no move can progress, dependencies form a cycle (e.g. swap).
      //    We break the cycle by spilling one pending source to a temporary
      //    register that is not currently used by any pending source.
      reg candidates[4] = {reg(arg0_reg), reg(arg1_reg), reg(arg2_reg), reg(arg3_reg)};
      sljit_s32 tempFst = 0;
      sljit_sw tempSnd = 0;
      bool hasTemp = 0;
      for(auto& candidate : candidates) {
        if(sourceUsedPending(candidate.fst, candidate.snd)) continue;
        tempFst = candidate.fst;
        tempSnd = candidate.snd;
        hasTemp = 1;
        break;
      }
      assert(hasTemp);
      for(int i = 0; i < 3; i++) {
        if(!pending[i]) continue;
        sljit_emit_op1(compiler, SLJIT_MOV, tempFst, tempSnd, srcfst[i], srcsnd[i]);
        srcfst[i] = tempFst;
        srcsnd[i] = tempSnd;
        break;
      }
    }

    // 4) Materialize non-register arguments now that register moves are stable.
    //    Immediates/memory operands are emitted last because they cannot be
    //    clobbered by register marshalling and do not participate in cycles.
    //    At this point register arguments are in final ABI locations.
    sljit_s32 type = SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 1);

    if constexpr(sizeof...(P) >= 1) {
      if(!registerArg[0]) setup_arg(reg(arg1_reg), std::get<0>(tuple));
      type |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 2);
    }
    if constexpr(sizeof...(P) >= 2) {
      if(!registerArg[1]) setup_arg(reg(arg2_reg), std::get<1>(tuple));
      type |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 3);
    }
    if constexpr(sizeof...(P) >= 3) {
      if(!registerArg[2]) setup_arg(reg(arg3_reg), std::get<2>(tuple));
      type |= SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_W, 4);
    }

    if constexpr(!std::is_void_v<V>) type |= SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_W);

    sljit_emit_op1(compiler, SLJIT_MOV, SLJIT_R(arg0_reg), 0, SLJIT_S0, 0);
    sljit_emit_icall(compiler, call_type, type, SLJIT_IMM, SLJIT_FUNC_ADDR(imm64{function}.data));
  }
//};
