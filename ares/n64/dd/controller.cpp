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
		case Command::ClearChangeFlag: {} break;
		case Command::ClearResetFlag: {} break;
		case Command::ReadVersion: {} break;
		case Command::SetDiskType: {} break;
		case Command::RequestStatus: {} break;
		case Command::Standby: {} break;
		case Command::IndexLockRetry: {} break;
		case Command::SetRTCYearMonth: {} break;
		case Command::SetRTCDayHour: {} break;
		case Command::SetRTCMinuteSecond: {} break;
		case Command::GetRTCYearMonth: {} break;
		case Command::GetRTCDayHour: {} break;
		case Command::GetRTCMinuteSecond: {} break;
		case Command::SetLEDBlinkRate: {} break;
		case Command::ReadProgramVersion: {} break;
		default: {
			ctl.error.undefinedCommand = 1;
		} break;
	}

	raise(IRQ::MECHA);
}
