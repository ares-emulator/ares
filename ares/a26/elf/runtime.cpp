auto Runtime::power(const Linker::Result& linked, bool ntsc) -> void {
  Thread::create(300'000'000, std::bind_front(&Runtime::main, this));
  ARMv6M::power();
  memory.power(linked);
  vcs.power();
  preinit = linked.preinit;
  init = linked.init;
  entrypoint = linked.entrypoint;
  systemType = ntsc ? 0 : 1;
  stage = Stage::Preinit;
  initIndex = 0;
  ready = true;
  failure = Failure::None;
  failureAddress = 0;
  startNext();
}

auto Runtime::unload() -> void {
  Thread::destroy();
  memory.unload();
  preinit.clear();
  init.clear();
  entrypoint = 0;
  systemType = 0;
  ready = false;
  failure = Failure::None;
  failureAddress = 0;
}

auto Runtime::main() -> void {
  auto wait = [&] {
    setClock(max(clock(), cpu.clock()));
    step(1);
  };
  if(!ready || faulted()) return wait();
  if(vcs.queue.size() >= 10) return wait();
  if(vcs.stalled()) return wait();

  auto error = instruction();
  if(error == Fault::Suspend) return wait();
  if(error == Fault::Return) {
    if(stage == Stage::Main) return fail(Failure::MainReturn);
    return startNext();
  }
  if(error != Fault::None) return fail(Failure::Instruction, r(15));
}

auto Runtime::access(n16 address, n8 value) -> n8 {
  if(!ready || faulted()) return value;
  return vcs.access(address, value, memory);
}

auto Runtime::invoke(u32 address, u32 stack, u32 argument0) -> Fault {
  if(!(address & 1)) return Fault::Alignment;
  r(0) = argument0;
  r(13) = stack & ~3u;
  r(14) = 0xffff'ffff;
  r(15) = address & ~1u;
  return Fault::None;
}

auto Runtime::startNext() -> void {
  constexpr u32 stackTop = Memory::StackBase + Memory::StackSize;
  if(stage == Stage::Preinit && initIndex >= preinit.size()) { stage = Stage::Init; initIndex = 0; }
  if(stage == Stage::Preinit) {
    if(auto error = invoke(preinit[initIndex++], stackTop, stackTop); error != Fault::None) {
      return fail(Failure::PreinitEntry, preinit[initIndex - 1]);
    }
    return;
  }
  if(stage == Stage::Init && initIndex >= init.size()) { stage = Stage::Main; initIndex = 0; }
  if(stage == Stage::Init) {
    if(auto error = invoke(init[initIndex++], stackTop, stackTop); error != Fault::None) {
      return fail(Failure::InitEntry, init[initIndex - 1]);
    }
    return;
  }

  auto pointer = stackTop - 12;
  memory.writeWord(pointer + 0, systemType);
  memory.writeWord(pointer + 4, (u32)frequency());
  memory.writeWord(pointer + 8, 0);
  if(auto error = invoke(entrypoint, pointer, pointer); error != Fault::None) {
    return fail(Failure::MainEntry, entrypoint);
  }
}

auto Runtime::fail(Failure reason, u32 address) -> void {
  failure = reason;
  failureAddress = address;
}

auto Runtime::error() const -> string {
  if(failure == Failure::Instruction) return {"ELF ARM runtime fault at 0x", hex(failureAddress)};
  if(failure == Failure::MainReturn) return "ELF elf_main returned unexpectedly";
  if(failure == Failure::PreinitEntry) return "ELF preinit entrypoint is not Thumb code";
  if(failure == Failure::InitEntry) return "ELF init entrypoint is not Thumb code";
  if(failure == Failure::MainEntry) return "ELF elf_main entrypoint is not Thumb code";
  return {};
}

auto Runtime::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto Runtime::get(u32 mode, n32 address, n32& value) -> Fault {
  auto location = (u32)address;
  if(mode & Prefetch) {
    if(location & 1) return Fault::Fetch;
    if(location >= Memory::StubBase && location < Memory::StubBase + VCSLib::StubCount * 4 && !(location & 3)) {
      if(auto error = vcs.execute(location, *this, memory); error != Fault::None) return error;
      value = 0x4770;  //BX LR
      return Fault::None;
    }
    if(location < Linker::TextBase || location - Linker::TextBase + 2 > memory.text.size()) return Fault::Fetch;
  }
  auto error = memory.get(mode, address, value);
  if(mode & Prefetch && error == Fault::Read) return Fault::Fetch;
  return error;
}

auto Runtime::getDebugger(u32 mode, n32 address, n32& value) -> Fault {
  auto location = (u32)address;
  if(mode & Half && location >= Memory::StubBase
    && location < Memory::StubBase + VCSLib::StubCount * 4 && !(location & 3)) {
    value = 0x4770;  //BX LR
    return Fault::None;
  }
  if(mode & Prefetch) {
    if(location & 1) return Fault::Fetch;
    if(location < Linker::TextBase || location - Linker::TextBase + 2 > memory.text.size()) return Fault::Fetch;
  }
  auto error = memory.get(mode, address, value);
  if(mode & Prefetch && error == Fault::Read) return Fault::Fetch;
  return error;
}

auto Runtime::set(u32 mode, n32 address, n32 value) -> Fault {
  return memory.set(mode, address, value);
}

auto Runtime::serialize(serializer& s) -> void {
  ARMv6M::serialize(s);
  Thread::serialize(s);
  memory.serialize(s);
  vcs.serialize(s);
  s((u32&)stage);
  s(initIndex);
  s(systemType);
  s((u32&)failure);
  s(failureAddress);
}
