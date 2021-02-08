auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(cpu.ram.size);
  memory.ram->setRead([&](u32 address) -> u8 {
    return cpu.ram.readByte(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return cpu.ram.writeByte(address, data);
  });

  memory.scratchpad = parent->append<Node::Debugger::Memory>("CPU Scratchpad");
  memory.scratchpad->setSize(cpu.scratchpad.size);
  memory.scratchpad->setRead([&](u32 address) -> u8 {
    return cpu.scratchpad.readByte(address);
  });
  memory.scratchpad->setWrite([&](u32 address, u8 data) -> void {
    return cpu.scratchpad.writeByte(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(32, 2);

  tracer.exception = parent->append<Node::Debugger::Tracer::Notification>("Exception", "CPU");
  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");
  tracer.message = parent->append<Node::Debugger::Tracer::Notification>("Message", "CPU");
  tracer.function = parent->append<Node::Debugger::Tracer::Notification>("Function", "CPU");
}

auto CPU::Debugger::instruction() -> void {
  if(!tracer.instruction->enabled()) return;

  u32 address = cpu.pipeline.address;
  u32 instruction = cpu.pipeline.instruction;
  if(tracer.instruction->address(address)) {
    cpu.disassembler.showColors = 0;
    tracer.instruction->notify(cpu.disassembler.disassemble(address, instruction), {});
    cpu.disassembler.showColors = 1;
  }
}

auto CPU::Debugger::exception(u8 code) -> void {
  if(!tracer.exception->enabled()) return;

  string type;
  if(code ==  0) type = "Interrupt";
  if(code ==  4) type = "AddressLoad";
  if(code ==  5) type = "AddressStore";
  if(code ==  6) type = "BusInstruction";
  if(code ==  7) type = "BusData";
  if(code ==  8) type = "SystemCall";
  if(code ==  9) type = "Breakpoint";
  if(code == 10) type = "ReservedInstruction";
  if(code == 11) type = "CoprocessorDisabled";
  if(code == 12) type = "ArithmeticOverflow";
  if(code == 13) type = "Trap";

  if(code ==  0) return;  //interrupt exceptions are logged by interrupt() instead
  if(code ==  8) return;  //ignore SYSCALL exceptions (they are used often to call BIOS functions)

  tracer.exception->notify({type, " PC:", hex(cpu.ipu.pc, 8L)});
}

auto CPU::Debugger::interrupt(u8 mask) -> void {
  if(!tracer.interrupt->enabled()) return;

  string source;
  if(mask & 0x01) {
    source.append("Software0,");
  }
  if(mask & 0x02) {
    source.append("Software1,");
  }
  if(mask & 0x04) {
    if(PlayStation::interrupt.vblank.poll()) source.append("Vblank,");
    if(PlayStation::interrupt.gpu.poll()) source.append("GPU,");
    if(PlayStation::interrupt.cdrom.poll()) source.append("CDROM,");
    if(PlayStation::interrupt.dma.poll()) source.append("DMA,");
    if(PlayStation::interrupt.timer0.poll()) source.append("Timer0,");
    if(PlayStation::interrupt.timer1.poll()) source.append("Timer1,");
    if(PlayStation::interrupt.timer2.poll()) source.append("Timer2,");
    if(PlayStation::interrupt.peripheral.poll()) source.append("Peripheral,");
    if(PlayStation::interrupt.sio.poll()) source.append("SIO,");
    if(PlayStation::interrupt.spu.poll()) source.append("SPU,");
    if(PlayStation::interrupt.pio.poll()) source.append("PIO,");
  }
  source.trimRight(",", 1L);
  tracer.interrupt->notify(source);
}

auto CPU::Debugger::messageChar(char c) -> void {
  if(!tracer.message->enabled()) return;

  static string message;
  if(c == '\n') {
    if(message) {
      tracer.message->notify(message);
      message = {};
    }
  } else if(message.size() < 512) {
    message.append(c);
  }
}

auto CPU::Debugger::messageText(u32 address) -> void {
  if(!tracer.message->enabled()) return;

  for(u32 index : range(512)) {
    char data = bus.read<Byte>(address + index);
    if(!data) break;
    messageChar(data);
  }
}

auto CPU::Debugger::message() -> void {
  if(!tracer.message->enabled()) return;

  if(cpu.ipu.pc == 0xa0 && cpu.ipu.r[9] == 0x3c) messageChar((char)cpu.ipu.r[4]);
  if(cpu.ipu.pc == 0xb0 && cpu.ipu.r[9] == 0x3d) messageChar((char)cpu.ipu.r[4]);
  if(cpu.ipu.pc == 0xa0 && cpu.ipu.r[9] == 0x3e) messageText(cpu.ipu.r[4]);
  if(cpu.ipu.pc == 0xb0 && cpu.ipu.r[9] == 0x3f) messageText(cpu.ipu.r[4]);
}

auto CPU::Debugger::function() -> void {
  if(!tracer.function->enabled()) return;

  u32 pc = cpu.ipu.pc & 0x1fff'ffff;
  u32 p0 = cpu.ipu.r[4];
  u32 p1 = cpu.ipu.r[5];
  u32 p2 = cpu.ipu.r[6];
  u32 p3 = cpu.ipu.r[7];
  string call;

  auto hex8 = [](u32 value) -> string {
    return {"$", hex(value, 2L)};
  };

  auto hex16 = [](u32 value) -> string {
    return {"$", hex(value, 4L)};
  };

  auto hex32 = [](u32 value) -> string {
    return {"$", hex(value, 8L)};
  };

  auto character = [](u32 value) -> string {
    if(value >= 0x20 && value <= 0x7e) {
      return {"'", (char)value, "'"};
    }
    if(value == 0x09) return "'\\t'";
    if(value == 0x0a) return "'\\n'";
    if(value == 0x0d) return "'\\r'";
    return {"$", hex(value, 2L)};
  };

  auto text = [](u32 address) -> string {
    string output;
    output.append("\"");
    for(u32 n : range(256)) {
      char value = bus.read<Byte>(address + n);
      if(!value) break;
      else if(value == 0x09) output.append("\\t");
      else if(value == 0x0a) output.append("\\n");
      else if(value == 0x0d) output.append("\\r");
      else if(value >= 0x20 && value <= 0x7e) output.append(value);
      else output.append("$", hex(value, 2L));
    }
    output.append("\"");
    return output;
  };

  auto event = [](u32 value) -> string {
    switch(value) {
    case 0xf000'0001: return "vblank";
    case 0xf000'0002: return "gpu";
    case 0xf000'0003: return "cdromDecoder";
    case 0xf000'0004: return "dmaController";
    case 0xf000'0005: return "timer0";
    case 0xf000'0006: return "timer1_2";
    case 0xf000'0008: return "controller";
    case 0xf000'0009: return "spu";
    case 0xf000'000a: return "pio";
    case 0xf000'000b: return "sio";
    case 0xf000'0010: return "exception";
    case 0xf000'0011: return "memoryCard";
    case 0xf000'0012: return "memoryCard1";
    case 0xf000'0013: return "memoryCard2";
    case 0xf200'0000: return "rootCounter0";
    case 0xf200'0001: return "rootCounter1";
    case 0xf200'0002: return "rootCounter2";
    case 0xf200'0003: return "rootCounter3";
    case 0xf400'0001: return "memoryCardHLE";
    case 0xf400'0002: return "libmath";
    }
    return {"$", hex(value, 8L)};
  };

  auto spec = [](u32 value) -> string {
    switch(value) {
    case 0x0001: return "counterReachedZero";
    case 0x0002: return "interrupted";
    case 0x0004: return "endOfIO";
    case 0x0008: return "fileWasClosed";
    case 0x0010: return "commandAcknowledged";
    case 0x0020: return "commandCompleted";
    case 0x0040: return "dataReady";
    case 0x0080: return "dataEnd";
    case 0x0100: return "timeout";
    case 0x0200: return "unknownCommand";
    case 0x0400: return "endOfReadBuffer";
    case 0x0800: return "endOfWriteBuffer";
    case 0x1000: return "generalInterrupt";
    case 0x2000: return "newDevice";
    case 0x4000: return "systemCall";
    case 0x8000: return "error";
    case 0x8001: return "previousWriteErrorOccurred";
    case 0x0301: return "libmathDomainError";
    case 0x0302: return "libmathRangeError";
    }
    return {"$", hex(value, 4L)};
  };

  #define op_(id, fn) case id: call = {fn, "(...)"}; break
  #define op0(id, fn) case id: call = {fn, "()"}; break
  #define op1(id, fn, p0) case id: call = {fn, "(", p0, ")"}; break;
  #define op2(id, fn, p0, p1) case id: call = {fn, "(", p0, ",", p1, ")"}; break
  #define op3(id, fn, p0, p1, p2) case id: call = {fn, "(", p0, ",", p1, ",", p2, ")"}; break;
  #define op4(id, fn, p0, p1, p2, p3) case id: call = {fn, "(", p0, ",", p1, ",", p2, ",", p3, ")"}; break;

  if(pc == 0x80) switch(cpu.ipu.r[4]) {
  op0(0x00, "NoFunction");
  op0(0x01, "EnterCriticalSection");
  op0(0x02, "ExitCriticalSection");
  op_(0x03, "ChangeThreadSubFunction");
  op2(0x04, "DeliverEvent", event(p0), spec(p1));
  }

  if(pc == 0xa0) switch(cpu.ipu.r[9]) {
  op2(0x00, "FileOpen", text(p0), hex32(p1));
  op3(0x01, "FileSeek", hex8(p0), hex32(p1), hex8(p2));
  op3(0x02, "FileRead", hex8(p0), hex32(p1), hex32(p2));
  op3(0x03, "FileWrite", hex8(p0), hex32(p1), hex32(p2));
  op1(0x04, "FileClose", hex8(p0));
  op_(0x05, "FileIoctl");
  op_(0x06, "exit");
  op_(0x07, "FileGetDeviceFlag");
  op_(0x08, "FileGetc");
  op_(0x09, "FilePutc");
  op_(0x0a, "todigit");
  op_(0x0b, "atof");
  op_(0x0c, "strtoul");
  op_(0x0d, "strtol");
  op_(0x0e, "abs");
  op_(0x0f, "labs");
  op_(0x10, "atoi");
  op_(0x11, "atol");
  op_(0x12, "atob");
  op_(0x13, "SaveState");
  op_(0x14, "RestoreState");
  op2(0x15, "strcat", text(p0), text(p1));
  op3(0x16, "strncat", text(p0), text(p1), hex32(p2));
  op2(0x17, "strcmp", text(p0), text(p1));
  op3(0x18, "strncmp", text(p0), text(p1), hex32(p2));
  op2(0x19, "strcpy", hex32(p0), text(p1));
  op3(0x1a, "strncpy", hex32(p0), text(p1), hex32(p2));
  op1(0x1b, "strlen", text(p0));
  op_(0x1c, "index");
  op_(0x1d, "rindex");
  op_(0x1e, "strchr");
  op_(0x1f, "strrchr");
  op_(0x20, "strpbrk");
  op_(0x21, "strspn");
  op_(0x22, "strcspn");
  op_(0x23, "strtok");
  op_(0x24, "strstr");
  op1(0x25, "toupper", character(p0));
  op1(0x26, "tolower", character(p0));
  op_(0x27, "bcopy");
  op_(0x28, "bzero");
  op_(0x29, "bcmp");
  op3(0x2a, "memcpy", hex32(p0), hex32(p1), hex32(p2));
  op3(0x2b, "memset", hex32(p0), character(p1), hex32(p2));
  op3(0x2c, "memmove", hex32(p0), hex32(p1), hex32(p2));
  op3(0x2d, "memcmp", hex32(p0), hex32(p1), hex32(p2));
  op3(0x2e, "memchr", hex32(p0), character(p1), hex32(p2));
  op0(0x2f, "rand");
  op1(0x30, "srand", hex32(p0));
  op_(0x31, "qsort");
  op_(0x32, "strtod");
  op1(0x33, "malloc", hex32(p0));
  op1(0x34, "free", hex32(p0));
  op_(0x35, "lsearch");
  op_(0x36, "bsearch");
  op2(0x37, "calloc", hex32(p0), hex32(p1));
  op2(0x38, "realloc", hex32(p0), hex32(p1));
  op_(0x39, "InitHeap");
  op_(0x3a, "SystemErrorExit");
  op0(0x3b, "std_in_getchar");
  op1(0x3c, "std_out_putchar", character(p0));
  op_(0x3d, "std_in_gets");
  op1(0x3e, "std_out_puts", text(p0));
  op2(0x3f, "printf", text(p0), "...");
  op0(0x40, "SystemErrorUnresolvedException");
  op_(0x41, "LoadExeHeader");
  op_(0x42, "LoadExeFile");
  op_(0x43, "DoExecute");
  op0(0x44, "FlushCache");
  op_(0x45, "init_a0_b0_c0_vectors");
  op_(0x46, "GPU_dw");
  op_(0x47, "gpu_send_dma");
  op_(0x48, "SendGP1Command");
  op_(0x49, "GPU_cw");
  op_(0x4a, "GPU_cwp");
  op_(0x4b, "send_gpu_linked_list");
  op0(0x4c, "gpu_abort_dma");
  op0(0x4d, "GetGPUStatus");
  op0(0x4e, "gpu_sync");
  op_(0x4f, "SystemError");
  op_(0x50, "SystemError");
  op_(0x51, "LoadAndExecute");
  op_(0x52, "GetSysSp");
  op_(0x53, "set_ioabort_handler");
  op0(0x54, "CdInit");
  op0(0x55, "_bu_init");
  op0(0x56, "CdRemove");
  op0(0x5b, "dev_tty_init");
  op_(0x5c, "dev_tty_open");
  op_(0x5d, "dev_tty_in_out");
  op_(0x5e, "dev_tty_ioctl");
  op_(0x5f, "dev_cd_open");
  op_(0x60, "dev_cd_read");
  op_(0x61, "dev_cd_close");
  op_(0x62, "dev_cd_firstfile");
  op_(0x63, "dev_cd_nextfile");
  op_(0x64, "dev_cd_chdir");
  op_(0x65, "dev_card_open");
  op_(0x66, "dev_card_read");
  op_(0x67, "dev_card_write");
  op_(0x68, "dev_card_close");
  op_(0x69, "dev_card_firstfile");
  op_(0x6a, "dev_card_nextfile");
  op_(0x6b, "dev_card_erase");
  op_(0x6c, "dev_card_undelete");
  op_(0x6d, "dev_card_format");
  op_(0x6e, "dev_card_rename");
  op_(0x6f, "card_clear_error");
  op0(0x70, "_bu_init");
  op0(0x71, "CdInit");
  op0(0x72, "CdRemove");
  op_(0x78, "CdAsyncSeekL");
  op_(0x7c, "CdAsyncGetStatus");
  op_(0x7e, "CdAsyncReadSector");
  op_(0x81, "CdAsyncSetMode");
  op0(0x90, "CdromIoIrqFunc1");
  op0(0x91, "CdromDmaIrqFunc1");
  op0(0x92, "CdromIoIrqFunc2");
  op0(0x93, "CdromDmaIrqFunc2");
  op_(0x94, "CdromGetInt5errCode");
  op0(0x95, "CdInitSubFunc");
  op0(0x96, "AddCDROMDevice");
  op0(0x97, "AddMemCardDevice");
  op0(0x98, "AddDuartTtyDevice");
  op0(0x99, "AddDummyTtyDevice");
  op0(0x9a, "AddMessageWindowDevice");
  op0(0x9b, "AddCdromSimDevice");
  op_(0x9c, "SetConf");
  op_(0x9d, "GetConf");
  op_(0x9e, "SetCdromIrqAutoAbort");
  op_(0x9f, "SetMemSize");
  op0(0xa0, "WarmBoot");
  op_(0xa1, "SystemErrorBootOrDiskFailure");
  op0(0xa2, "EnqueueCdIntr");
  op0(0xa3, "DequeueCdIntr");
  op_(0xa4, "CdGetLbn");
  op_(0xa5, "CdReadSector");
  op0(0xa6, "CdGetStatus");
  op0(0xa7, "bu_callback_okay");
  op0(0xa8, "bu_callback_err_write");
  op0(0xa9, "bu_callback_err_busy");
  op0(0xaa, "bu_callback_err_eject");
  op_(0xab, "_card_info");
  op_(0xac, "_card_async_load_directory");
  op_(0xad, "set_card_auto_format");
  op0(0xae, "bu_callback_err_prev_write");
  op_(0xaf, "card_write_test");
  op_(0xb2, "ioabort_raw");
  op_(0xb4, "GetSystemInfo");
  }

  if(pc == 0xb0) switch(cpu.ipu.r[9]) {
  op_(0x00, "alloc_kernel_memory");
  op_(0x01, "free_kernel_memory");
  op_(0x02, "init_timer");
  op_(0x03, "get_timer");
  op_(0x04, "enable_timer_irq");
  op_(0x05, "disable_timer_irq");
  op_(0x06, "restart_timer");
  op2(0x07, "DeliverEvent", event(p0), spec(p1));
  op4(0x08, "OpenEvent", event(p0), spec(p1), hex32(p2), hex32(p3));
  op1(0x09, "CloseEvent", event(p0));
  op1(0x0a, "WaitEvent", event(p0));
  op1(0x0b, "TestEvent", event(p0));
  op1(0x0c, "EnableEvent", event(p0));
  op1(0x0d, "DisableEvent", event(p0));
  op_(0x0e, "OpenThread");
  op_(0x0f, "CloseThread");
  op_(0x10, "ChangeThread");
  op_(0x12, "InitPad");
  op0(0x13, "StartPad");
  op0(0x14, "StopPad");
  op_(0x15, "OutdatedPadInitAndStart");
  op0(0x16, "OutdatedPadGetButtons");
//op0(0x17, "ReturnFromException");
  op0(0x18, "SetDefaultExitFromException");
  op_(0x19, "SetCustomExitFromException");
  op2(0x20, "UnDeliverEvent", event(p0), spec(p1));
  op2(0x32, "FileOpen", text(p0), hex32(p1));
  op3(0x33, "FileSeek", hex8(p0), hex32(p1), hex8(p2));
  op3(0x34, "FileRead", hex8(p0), hex32(p1), hex32(p2));
  op3(0x35, "FileWrite", hex8(p0), hex32(p1), hex32(p2));
  op1(0x36, "FileClose", hex8(p0));
  op_(0x37, "FileIoctl");
  op_(0x38, "exit");
  op_(0x39, "FileGetDeviceFlag");
  op_(0x3a, "FileGetc");
  op_(0x3b, "FilePutc");
  op0(0x3c, "std_in_getchar");
  op1(0x3d, "std_out_putchar", character(p0));
  op_(0x3e, "std_in_gets");
  op1(0x3f, "std_out_puts", text(p0));
  op_(0x40, "chdir");
  op_(0x41, "FormatDevice");
  op_(0x42, "firstfile");
  op_(0x43, "nextfile");
  op_(0x44, "FileRename");
  op_(0x45, "FileDelete");
  op_(0x46, "FileUndelete");
  op_(0x47, "AddDevice");
  op_(0x48, "RemoveDevice");
  op0(0x49, "PrintInstalledDevices");
  op_(0x4a, "InitCard");
  op0(0x4b, "StartCard");
  op0(0x4c, "StopCard");
  op_(0x4d, "_card_info_subfunc");
  op_(0x4e, "write_card_sector");
  op_(0x4f, "read_card_sector");
  op0(0x50, "allow_new_card");
  op_(0x51, "Krom2RawAdd");
  op_(0x53, "Krom2Offset");
  op0(0x54, "GetLastError");
  op_(0x55, "GetLastFileError");
  op_(0x56, "GetC0Table");
  op_(0x57, "GetB0Table");
  op0(0x58, "get_bu_callback_port");
  op_(0x59, "testdevice");
  op_(0x5b, "ChangeClearPad");
  op_(0x5c, "get_card_status");
  op_(0x5d, "wait_card_status");
  }

  if(pc == 0xc0) switch(cpu.ipu.r[9]) {
  op_(0x00, "EnqueueTimerAndVblankIrqs");
  op_(0x01, "EnqueueSyscallHandler");
  op_(0x02, "SysEnqIntRP");
  op_(0x03, "SysDeqIntRP");
  op0(0x04, "get_free_EvCB_slot");
  op0(0x05, "get_free_TCB_slot");
  op0(0x06, "ExceptionHandler");
  op0(0x07, "InstallExceptionHandlers");
  op_(0x08, "SysInitMemory");
  op0(0x09, "SysInitKernelVariables");
  op_(0x0a, "ChangeClearRCnt");
  op_(0x0c, "InitDefInt");
  op_(0x0d, "SetIrqAutoAck");
  op_(0x12, "InstallDevices");
  op0(0x13, "FlushStdInOutPut");
  op_(0x15, "tty_cdevinput");
  op0(0x16, "tty_cdevscan");
  op_(0x17, "tty_circgetc");
  op_(0x18, "tty_circputc");
  op_(0x19, "ioabort");
  op_(0x1a, "set_card_find_mode");
  op_(0x1b, "KernelRedirect");
  op0(0x1c, "AdjustA0Table");
  op0(0x1d, "get_card_find_mode");
  }

  //todo: handle break function calls

  #undef op_
  #undef op0
  #undef op1
  #undef op2
  #undef op3
  #undef op4

  if(call) {
    tracer.function->notify(call);
  }
}
