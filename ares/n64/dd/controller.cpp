auto DD::command(n16 command) -> void {
  ctl.error.undefinedCommand = 0;
  switch(command) {
    case Command::Nop: {} break;
    case Command::ReadSeek: {
      io.status.writeProtect = 0;
      io.currentTrack = io.data | 0x6000;
      seekTrack();
    } break;
    case Command::WriteSeek: {
      io.currentTrack = io.data | 0x6000;
      io.status.writeProtect = seekTrack();
    } break;
    case Command::Recalibration: {} break;
    case Command::Sleep: {} break;
    case Command::Start: {} break;
    case Command::SetStandby: {
      if (!io.data.bit(24)) {
        ctl.standbyDelayDisable = 1;
      } else {
        ctl.standbyDelayDisable = 0;

        ctl.standbyDelay = io.data.bit(16,23);
        if (ctl.standbyDelay < 1) ctl.standbyDelay = 1;
        if (ctl.standbyDelay > 0x10) ctl.standbyDelay = 0x10;
        ctl.standbyDelay *= 0x17;
      }
    } break;
    case Command::SetSleep: {
      if (!io.data.bit(24)) {
        ctl.sleepDelayDisable = 1;
      } else {
        ctl.sleepDelayDisable = 0;

        ctl.sleepDelay = io.data.bit(16,23);
        if (ctl.sleepDelay > 0x96) ctl.sleepDelay = 0x96;
        ctl.sleepDelay *= 0x17;
      }
    } break;
    case Command::ClearChangeFlag: {
      io.status.diskChanged = 0;
    } break;
    case Command::ClearResetFlag: {
      io.status.diskChanged = 0;
      io.status.resetState = 0;
    } break;
    case Command::ReadVersion: {
      if (!io.data.bit(0)) io.data = 0x0114;
      else io.data = 0x5300;
    } break;
    case Command::SetDiskType: {
      ctl.diskType = io.data.bit(0, 3);
    } break;
    case Command::RequestStatus: {
      io.data.bit(0) = ctl.error.selfDiagnostic;
      io.data.bit(1) = ctl.error.servoNG;
      io.data.bit(2) = ctl.error.indexGapNG;
      io.data.bit(3) = ctl.error.timeout;
      io.data.bit(4) = ctl.error.undefinedCommand;
      io.data.bit(5) = ctl.error.invalidParam;
      io.data.bit(6) = ctl.error.unknown;
    } break;
    case Command::Standby: {} break;
    case Command::IndexLockRetry: {} break;
    case Command::SetRTCYearMonth: {
      rtc.write<Half>(0, io.data);
    } break;
    case Command::SetRTCDayHour: {
      rtc.write<Half>(2, io.data);
    } break;
    case Command::SetRTCMinuteSecond: {
      rtc.write<Half>(4, io.data);
    } break;
    case Command::GetRTCYearMonth: {
      io.data = rtc.read<Half>(0);
      } break;
    case Command::GetRTCDayHour: {
      io.data = rtc.read<Half>(2);
      } break;
    case Command::GetRTCMinuteSecond: {
      io.data = rtc.read<Half>(4);
      } break;
    case Command::SetLEDBlinkRate: {
      if (io.data.bit(24,31) != 0) ctl.ledOnTime = io.data.bit(24,31);
      if (io.data.bit(16,23) != 0) ctl.ledOffTime = io.data.bit(16,23);
    } break;
    case Command::ReadProgramVersion: {
      io.data = 0x0003;
    } break;
    default: {
      ctl.error.undefinedCommand = 1;
    } break;
  }

  queue.insert(Queue::DD_MECHA_Response, 500);
}

auto DD::mechaResponse() -> void {
  raise(IRQ::MECHA);
}
