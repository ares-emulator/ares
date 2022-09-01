auto DD::command(n16 command) -> void {
  ctl.error.undefinedCommand = 0;
  switch(command) {
    case Command::Nop: {} break;
    case Command::ReadSeek: {} break;
    case Command::WriteSeek: {} break;
    case Command::Recalibration: {} break;
    case Command::Sleep: {} break;
    case Command::Start: {} break;
    case Command::SetStandby: {} break;
    case Command::SetSleep: {} break;
    case Command::ClearChangeFlag: {
      io.status.diskChanged = 0;
    } break;
    case Command::ClearResetFlag: {
      io.status.diskChanged = 0;
      io.status.resetState = 0;
    } break;
    case Command::ReadVersion: {} break;
    case Command::SetDiskType: {} break;
    case Command::RequestStatus: {} break;
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
    case Command::SetLEDBlinkRate: {} break;
    case Command::ReadProgramVersion: {} break;
    default: {
      ctl.error.undefinedCommand = 1;
    } break;
  }

  raise(IRQ::MECHA);
}
