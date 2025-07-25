auto CPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);
  s(scratchpad);

  s(exeLoaded);
  s(waitCycles);

  s(pipeline.address);
  s(pipeline.instruction);

  for(auto& load : delay.load) {
    u32 index = load.target ? load.target - ipu.r : ~0;
    s(index);
    load.target = index < 32 ? &ipu.r[index] : nullptr;
    s(load.source);
  }
  s(delay.branch[0].slot);
  s(delay.branch[0].take);
  s(delay.branch[0].address);
  s(delay.branch[1].slot);
  s(delay.branch[1].take);
  s(delay.branch[1].address);
  s(delay.interrupt);

  for(auto& line : icache.lines) {
    s(line.words);
    s(line.tag);
  }

  s(exception.triggered);

  s(breakpoint.lastPC);

  s(ipu.r);
  s(ipu.lo);
  s(ipu.hi);
  s(ipu.pb);
  s(ipu.pc);
  s(ipu.pd);

  s(scc.breakpoint.address.code);
  s(scc.breakpoint.address.data);
  s(scc.breakpoint.mask.code);
  s(scc.breakpoint.mask.data);
  s(scc.breakpoint.status.any);
  s(scc.breakpoint.status.code);
  s(scc.breakpoint.status.data);
  s(scc.breakpoint.status.read);
  s(scc.breakpoint.status.write);
  s(scc.breakpoint.status.trace);
  s(scc.breakpoint.redirection);
  s(scc.breakpoint.unknown);
  s(scc.breakpoint.test.code);
  s(scc.breakpoint.test.data);
  s(scc.breakpoint.test.read);
  s(scc.breakpoint.test.write);
  s(scc.breakpoint.test.trace);
  s(scc.breakpoint.enable.master);
  s(scc.breakpoint.enable.kernel);
  s(scc.breakpoint.enable.user);
  s(scc.breakpoint.enable.trap);
  s(scc.targetAddress);
  s(scc.badVirtualAddress);
  s(scc.status.frame[0].interruptEnable);
  s(scc.status.frame[0].userMode);
  s(scc.status.frame[1].interruptEnable);
  s(scc.status.frame[1].userMode);
  s(scc.status.frame[2].interruptEnable);
  s(scc.status.frame[2].userMode);
  s(scc.status.interruptMask);
  s(scc.status.cache.isolate);
  s(scc.status.cache.swap);
  s(scc.status.cache.parityZero);
  s(scc.status.cache.loadWasData);
  s(scc.status.cache.parityError);
  s(scc.status.tlbShutdown);
  s(scc.status.vectorLocation);
  s(scc.status.reverseEndian);
  s(scc.status.enable.coprocessor0);
  s(scc.status.enable.coprocessor1);
  s(scc.status.enable.coprocessor2);
  s(scc.status.enable.coprocessor3);
  s(scc.cause.exceptionCode);
  s(scc.cause.interruptPending);
  s(scc.cause.coprocessorError);
  s(scc.cause.branchTaken);
  s(scc.cause.branchDelay);
  s(scc.epc);

  s(gte.v.a.x);
  s(gte.v.a.y);
  s(gte.v.a.z);
  s(gte.v.b.x);
  s(gte.v.b.y);
  s(gte.v.b.z);
  s(gte.v.c.x);
  s(gte.v.c.y);
  s(gte.v.c.z);
  s(gte.rgbc.r);
  s(gte.rgbc.g);
  s(gte.rgbc.b);
  s(gte.rgbc.t);
  s(gte.otz);
  s(gte.ir.x);
  s(gte.ir.y);
  s(gte.ir.z);
  s(gte.ir.t);
  s(gte.screen[0].x);
  s(gte.screen[0].y);
  s(gte.screen[0].z);
  s(gte.screen[1].x);
  s(gte.screen[1].y);
  s(gte.screen[1].z);
  s(gte.screen[2].x);
  s(gte.screen[2].y);
  s(gte.screen[2].z);
  s(gte.screen[3].x);
  s(gte.screen[3].y);
  s(gte.screen[3].z);
  s(gte.rgb);
  s(gte.mac.x);
  s(gte.mac.y);
  s(gte.mac.z);
  s(gte.mac.t);
  s(gte.lzcs);
  s(gte.lzcr);
  s(gte.rotation.a.x);
  s(gte.rotation.a.y);
  s(gte.rotation.a.z);
  s(gte.rotation.b.x);
  s(gte.rotation.b.y);
  s(gte.rotation.b.z);
  s(gte.rotation.c.x);
  s(gte.rotation.c.y);
  s(gte.rotation.c.z);
  s(gte.translation.x);
  s(gte.translation.y);
  s(gte.translation.z);
  s(gte.light.a.x);
  s(gte.light.a.y);
  s(gte.light.a.z);
  s(gte.light.b.x);
  s(gte.light.b.y);
  s(gte.light.b.z);
  s(gte.light.c.x);
  s(gte.light.c.y);
  s(gte.light.c.z);
  s(gte.backgroundColor.r);
  s(gte.backgroundColor.g);
  s(gte.backgroundColor.b);
  s(gte.color.a.r);
  s(gte.color.a.g);
  s(gte.color.a.b);
  s(gte.color.b.r);
  s(gte.color.b.g);
  s(gte.color.b.b);
  s(gte.color.c.r);
  s(gte.color.c.g);
  s(gte.color.c.b);
  s(gte.farColor.r);
  s(gte.farColor.g);
  s(gte.farColor.b);
  s(gte.ofx);
  s(gte.ofy);
  s(gte.h);
  s(gte.dqa);
  s(gte.dqb);
  s(gte.zsf3);
  s(gte.zsf4);
  s(gte.flag.value);
  s(gte.lm);
  s(gte.tv);
  s(gte.mv);
  s(gte.mm);
  s(gte.sf);
}
