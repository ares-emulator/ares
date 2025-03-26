// Pioneer PD6103A
auto MCD::LD::load() -> void {
  currentDriveState = 0x02; // 0x02 = CD door closed
  targetDriveState = currentDriveState;
  currentPauseState = false;
  targetPauseState = false;
  areOutputRegsFrozen = false;
  areInputRegsFrozen = false;
  currentMdGraphicsFader = 0x3F;
}

auto MCD::LD::unload() -> void {
}

auto MCD::LD::read(n24 address) -> n8 {
  bool isOutput = (address & 0x80);
  u8 regNum = (address & 0x3f) >> 1;
//  ares::_debug.reset();
//  debug(unverified, "[MCD::readLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L));

  // Retieve the current value of the target register
  n8 data = 0;
  if (!isOutput) {
    // Reading back the input registers always returns the last written value to that register
    data = inputRegs[regNum];
  } else if (areOutputRegsFrozen) {
    // Reading the output register block when frozen doesn't require us to update the values
    data = outputFrozenRegs[regNum];
  } else {
    // Our output register block isn't frozen, so we generate the data value based on the current system state.
    data = getOutputRegisterValue(regNum);
  }

  //  debug(unverified, "[MCD::readLD] reg=0x", hex(regNum, 2L), " = ", hex(data, 2L));
  if (!isOutput && (regNum != 0x00)) {
    debug(unusual, "[MCD::readLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
  }
  return data;
}

auto MCD::LD::write(n24 address, n8 data) -> void {
  bool isOutput = (address & 0x80);
  u8 regNum = (address & 0x3f) >> 1;
  ares::_debug.reset();
//  debug(unverified, "[MCD::writeLD] reg=0x", hex(regNum, 2L), " = ", hex(data, 2L));
  if (regNum != 0x00) {
    debug(unverified, "[MCD::writeLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
    if (isOutput) {
      debug(unusual, "[MCD::writeLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
    }
  }

  if (!isOutput) {
    if (areInputRegsFrozen) {
      // If the input registers are currently frozen, update the frozen state of the target register.
      inputFrozenRegs[regNum] = data;

      // Only trigger changes on a few limited registers while frozen
      if (regNum == 0x00) {
        // If we're changing the input register frozen state, perform the necessary updates now.
        auto previousInputRegsFrozenState = areInputRegsFrozen;
        areInputRegsFrozen = !data.bit(7);
        if (!areInputRegsFrozen && previousInputRegsFrozenState) {
          // If the input register block is being unfrozen, apply all the input registers now with their current values
          // from the frozen block. Note that we push all the seek registers through together first in a block. Since
          // these can trigger immediately on write, we need to make sure they're all set together.
          inputRegs[0x06] = inputFrozenRegs[0x06];
          inputRegs[0x07] = inputFrozenRegs[0x07];
          inputRegs[0x08] = inputFrozenRegs[0x08];
          inputRegs[0x09] = inputFrozenRegs[0x09];
          inputRegs[0x0A] = inputFrozenRegs[0x0A];
          inputRegs[0x0B] = inputFrozenRegs[0x0B];
          for (int i = 0; i < inputRegisterCount; ++i) {
            processInputRegisterWrite(regNum, inputFrozenRegs[i]);
          }
        } else if (areInputRegsFrozen && !previousInputRegsFrozenState) {
          // If the input register block is being frozen, copy the current register state into the frozen register state.
          for (int i = 0; i < inputRegisterCount; ++i) {
            inputFrozenRegs[i] = inputRegs[i];
          }
        }
      }

      //##TODO## Other registers are known to allow updates when frozen. See our documentation for input reg 0x00.
    } else {
      // Trigger any required behaviour in response to this input register write
      processInputRegisterWrite(regNum, data);
    }
  } else if (areOutputRegsFrozen) {
    // Perform writes to the output register block while the register output block is frozen. In this state, most
    // locations can be written to freely.
    if (regNum == 0x00) {
      // The upper two bits of register 0x00 still update when output registers are frozen
      outputFrozenRegs[regNum] = (data & 0x3F) | (outputRegs[regNum] & 0xC0);
      // Bits 1-5 are not driven, so writes to them are retained forever.
      outputRegs[regNum] = (data & 0x3E) | (outputRegs[regNum] & 0xC1);
    } else {
      outputFrozenRegs[regNum] = data;
      if (regNum >= 0x1A) {
        // Register positions 0x1A to 0x1F are not driven, and retain all values written.
        outputRegs[regNum] = data;
      }
    }
  } else {
    // Perform writes to the output register block while the register output block is not frozen. Most writes will not
    // work, or at least the written values will have reverted by the time another read can be attempted.
    if (regNum == 0x00) {
      // Bits 1-5 are not driven, so writes to them are retained forever.
      outputRegs[regNum] = (data & 0x3E) | (outputRegs[regNum] & 0xC1);
    } else if (regNum >= 0x1A) {
      // Register positions 0x1A to 0x1F are not driven, and retain all values written.
      outputRegs[regNum] = data;
    }
  }
}

auto MCD::LD::getOutputRegisterValue(int regNum) -> n8
{
  // Retrieve the previous value of the target register
  n8 previousData = outputRegs[regNum];

  // Start the new output value in the same state as the previous value
  n8 data = previousData;

  // Update the output value
  u8 flags;
  u8 minute;
  u8 second;
  u8 frame;
  switch (regNum) {
  case 0x00:
    //         --------------------------------- (Buffered in $5910)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x00|-------------------------------|
    // 0xFDFE81|*U7|*U6|        *-         |*U0|
    //         ---------------------------------
    // ##NEW## 2025
    // *U7: NOT latched from reg 0 as said below. Seen set to 1 always right now.
    // *U6: Also NOT latched from reg 0 as said below. Seen set to 0 always right now.
    // *-: Not driven. Retains last value written to it.
    // ##OLD## Before 2025
    // *U7: Latched from input register 0
    // *U6: Latched from input register 0
    // *U0: Unknown. The first output register bit 0 is directly tied to the input register bit 0 here.
    // ##OLD## Preserved when reading, modifying, and writing back this register.
    data.bit(7) = 1;
    data.bit(6) = 0;
    data.bit(0) = inputRegs[0x00].bit(0);
    break;
  case 0x01:
    //         --------------------------------- (Buffered in $5911)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x01|-------------------------------|
    // 0xFDFE83|*U7| ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // *U7: Unknown. Always seen set to 1 so far. Analysis of DRVINIT suggests this entire register may somehow represent the
    //      requested or desired drive state, while output reg 0x02 gives the true, live state. The bios compares the cached
    //      contents of output reg 0x01 with the live state of 0x02 to determine when a drive operation is complete.
    data = 0;
    data.bit(7) = 1;
    break;
  case 0x02:
    //         --------------------------------- (Buffered in $5912)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x02|-------------------------------|
    // 0xFDFE85|*U7| ? |*U5|*U4| ? | ? | ? | ? |
    //         ---------------------------------
    // *U7: Set to 1 when an LD is in the tray
    // *U5: Set to 1 when a CD is in the tray
    // *U4: Set to 1 when the the drive tray is closed and empty
    // ##NEW## 2025
    // 0xA5 when GGV1069 is spun up (mech request 0x04), 0x80 when it is loaded but not spinning. 0xA5 persisted after stop (mech request 0x03, then 0x02)
    // 0xC5 when Triad Stone is spun up (mech request 0x04), 0x80 when it is loaded but not spinning. 0xC5 persisted after stop (mech request 0x03, then 0x02)
    //   Actually seen to start as 0xC0, then go to 0xC4, then quickly transition to 0xC5.
    // 0xC6 when a CLV full size Laserdisc is spun up
    // 0x28 when a CD-V disc (PAL) is spun up
    // 0x20 when an audio CD is spun up
    data = 0;
    data.bit(7) = 0; // 1 = LD in tray
    data.bit(5) = currentDriveState >= 0x02; // 1 = CD in tray
    data.bit(4) = 0; // 1 = CD door closed and empty
    break;
  case 0x03:
    //         --------------------------------- (Buffered in $5913)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x03|-------------------------------|
    // 0xFDFE87| ? |*U6| ? |*U4|*U3| ? | ? | ? |
    //         ---------------------------------
    // ##NEW## 2025
    // *U3: Set during spinup of 30/20cm CAV/CLV disc a bit after U6, and a CD without U6. Cleared when drive tray opened.
    // *U4: Set during spinup of 20cm disc a bit after U6, very slightly before U3 but almost same time. Cleared when drive tray opened.
    // *U6: Set during spinup of 30/20cm CAV/CLV disc. Cleared when drive tray opened.
    // ##OLD## Pre 2025
    // *U4: Unknown. DRVINIT tests this, and only reads TOC info from the loaded disk if it is set to 0. It's possible this
    //      is an error in the bios routines however, and they meant to test U4 in the above output register 0x02. This would
    //      make sense.
    //##FIX## This is supposed to be latched until the tray is ejected
    data.bit(3) = currentDriveState >= 0x04;
    break;
  case 0x04:
    //         --------------------------------- (Buffered in $5914)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x04|-------------------------------|
    // 0xFDFE89|*U7| ? |*U5| ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NEW## 2025
    // *U5: Always seen set to 1, unless an LD is currently spun up. Stays set to 1 when a CD is spun up.
    // *U7: Set to 1 if bit 4 of input register 0x0D is set, unless a CD is in the drive, whether it is spun up or not.
    // ##OLD## Before 2025
    // *U7: Set to 1 if bit 4 of input register 0x0D is set
    //##FIX## Add logic here for Laserdiscs
    data = 0;
    data.bit(5) = true;
    if (currentDriveState < 0x02) {
      data.bit(7) = inputRegs[0x0D].bit(4);
    }
    break;
  case 0x05:
    //         --------------------------------- (Buffered in $5915)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x05|-------------------------------|
    // 0xFDFE8B|       Pressed Button ID       |
    //         ---------------------------------
    // *: Returns the ID number of the currently pressed button, or 0x7F if no button is currently pressed. Note that
    //    only one button press can be handled at a time. If two buttons are pressed on the unit at the same time, this
    //    register returns 0x7F as if no button is pressed. This occurs even if the two button presses occur greatly
    //    separated in time. As soon as the second button is pressed on the unit, this register returns 0x7F until one
    //    of the buttons is released, at which time, the code of the remaining pressed button is returned. The same
    //    behaviour is seen from button presses on the remote control. If a button is pressed on the remote however, then
    //    a button is pressed on the unit, the remote is ignored, and the unit button press is seen instead. Button
    //    presses on the remote are also ignored while a button is being pressed on the unit, but do become visible when
    //    the unit button is released. The following are the known button codes:
    //   -"0-9" (remote): 0x00-0x09
    //   -"D/A/CX" (remote): 0x0C
    //   -"SCAN >>" (remote): 0x10
    //   -"SCAN <<" (remote): 0x11
    //   -"CHP/TIME" (remote): 0x13
    //   -"EJECT" (remote): 0x16
    //   -"PLAY" (remote): 0x17
    //   -"PAUSE" (remote): 0x18
    //   -"POWER" (remote): 0x1C
    //   -"AUDIO" (remote): 0x1E
    //   -"+10" (remote): 0x1F
    //   -"DISPLAY" (remote): 0x43
    //   -"CLEAR" (remote): 0x45
    //   -"SKIP >>|" (remote): 0x52
    //   -"SKIP |<<" (remote): 0x53
    //   -"PLAY/STILL" (unit): 0x6D
    //   -"LD" eject (unit): 0x6F
    //   -"DIGITAL MEMORY" (unit): 0x70
    //   -"CD" eject (unit): 0x77
    //   Note that the "RESET" button isn't a normal button. Holding this button in doesn't result in a contunuous reset.
    //   Once the reset has been processed, holding it in does nothing, and after the reset, no button press is registered in
    //   code. In addition, if another button is pressed on the unit, that button press is registered, and retained, across
    //   the reset process, even if the reset button is held in at the same time. This shows that the reset button isn't a normal
    //   digital button, and doesn't generate a button press that can be read through the same code pathway as the rest of the
    //   buttons.
    // ##TODO## Determine the exact code format sent from the remote
    // ##TODO## Search for other valid button codes using a universal remote. Other pioneer laserdisc players probably had
    // more buttons that are also compatible with this unit. There might even be hidden service menus that can only be accessed
    // using special service buttons.
    data = 0x7F; // No buttons pressed (remote)
    break;
  case 0x06:
    //         --------------------------------- (Buffered in $5916)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x06-0x0B seem related to seeking and reading. See 0x369E (CDBIOS_ROMSEEK) and 0x359A (CDBIOS_ROMREAD).
    // Reg 0x06|-------------------------------| ##NOTE## Related to reg 0x0A? See 0x2960 and 0x295E.
    // 0xFDFE8D|*U7| ? |*U5|*U4|      *U30     |
    //         ---------------------------------
    // ##NEW## 2025
    // *U7: Drive busy flag. Set when the drive is still transitioning to the state flagged in U30. Also set when a seek is being
    //      performed.
    // *U5: Set to 1 when a seek operation is currently being performed, but not yet complete.
    // ##OLD## Before 2025
    // *U7: Drive busy flag. Set when the drive is still transitioning to the state flagged in U30.
    //      ##OLD## Tested at 0x2744, 0x364A
    // *U5: The function of this bit is unknown. DRVINIT tests this, and the drive appears to be requested to open if this is set.
    //      A case of this being set on the hardware has yet to be observed however. This might be some kind of error state flag.
    // *U4: Reports on the current requested pause state. Usually this appears to reflect the state of U4 in input register 0x02,
    //      to indicate the requested pause state, and it matches the requested state even if that request is ignored. Read notes
    //      in input register 0x02 for further info on ignored pause requests. It isn't directly tied to the input register however,
    //      as it is set by the unit itself after a drive state change of 0x4 is processed, to load a disk. After this process is
    //      complete, this bit is set automatically, so that the disk is left with a paused read at the start of the disk content.
    // *U30: Current mechanical drive state. See input register 0x02 for further information. This register reports the current
    //       drive state, which may be different from the requested drive state.
    //       -Note that state changes to invalid state numbers are ignored, and do not change the number reported here.
    //       -Note that due to a possible bug, when a drive state of 0x2 is requested, to close the drive tray, that number
    //        always becomes effective here, even if the drive was already closed and video is currently playing. This is in
    //        contrast to a drive state request of 0x4, to load a disk, which does not appear here if a disk is currently playing.
    //       -Note that when a drive state request of 0x4 is being processed, this register first reports 0x4 when the disk is
    //        being loaded into the mechanism, then 0x5 when the laser moves and the disk starts to spin up, and a small read is
    //        performed (probably the TOC), then 0x4 again just before the command is complete. The state of U7 remains set to
    //        true through this process. Another case of this has also been observed. If the disk has only just been loaded into
    //        the tray, but never loaded until now, this register reports a code of 0x2 almost immediately, then once the disk is
    //        finished being loaded into the mechanism, it transitions to 0x4, and stays on this code for the duration of the
    //        operation, even during the seek and read process for the TOC.
    //       -Note that if a drive state request of 0x4 fails, IE, because of a mechanical problem loading the disk, or a digital
    //        problem determining the disk type or content, the current drive state transitions to 0x2, and the disk stops spinning.
    //        This also applies if no disk is currently in the drive. The state transitions to 0x2 almost immediately in this case.
    //       ##OLD## DRVOPEN doesn't do anything if this is set to 1. DRVINIT also compares this to 1, 2, 3, 4 and 5.
    //       ##OLD## Note: Seek sets this register to 0x02

    // Update the mechanical drive state
    if ((previousData.bit(0, 3) != currentDriveState) || seekPerformedSinceLastFlagsRead) {
      // If the drive state has changed since the last time this register was read, we pretend the drive is still
      // transitioning to the new state for one read, then we swap it over. In the real hardware, most of these
      // transitions take many seconds. We put this small wait state in here not to try and obtain accurate timing, but
      // as a defensive measure in case any code out there malfunctions if the state changes instantly without going
      // into a busy state at least once. It's possible that this could be interpreted as an error or failure to apply
      // the change.
      if (previousData.bit(7) != 1) {
        data.bit(7) = 1;
        if (seekPerformedSinceLastFlagsRead) {
          data.bit(5) = 1;
        }
      }
      else {
        data.bit(7) = 0;
        data.bit(5) = 0;
        data.bit(0, 3) = currentDriveState;
        seekPerformedSinceLastFlagsRead = false;
      }
    }

    // Update the pause state
    data.bit(4) = currentPauseState || targetPauseState;
    break;
  case 0x07: {
    //         --------------------------------- (Buffered in $5917)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x07|-------------------------------|
    // 0xFDFE8F|     *U74      |*U3|    *U20   |
    //         ---------------------------------
    // *U74: Current effective playback mode, as selected by input register 0x03. Attempts to set the playback mode to an invalid
    //       state do not adjust this register.
    // *U3:  In fast forward or frame step mode, indicates the selected step direction, as specified in input register 0x03, with
    //       one slight exception, in that when frame step mode is active, if the playback is paused, either by the step speed
    //       being set directly to 0, or reverting from 1 to 0 automatically after a frame is latched, this register always reads as
    //       cleared. In frameskip mode, this register is set if the output image isn't currently set to update at all, as is the
    //       case if the step speed is set to 1, indicating a single frame update only.
    // *U20: Current step speed, as selected by input register 0x03. Note that invalid step speed settings for the current playback
    //       mode do not change this register. Only the true current effective step speed value is displayed here.
    int newInputMode = inputRegs[0x03].bit(4, 7);
    if (newInputMode <= 0x03) {
      data.bit(4, 7) = newInputMode;
      switch (newInputMode) {
      case 0:
        data.bit(0, 3) = 0;
        break;
      case 1:
        data.bit(0, 2) = inputRegs[0x03].bit(0, 2);
        data.bit(3) = (inputRegs[0x03].bit(0, 2) == 1);
        break;
      case 2:
        data.bit(0, 3) = inputRegs[0x03].bit(0, 3);
        break;
      case 3:
        data.bit(0, 3) = inputRegs[0x03].bit(0, 3);
        if (data.bit(0, 2) == 0x07) {
          data.bit(0, 2) == 0x06;
        }
        break;
      }
    }
    break;
  }
  case 0x08:
    //         --------------------------------- (Buffered in $5918)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x08|-------------------------------| ##NOTE## Related to reg 0x0C? See 0x2960 and 0x295E.
    // 0xFDFE91|*U7|*U6| ? |*U4| ? | ? | ? |*U0|
    //         ---------------------------------
    //##NEW## 2025
    // *U0: Set when a CD is loaded and spinning. Retained until drive tray is opened. Not yet tested for Laserdiscs.
    //##OLD## 2025
    // *U7: Observed to be set when testing bad seeking operations
    // *U6: Observed to be set when a CD was detected when the drive tray was closed. The disk has not been spun up at this point.
    // *U4: Observed to be set when testing bad seeking operations
    // *U0: Unknown. DRVINIT tests this.
    //##FIX## Wrong for Laserdiscs
    //##FIX## Should be tied to disc presense
    data.bit(6) = (currentDriveState >= 2);
    data.bit(0) = (currentDriveState >= 4);
    break;
  case 0x09:
    //         --------------------------------- (Buffered in $5919)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x09|-------------------------------|
    // 0xFDFE93| ? | ? | ? |*U4| ? |*U2|*U1|*U0|
    //         ---------------------------------
    // ##NEW## 2025
    // *U4: Expanding on the below, this is very much a "last operation failed" type flag. When either register 0x02 or 0x03 are set,
    // they first clear this flag, then set it if there's an error. This means if changing reg 0x03 has flagged an error for example,
    // then you do a valid write to 0x02 that doesn't trigger an error, the flag will be cleared, despite 0x03 being invalid. It's
    // not a live effective error state.
    // ##OLD## Before 2025
    // *U4: This bit appears to be cleared if the previously requested drive code in input register 0x02 was valid, and set if it
    //      was invalid. See input register 0x02 for a list of valid codes. Note that the exact same function has also confirmed to
    //      be performed by this register relating to the playback mode in input register 0x03. If an invalid playback mode is
    //      selected, this bit is set. We have also now seen this bit set if the lower 3 bits of input register 0x06 don't correspond
    //      to one of a set of valid values.
    // *U2: Observed set along with U1 and U0 when testing bad seeking operations. May indicate a target sector number which could
    //      not be located?
    // *U1: This bit has been observed to be set along with U0 if a valid seek mode was set, but the seek operation failed.
    // *U0: This bit has been observed to be set along with U4 if the lower 3 bits of input register 0x06 don't correspond to a valid
    //      seek mode. Also seen set along with U1 if a seek fails. Also now seen to be set when the video halts for a PSC marker.
    // ##NOTE##
    // -U0, U1, and U2, along with U4 in output reg 0x09 have all now seen to be set automatically when the last track on the disk
    //  finishes playing.
    data.bit(4) = operationErrorFlag1;
    break;
  case 0x0A:
    //         --------------------------------- (Buffered in $591A)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x0A|-------------------------------| ##NOTE## Related to reg 0x0E? See 0x2960 and 0x295E.
    // 0xFDFE95| -   -   -   -   - |    SM     |
    //         ---------------------------------
    // -Returns the current state of input register 0x06. See notes on that register for more info.
    // -Note that due to direct byte value comparisons made in the subcpu bios with an infinite delay loop, we know this register must
    //  be a direct literal match for input register 0x06 on all bits.
    data = inputRegs[0x06];
    break;
  case 0x0B:
    //         --------------------------------- (Buffered in $591B)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x0B|-------------------------------|
    // 0xFDFE97|           Track No            |
    //         ---------------------------------
    // *U70: Returns the current state of input register 0x07. See notes on that register for more info.
    data = inputRegs[0x07];
    break;
  case 0x0C:
    //         --------------------------------- (Buffered in $591C)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0C|-------------------------------|
    // 0xFDFE99|           SectorNoU           |
    //         ---------------------------------
    // *U70: Returns the current state of input register 0x08. See notes on that register for more info.
    data = inputRegs[0x08];
    break;
  case 0x0D:
    //         --------------------------------- (Buffered in $591D)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0D|-------------------------------|
    // 0xFDFE9B|        Minutes/SectorNoM      |
    //         ---------------------------------
    // *Minutes: Returns the current state of input register 0x09. See notes on that register for more info.
    data = inputRegs[0x09];
    break;
  case 0x0E:
    //         --------------------------------- (Buffered in $591E)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0E|-------------------------------|
    // 0xFDFE9D|        Seconds/SectorNoL      |
    //         ---------------------------------
    // *Seconds: Returns the current state of input register 0x0A. See notes on that register for more info.
    data = inputRegs[0x0A];
    break;
  case 0x0F:
    //         --------------------------------- (Buffered in $591F)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0F|-------------------------------|
    // 0xFDFE9F|            Frames             |
    //         ---------------------------------
    // *Frames: Returns the current state of input register 0x0B. See notes on that register for more info.
    data = inputRegs[0x0B];
    break;
  case 0x10:
    //         --------------------------------- (Buffered in $5920)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x10|-------------------------------|
    // 0xFDFEA1|      Track Info Selection     |
    //         ---------------------------------
    // Track Info Selection: Returns the current state of input register 0x05. See notes on that register for more info.
    data = inputRegs[0x05];
    break;
  case 0x11: {
    //         --------------------------------- (Buffered in $5921)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x11|-------------------------------|
    // 0xFDFEA3|          TOC Flags            |
    //         ---------------------------------
    // TOC Flags: The TOC flags for the currently selected track are output here. This is the currently playing track if input
    //            register 0x05 is set to 0, otherwise it is the track number indicated in input register 0x05. Note that the
    //            format of these flags is not currently known.
    //##NEW## 2025
    // -Upper 4 bits are TOC flags. Lower 4 bits are seen at 0x1 for CDs tested so far.
    //##OLD## 2025
    // ##TODO## Decode the TOC flags, and determine if they correspond to a known standard, and how they compare with CD data.
    // ##NOTE##
    // -Seen as 0x01 for audio CD track
    // -Seen as 0x44 for first track
    // -Seen as 0x04 for second track
    // -Seen as 0x00 for invalid tracks
    // -Bits 3-0 are believed to correspond to the "Sub-channel Q Control Bits" CD code standard for the selected track. Here's
    //  an excerpt from the standard:
    //                       Table 13-22: Sub-channel Q Control Bits
    // ==============================================================================
    //  Bit           equals zero                   equals one             
    // ------------------------------------------------------------------------------
    //   0       Audio without pre-emphasis    Audio with pre-emphasis  
    //   1       Digital copy prohibited       Digital copy permitted   
    //   2       Audio track                   Data track               
    //   3       Two channel audio             Four channel audio       
    // ==============================================================================
    if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0;
    } else {
      auto trackToQuery = (selectedTrackInfo == 0 ? mcd.cdd.getCurrentTrack() : (n7)selectedTrackInfo);
      mcd.cdd.getTrackTocData(selectedTrackInfo, flags, minute, second, frame);
      data.bit(4, 7) = flags;
      data.bit(0, 3) = 0x01;
    }
    break;}
  case 0x12:
    //         --------------------------------- (Buffered in $5922)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x12|-------------------------------|
    // 0xFDFEA5|           Minutes (R)         |
    //         ---------------------------------
    // Minutes: The BCD minute value of the current seek position if input register 0x05 is set to 0, otherwise it is the recorded
    //          minute length from the TOC of the track number indicated in input register 0x05.
    if ((selectedTrackInfo == 0xA0) || (selectedTrackInfo == 0xB0)) {
      data = BCD::encode(mcd.cdd.session.firstTrack);
    } else if ((selectedTrackInfo == 0xA1) || (selectedTrackInfo == 0xB1)) {
      mcd.cdd.getLeadOutTimecode(minute, second, frame);
      data = BCD::encode(minute);
    } else if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0xFF;
    } else  if (selectedTrackInfo == 0) {
      mcd.cdd.getCurrentTimecode(minute, second, frame);
      data = BCD::encode(minute);
    } else {
      mcd.cdd.getTrackTocData(selectedTrackInfo, flags, minute, second, frame);
      data = BCD::encode(minute);
    }
    break;
  case 0x13:
    //         --------------------------------- (Buffered in $5923)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x13|-------------------------------|
    // 0xFDFEA7|           Seconds (R)         |
    //         ---------------------------------
    // Seconds: The BCD second value of the current seek position if input register 0x05 is set to 0, otherwise it is the recorded
    //          second length from the TOC of the track number indicated in input register 0x05.
    if ((selectedTrackInfo == 0xA0) || (selectedTrackInfo == 0xB0)) {
      data = BCD::encode(mcd.cdd.getTrackCount());
    } else if ((selectedTrackInfo == 0xA1) || (selectedTrackInfo == 0xB1)) {
      mcd.cdd.getLeadOutTimecode(minute, second, frame);
      data = BCD::encode(second);
    } else if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0xFF;
    } else  if (selectedTrackInfo == 0) {
      mcd.cdd.getCurrentTimecode(minute, second, frame);
      data = BCD::encode(second);
    } else {
      mcd.cdd.getTrackTocData(selectedTrackInfo, flags, minute, second, frame);
      data = BCD::encode(second);
    }
    break;
  case 0x14:
    //         --------------------------------- (Buffered in $5924)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x14|-------------------------------|
    // 0xFDFEA9|           Frames (R)          |
    //         ---------------------------------
    // Frames: The BCD frame value of the current seek position if input register 0x05 is set to 0, otherwise it is the recorded
    //         frame length from the TOC of the track number indicated in input register 0x05.
    if ((selectedTrackInfo == 0xA0) || (selectedTrackInfo == 0xB0)) {
      data = 0x00;
    } else if ((selectedTrackInfo == 0xA1) || (selectedTrackInfo == 0xB1)) {
      mcd.cdd.getLeadOutTimecode(minute, second, frame);
      data = BCD::encode(frame);
    } else if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0xFF;
    } else  if (selectedTrackInfo == 0) {
      mcd.cdd.getCurrentTimecode(minute, second, frame);
      data = BCD::encode(frame);
    } else {
      mcd.cdd.getTrackTocData(selectedTrackInfo, flags, minute, second, frame);
      data = BCD::encode(frame);
    }
    break;
  case 0x15:
    //         --------------------------------- (Buffered in $5925)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x15|-------------------------------|
    // 0xFDFEAB|         Current Track         |
    //         ---------------------------------
    // Current Track: When either a CD or LD is playing, this reports the currently playing track number.
    // ##TODO## Re-test and correct these notes for CD mode. Seems that in CD mode output regs 0x17-0x19 actually report the current
    //          time as per 0x12-0x14 perhaps, and output reg 0x16 outputs some other unknown data.
    data = BCD::encode(mcd.cdd.getCurrentTrack());
    break;
  case 0x16:
    //         --------------------------------- (Buffered in $5926)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x16|-------------------------------|
    // 0xFDFEAD|       Current SectorNoU       |
    //         ---------------------------------
    // Current SectorNoU: Upper data of current sector number, in BCD format.
    // ##OLD##
    // ##NOTE##
    // -When a music CD was playing, this seemed fixed at a value of 1. Does this report the session number for a CD?
    // ##FIX## I believe this note got applied to the wrong register
    // *U0: Observed to be set when a CD was detected when the drive tray was closed. The disk has not been spun up at this point.
    data = 0x01;
    break;
  case 0x17:
    //         --------------------------------- (Buffered in $5927)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x17|-------------------------------|
    // 0xFDFEAF|       Current SectorNoM       |
    //         ---------------------------------
    // Current SectorNoM: Middle data of current sector number, in BCD format.
    // ##NOTE##
    // -When a CD was playing, output registers 0x17-0x19 provided this counter, and it had different properties.
    // -When a CD was playing, this counter was relative to the start of the current track.
    //##FIX## Wrong for Laserdiscs
    mcd.cdd.getCurrentTrackRelativeTimecode(minute, second, frame);
    data = BCD::encode(minute);
    break;
  case 0x18:
    //         --------------------------------- (Buffered in $5928)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x18|-------------------------------|
    // 0xFDFEB1|       Current SectorNoL       |
    //         ---------------------------------
    // Current SectorNoL: Lower data of current sector number, in BCD format.
    // ##NOTE##
    // -When a CD is playing, this was observed to only count from 0 to 59.
    //##FIX## Wrong for Laserdiscs
    mcd.cdd.getCurrentTrackRelativeTimecode(minute, second, frame);
    data = BCD::encode(second);
    break;
  case 0x19:
    //         --------------------------------- (Buffered in $5929)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x19|-------------------------------|
    // 0xFDFEB3|             *U70              |
    //         ---------------------------------
    // *U70: Even lower data of some kind of seek counter in BCD form, counting from 0 to 74. Only seen to be active when a CD is
    //       playing, and not when an LD is playing.
    //##FIX## Wrong for Laserdiscs
    mcd.cdd.getCurrentTrackRelativeTimecode(minute, second, frame);
    data = BCD::encode(frame);
    break;
    // These output registers are unused and undriven.
    //##FIX## These registers report input target seek locations under some seek modes. Document them here.
  case 0x1A:
  case 0x1B:
  case 0x1C:
  case 0x1D:
  case 0x1E:
  case 0x1F:
  default:
    break;
  }

  // Record the updated data value in the output register block
  outputRegs[regNum] = data;

  // Return the calculated value to the caller
  return data;
}

auto MCD::LD::processInputRegisterWrite(int regNum, n8 data) -> void
{
  // Retrieve the previous state of the target input register
  n8 previousData = inputRegs[regNum];

  // Trigger any required changes based on the updated input register
  switch (regNum) {
  case 0x00:
    //         --------------------------------- (Buffered in $5930 (edit buffer)/ and $5050 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x00|-------------------------------|
    // 0xFDFE41|*U7|*U6| -   -   -   -   - |*U0|
    //         ---------------------------------
    // *U7: Apply register writes. When this bit is cleared, most register writes are not latched, and do not take
    //      effect until this register is modified to set this bit to true, at which point, those register writes
    //      take effect. Note that the last written data can still be read back by reading the current values of the
    //      input registers, they just don't take effect on the hardware immediately.
    //      -Note that the way this is implemented, when this register is set with this value set to true, all
    //       affected registers simply get applied immediately, whether they've changed or not. If seeking is
    //       enabled, this also means that a seek operation will be performed immediately at this point, whether
    //       any seeking registers were modified while register writes were disabled or not.
    //      -Note that not all input registers can have their writes delayed. The LD and VDP graphics faders, as well
    //       as U0 of register 0x19, have all been confirmed to take effect immediately, regardless of the state of
    //       this register. Other registers may exhibit similar behaviour.
    //      -##NEW## 2025 - We have confirmed that both U7 and U6 update regardless of U7 state, while U0 does not
    //       take effect unless U7 is set.
    //      ##TODO## Test all registers to confirm how they interact with this setting
    // *U6: Freeze output registers. When this bit is set, the output register block retains the same values, and
    //      they do not change until this bit is cleared. Note that while the output registers are frozen, they
    //      can all be written to in code, and read back again with the new values. These modifications have no
    //      apparent effect, and the correct output values are restored when the registers are unfrozen.
    //      ##NOTE## 2025 - Output register 0x00 bits U7 and U6 do not retain written values when the output registers
    //      are frozen, however U0 does. All other registers retain written values in all bits when output registers
    //      are frozen, and immediately update when they are unfrozen.
    //      ##OLD# Preserved when reading, modifying, and writing back this register. Seen to be set explicitly at
    //      0x1E98, and cleared explicitly at 0x1EFA.
    // *U0: Unknown. When this register is written to with U7 set, the first output register bit 0 latches the value
    //      set in this register location. If U7 is not set, there is no apparent effect.
    //      ##OLD## Preserved when reading, modifying, and writing back this register.
    if (data.bit(7)) {
      outputRegs[0] = data;
    }
    break;
  case 0x01:
    //         --------------------------------- (Buffered in $5931 (edit buffer)/ and $5051 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x01|-------------------------------|
    // 0xFDFE43| *U76  | -   - |     *U30      |
    //         ---------------------------------
    // *U76: Set by UNK11F based on the lower 2 bits supplied in d1. Seems to affect audio and video track selection.
    //       -0x0: No space berserker video or audio
    //        -##NOTE## New results show this enabled space berserker background audio (digital), just no video.
    //        -##NOTE## Further tests show this is not a simple register. Changing this setting to 0 from 2 or 3 disabled LD video,
    //         but retained the background audio (digital). Changing this setting from 1 to 0 disabled LD video, and also disabled comms audio.
    //       -0x1: Space berserker video, no background audio, no comms video (even field display?).
    //       -0x2/0x3: Space berserker speech (analog) and background audio (digital), with selectable field display controlled by bit 0 of input register
    //                 0x0C.
    // *U30: Set by UNK11F based on a value stored at 0x1A81. No observed effect from changing this yet.
    break;
  case 0x02: {
    //         --------------------------------- (Buffered in $5932 (edit buffer)/ and $5052 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x02|-------------------------------| ##NOTE## A lot of use with this register
    // 0xFDFE45|*U7| ? |*U5|*U4|     *U30      | ##NOTE## Related to reg 0x06 ($5916)
    //         ---------------------------------
    //##NEW## 2025
    // *U4: When testing this again with CDs, the behaviour of the pause flag not being effective when triggered from the tray open
    //      state along with a 0x5 command could not be repeated.
    //      ##TODO## Compare with LDs and re-test
    // *U30:
    //       -0x7: Tests show this works like play, except it doesn't do anything if the current drive state is not 0x5. No error
    //             flags are set to indicate failure if this is not the case, it simply doesn't do anything, and output register 0x06
    //             continues to report the previously active drive state. Note that even when correctly applied from drive state 0x5,
    //             the active drive state stays at 0x5. This then acts more like a command than an active state.
    //##OLD## Before 2025
    // *U7: Flags the CD tray to be opened when an open command is sent and this register is set, otherwise the LD tray is opened.
    //      See the description of the open command for further notes.
    // *U5: Perform seek. When a drive code 0x5 is issued and this bit is set, the drive seeks to the last requested position through
    //      the location registers and begins playback from there, discarding the current read location. If this bit is set, any changes
    //      to the current seek mode or target seek location registers will, in most cases, trigger an immediate seek operation to the
    //      specified target.
    //      ##TODO## Do more testing on exactly which registers trigger seeking immediately and under which modes.
    //      ##OLD## Cleared at 0x2CF6. Set in ROMSEEK.
    // *U4: Pause playback. When a drive code 0x5 is issued and this bit is set, any current read operation seems to be paused, and the
    //      LD video output is blanked. The read resumes from the current location as soon as a drive code 0x5 is issued again with
    //      this bit clear.
    //      -Note that this bit is ignored if the drive tray is currently open and a drive code 0x5 is issued, and instead, the disk
    //       starts playing when it is loaded. This is really a firmware bug, but that's how the hardware behaves.
    //      -Note that this bit is only effective when it transitions between 0 and 1 and a valid drive code is issued along with it.
    //       For example, if a disk is currently playing and an invalid drive code of 0x8 is issued, the playback is not paused. If a
    //       drive code of 0x5 is then issued and U4 is set however, playback will not be paused either. Playback can only be paused
    //       again if a valid drive code is issued with U4 cleared, after which another drive command can then be issued with U4 set
    //       to pause playback.
    //      ##OLD## Set in ROMSEEK
    // *U30: Requested mechanical drive state. Setting this register causes a drive request to be performed. The following state
    //       codes have been observed:
    //       -0x7: Unknown. Never seen as the active state, but safe for use with pause/resume. See U4 for more info.
    //       -0x5: Play disk. The exact behaviour of this state depends on the state of U4 and U5 when this state change is
    //             requested. This command closes the tray and spins up the disk if required.
    //       -0x4: Load disk. Causes the currently present disk in the tray to spin up. This command does nothing if a disk is
    //             already spinning, and U4 in output reg 0x09 is set.
    //       -0x3: Unload disk. Causes the disk to stop spinning. Does nothing if this is already the case.
    //       -0x2: Closes the drive tray, either the CD or LD tray, whichever one is open. Does nothing if no trays are open.
    //       -0x1: Opens the drive tray. Does nothing if the tray is already open. Note that whether the CD or LD drive tray is
    //             opened depends on the state of U7. If U7 is set, the CD tray is opened, while the LD tray is opened if U7 is
    //             clear. Note also that if an LD is currently in the drive tray, the state of U7 is ignored, and the LD tray is
    //             always opened. If a CD is in the drive tray however, either the LD or CD tray can be opened. The CD tray sits
    //             within the LD tray, so it's possible to remove a CD from the drive by opening the LD tray, but the reverse is
    //             not true.
    //       -0x0: Unknown. Never seen as the active state, but safe for use with pause/resume. See U4 for more info.
    //       No other drive codes appear to be valid, based on observed behaviour such as output register 0x09 bit 4, and the
    //       paused read state change behaviour.
    // Note: ROMSEEK Sets this register to 0x35 as the last step when performing a seek operation. It is also implied
    //       that bits 4 and 5 are separate from each other and from the lower 4 bits, as these bits are set separately.
    // ##NOTE##
    // -This register actively changes output register 0x0E
    // -Other bits in this register actively effect things. We've seen seeks not play automatically, seeks not re-seek if we're currently playing from that location, etc. More testing required.
    // -This register also seems complex. If we're playing with the register data set to 0x25, then we scroll back from 0x20 to 0x15, nothing happens, but if we jump straight to 0x15, playback stops.
    //  If we then scroll up to 0x20, playback resumes. 0x17 has the same effect.

    // Update the seek enable state
    seekEnabled = data.bit(5);

    // Clear the operation error flag before we do anything
    operationErrorFlag1 = false;

    // Update the pause state if required. Note that hardware tests have shown that the pause flag is only effective
    // when it changes state at the same time as drive state 0x05 or 0x07 requests are being issued, otherwise it is
    // ignored. Note that this does mean if the flag changes state along with a different drive state being issued, the
    // pause flag will have to be set back to its old value again, then a second register write made along with a drive
    // state of 0x05 or 0x07 in order for it to be effective.
    auto newDriveState = data.bit(0, 3);
    auto pauseFlag = data.bit(4);
    auto previousPauseFlag = previousData.bit(4);
    if (pauseFlag != previousPauseFlag) {
      if ((newDriveState == 0x5) || (newDriveState == 0x7)) {
        targetPauseState = pauseFlag;
        currentPauseState = targetPauseState;
      }
    }

    // Update the mechanical drive state
    if (newDriveState != 0) {
      // Update the target drive state
      targetDriveState = newDriveState;

      // Apply the new requested drive state
      switch (targetDriveState) {
      case 0x01: // Open tray
        mcd.cdd.eject();
        currentDriveState = 0x01;
        break;
      case 0x02: // Close tray
        mcd.cdd.stop();
        currentDriveState = 0x02;
        break;
      case 0x03: // Unload disc
        mcd.cdd.stop();
        currentDriveState = 0x03;
        break;
      case 0x04: // Load disc
        // If the disc isn't already loaded, insert it and seek to the start, otherwise do nothing.
        if (currentDriveState <= 3) {
          mcd.cdd.insert();
          mcd.cdd.seekToTrack(1, true);
          seekPerformedSinceLastFlagsRead = true;
          currentPauseState = true;
        }
        currentDriveState = 0x04;
        break;
      case 0x05: // Load and play disc
        // If the disc isn't already loaded, insert it and seek to the start, otherwise do nothing.
        if (currentDriveState <= 3) {
          mcd.cdd.insert();
          mcd.cdd.seekToTrack(1, true);
          seekPerformedSinceLastFlagsRead = true;
          currentPauseState = true;
        }
        // If seeking is enabled, perform the currently active seek operation.
        if (seekEnabled) {
          performSeekWithCurrentState();
        }
        // Either play or pause the disc depending on the pause flag
        if (targetPauseState) {
          mcd.cdd.pause();
        } else {
          mcd.cdd.play();
        }
        currentPauseState = targetPauseState;
        currentDriveState = 0x05;
        break;
      case 0x07: // Toggle pause from running state
        // Note that as this is a command rather than an actual state change, we don't update the current drive state
        // here.
        if (currentDriveState == 0x05) {
          if (targetPauseState) {
            mcd.cdd.pause();
          } else {
            mcd.cdd.play();
          }
        }
        break;
      default: // Invalid mode
        operationErrorFlag1 = true;
        break;
      }
    }
    break;}
  case 0x03: {
    //         --------------------------------- (Buffered in $5933 (edit buffer)/ and $5053 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x03|-------------------------------|
    // 0xFDFE47|     *U74      |*U3|   *U20    |
    //         ---------------------------------
    //##NEW## 2025
    // -The U74 playback mode is actually 0x0-0xF, and all modes above 0x3 are invalid, flagging an error.
    //##OLD## Before 2025
    // *U74: Playback mode.
    //       -0x4-0x7: Invalid. Any settings change which targets any of these modes is ignored, and the error bit U4 is set in output
    //                 register 0x09. The one slight exception to this rule, certainly a hardware bug, seems to be that if any invalid
    //                 target is selected, regardless of the state of any of the other bits in that write attempt to this register, if
    //                 fast forward mode is currently active, and the current step speed is 0x3 or above, when the invalid target is
    //                 written, the current playback mode, step direction, and step speed, will all revert to the last settings that
    //                 were applied which were not setting up a fast forward operation with a step speed of 0x3 or above. This occurs
    //                 regardless of the old or new step direction setting, and regardless of how many writes have occurred to setup a
    //                 fast forward operation with a step speed of 0x03 or above, since the previous settings being reverted to were
    //                 active.
    //       -0x3: Fast forward. When this setting is enabled, frames will advance faster than the normal rate, at a rate specified by
    //             bits 0-2. See the notes for these bits for additional info.
    //       -0x2: Frame stepping. When this setting is enabled, audio output is disabled, and frames will automatically advance at
    //             the rate specified by bits 0-2.
    //       -0x1: Frame skipping. When this setting is enabled, during playback, the audio plays as normal, but the video skips frames
    //             at the specified rate. The overall speed of playback is the same, but the image will only update at the interval
    //             specified.
    //       -0x0: Normal playback.
    // *U3: Step direction. When fast forward or frame stepping are active and this bit is set to 1, stepping occurs backwards,
    //      otherwise it occurs forwards. This bit is ignored for frameskip mode.
    // *U20: Step speed. This has no effect under normal playback mode, but when frame stepping or skipping is active, this controls
    //       the rate at which updates occur. The following are the observed rates:
    //       -0x7: 1 frame every 3 seconds
    //       -0x6: 1 frame per second
    //       -0x5: 1 frame every 0.5 seconds
    //       -0x4: 1 frame every 0.25 seconds
    //       -0x3: 1 frame every 0.133r seconds (4 frames every 3 seconds)
    //       -0x2: 1 frame every 0.0625 seconds (100 frames every 16 seconds)
    //       -0x1: 1 frame only. The image will not update after the initial frame. Note that under frame step mode, output register
    //             0x07 will report this step speed as 0x1 only until the single frame step has been performed, after which, the output
    //             register will now state a value of 0x0.
    //       -0x0: 0 frames. This pauses playback in frame stepping mode, and performs a normal playback in frame skipping mode.
    //       Under fast forward mode, this register is applied differently. The following are the observed rates in fast forward:
    //       -0x7/0x06: Search mode. Plays 0.75 seconds of footage forwards in time, with audio, then jumps either forward or back
    //        4 seconds from the resulting point. Note that setting this register to 0x07 is the same as 0x06, but 0x06 is the value
    //        that is reported back as the current mode in either case by output register 0x07.
    //       -0x5: 20x speed
    //       -0x4: 14x speed
    //       -0x3: 8x speed
    //       -0x2: 3x speed
    //       -0x1: 2x speed
    //       -0x0: 1x speed (Normal playback)
    // ##NOTE## All this talk of "frames" is relative to the video stream. It does not correspond with an apparent "frame" as reported
    //          by the MM:SS:FF output regs 0x12-0x14, but rather, apparent changes in the image on the screen. There does seem to be a
    //          one to one correspondence with video frames and sector numbers however, as reported in output regs 0x16-0x18.
    // ##OLD##
    // *U4: Set at 0x2314. Tested in DRVINIT.

    // Clear the operation error flag before we do anything
    operationErrorFlag1 = false;

    //##TODO## Implement playback mode support. Note that this does work for CDs as well as LDs.
    auto newPlaybackMode = data.bit(4, 7);
    if (newPlaybackMode >= 0x04) {
      operationErrorFlag1 = true;
    }
    break;
  }
  case 0x04:
    //         --------------------------------- (Buffered in $5934 (edit buffer)/ and $5054 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x04|-------------------------------|
    // 0xFDFE49| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x05:
    //         --------------------------------- (Buffered in $5935 (edit buffer)/ and $5055 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x05|-------------------------------|
    // 0xFDFE4B|      Track Info Selection     |
    //         ---------------------------------
    // Track Info Selection: If this is set to 0, output registers 0x11-0x14 report info on the track currently being played,
    //                       otherwise they report TOC information on the specified track number. Output register 0x10 returns
    //                       the value of this register.
    // ##NEW## 2025
    // -Value 0xA0 returns the first valid track number in output reg 0x12, and the last valid track number in 0x13. Used by bios.
    // -Value 0xA1 appears to returns the start of leadout time in the min/sec/frame regs
    // -Values 0xB0 and 0xB1 appear identical to 0xAx counterparts for a SegaCD game, but must be able to be something different.
    // -All these values are used by the bios
    // ##OLD## Before 2025
    // ##NOTE##
    // -Only track numbers 0x01-0x99 are valid. Note that although the odd end position of 0x99, all hex values from 0x01-0x99
    //  are valid, this isn't a BCD value.
    // -If an invalid track number is selected (>0x99), output registers 0x11-0x14 actually report internal data instead. It appears
    //  that somewhere, there's an internal data buffer, which the TOC information is loaded into. This isn't the only info in this
    //  buffer however. Upper register values have been observed to contain the internal state of data buffers which are directed to
    //  other output registers, as well as lots of unidentified data, some of which changes constantly (possibly running counters or
    //  timers of some kind). The correct content for these upper register values will take quite awhile to map out.
    // -Entering a value of 0xFF returns the exact same data as a value of 0x00. This may be because the actual current counter data
    //  is stored in the internal memory at this location, rather than this being a properly supported value.
    // -Testing under CD mode has confirmed this is a BCD value. Entry 0x0A was same as 0x10, 0x0B as 0x11, etc.
    // ##TODO## Document the internal upper register values accessed in this data block
    selectedTrackInfo = (data < 0x99 ? BCD::decode(data) : (u8)data);
    break;
  case 0x06: {
    //         --------------------------------- (Buffered in $5936)
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x06-0x0B seem related to seeking and reading. See 0x369E (CDBIOS_ROMSEEK) and 0x359A (CDBIOS_ROMREAD).
    // Reg 0x06|-------------------------------| ##NOTE## Related to reg 0x0A? See 0x2960 and 0x295E.
    // 0xFDFE4D| -   -   -   -   - |    SM     |
    //         ---------------------------------
    //##NEW## 2025
    // -Seeking is only performed if the current mechanical drive state is exactly 0x5. Invalid modes like 0x6 and 0x8 do not perform
    // seeking when writing to the SM value above, or when changing the following seek target registers. Likewie, the valid drive state
    // of 0x7, which previously seemed the same as play mode, now has a purpose. Under this mode seeking is not active either, however
    // the player is still in a valid play mode.
    //##OLD## Before 2025
    // SM: Seek mode
    //       -0x7: ?? Seems the same as 0x03
    //       -0x6: Seek to sector immedate. This is essentially the same as mode 0x0, except that writing to input regs 0x07-0x0B will
    //             trigger an immediate seek operation. Note that interestingly, specifying a target sector number of 1 or 0 will always
    //             report an initial sector number of 1 on the current sector number in output regs 0x16-0x18, so 0 isn't a valid sector
    //             number. Note that as per mode 0x0, specfying a target sector number beyond the end of the disk will cause the drive
    //             to advance up to the last valid sector number.
    //       -0x5: Seek to track. Seeks to the start of the track specified by input reg 0x07. Writing to input regs 0x07-0x0B will
    //             trigger an immediate seek operation if the target track number in input reg 0x07 is valid (tested after the write
    //             if 0x07 was the register being changed), while if the target track number is invalid, no seeking will occur.
    //             ##TODO## Note that in the case the target track number is invalid, writing to input register 0x02 with the perform
    //             seek flag set will not trigger a seek operation. In fact, once this has been attempted, most other seek mode settings
    //             will also not trigger a seek operation when setting input reg 0x02, even if they normally do. Once the next successful
    //             seek operation occurs, those other seek modes update again when writing to input reg 0x02. It seems this must set some
    //             kind of persistent seek error state which isn't automatically cleared. More investigation is required.
    //       -0x4: ?? Seems the same as 0x0.
    //             ##NOTE## There's an old comment for this bit that should be investigated. It is as follows:
    //                      -Note that this bit is ignored in CD mode. Seeking is apparently only allowed based on time in CD mode.
    //       -0x3: Seek to last and stop at track. Seeks to the last valid seek target that was latched, letting playback advance until
    //             the target track number identified by input register 0x07 is reached, at which point playback will automatically
    //             pause. Modifying seek target registers while this mode is enabled will cause the current seek target sector number to
    //             be output on out registers 0x1C-0x1E, but no other effect is immediately apparent. The output regs 0x1A-0x1F can be
    //             written to at this time with the new values being latched, and only refreshed when one of the seek target input regs
    //             0x07-0x0B is again written to, at which time all output regs 0x1A-0x1F are reloaded with correct values. Note that
    //             when the target time is reached (or playback stops for any reason probably) output regs 0x1A-0x1F are all set to 0xFF.
    //             Modifying any of the seek target registers 0x09-0x0B while playback is stopped will cause output regs 0x1A-0x1F to
    //             briefly flash with the correct values, before reverting to 0xFF.
    //             ##FIX## One effect has been seen from modifying the target track number in input reg 0x07. This immediately updates
    //             the end target track, so if it is modified to a track that is less than or equal to the currently playing track,
    //             playback will immediately pause.
    //       -0x2: Seek to time. The absolute disk time given in input registers 0x09-0x0B is used to specify the target seek time.
    //             Actual drive seems to pre-seek by approximately 10 frames then advance forward. Writing to input regs 0x07-0x0B will
    //             trigger an immediate seek operation. Note that unlike seeking relative to a track, the target time isn't offset by
    //             2 seconds for pregap adjustment, it is used directly.
    //       -0x1: Seek relative to track time. The target track number in input reg 0x07 specifies the track to operate on. Input regs
    //             0x09-0x0B specify a relative time to the target track to seek to. Writing to input regs 0x07-0x0B will trigger an
    //             immediate seek operation if the target track number in input reg 0x07 is valid (tested after the write if 0x07 was
    //             the register being changed), while if the target track number is invalid, no seeking will occur. Note that in this
    //             seek mode, the system will also seek an extra 2 seconds into the track, in addition to the relative seek time
    //             specified in input regs 0x09-0x0B, so the minimum seek offset into the track is two seconds. This seems to be
    //             designed to allow this seek mode to skip the 2 second pregap for audio tracks. Note that with this seek mode, we can
    //             see in the output regs that seeking is actually performed per sector, and time frame numbers don't exactly correlate
    //             with sector boundaries. It appears that a single time frame spans 2-3 sectors, with an apparent effect of the
    //             specified time appearing to pre-seek by a random 1-4 frames, rather than hitting the exact frame specified. Note that
    //             if input register 0x02 is written to with the perform seek flag set while the target track number is invalid, a seek
    //             operation will still be triggered, but the last valid seek target that was successfully performed will be carried out
    //             instead, regardless of how many incorrect seek operations have been attempted since then.
    //       -0x0: Seek to sector. The absolute sector number given in input registers 0x08-0x0A is used to specify the target sector
    //             number to seek to. Actual drive seems to pre-seek by approximately 5 sectors then advance forward. Writing to seek
    //             target registers doesn't take effect immediately, but current values will be used at the time input register 0x02 is
    //             written to with the perform seek flag set.
    // ##OLD## Referring to when TRK was thought to be bit 0 and DSK bit 1
    // Note:
    // -The TRK and DSK bits are mutually exclusive. One of them must be set, otherwise error flags are set in the output register
    //  0x09. If both of them are set however, it causes seemingly unpredictable and significant seeking problems. Sometimes seeking
    //  stops working, returning error codes in output register 0x09 for valid seek modes after attempting a seek operation with both
    //  these settings set. Sometimes, the drive gets stuck in an apparent loop, skipping back and forth, apparently trying to service
    //  both seek requests at once. Sometimes it gives up and flags an error, and sometimes it seeks to unexpected locations. If both
    //  these bits are set and the target track number is the first track, and the target time is 0, it does work however, as the same
    //  location is targetted by both seek requests. It also works if SM is set to true, and the target sector number is the same as
    //  the starting sector for the target track. Note that U0 in output register 0x1F also appears to be changed to the set state
    //  when these bits are both set, and U0 then remains set from that point on, even if the issue is corrected. It is not currently
    //  known if this bit can be cleared after it has been set.

    // Update the current seek mode
    auto seekMode = data.bit(0, 2);
    if (currentSeekMode != seekMode) {
      currentSeekMode = seekMode;
      if (seekEnabled && (currentDriveState = 0x5) && ((currentSeekMode == 2) || (currentSeekMode == 6))) {
        performSeekWithCurrentState();
      }
    }
    break;}
  case 0x07:
    //         --------------------------------- (Buffered in $5937 (edit buffer)/ and $5057 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x07|-------------------------------|
    // 0xFDFE4F|           Track No            |
    //         ---------------------------------
    // Track No: Requested track number for seeking. Note that 0x01 is the first track on the disk, 0x02 is the second, etc.
    // ##NOTE##
    // -Seek sets this to 0x00
    // -In CD mode, this was observed to be a BCD value. A value of 0x0A was the same as 0x10, 0x0B as 0x11, etc.
    if (seekEnabled && (currentDriveState = 0x5) && ((currentSeekMode == 1) || (currentSeekMode == 2) || (currentSeekMode == 5) || (currentSeekMode == 6))) {
      performSeekWithCurrentState();
    }
    break;
  case 0x08:
    //         --------------------------------- (Buffered in $5938 (edit buffer)/ and $5058 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x08|-------------------------------| ##NOTE## Related to reg 0x0C? See 0x2960 and 0x295E.
    // 0xFDFE51|           SectorNoU           |
    //         ---------------------------------
    // SectorNoU: Upper data of seek sector number, in BCD format. The seek sector number is an actual target sector
    //            location. Data is stored in BCD form, with each nybble running from 0x0-0x9. If any digit exceeds
    //            the maximum number for that digit, it is evaluated as 0, with a carry generated into the next digit
    //            when the effective target sector number is calculated.
    // ##FIX## We know this isn't actually a sector number at all. What we do know is that when seeking, these are the
    // actual units which seeking uses. When seeking to a time, the actual time the unit arrives at might be slightly
    // before or after the target, but when using this "sector" seek mode, we hit the exact intervals requested, which
    // correspond with the error boundaries of our time-based seeking. It's not clear what these numbers represent though.
    // They don't seem to match up with the digital data sector numbers. Perhaps they're some kind of data segment numbers
    // from the LD video stream?
    // ##NOTE##
    // -The start of the video track in space berserker is at location 0x3661 in these units
    // -The ROMREAD bios routine sets this register to 0xFF, and register 0x07 to 0x00. The reason this register is set to
    //  0xFF is unknown. It appears to have no effect.
    if (seekEnabled && (currentDriveState = 0x5) && ((currentSeekMode == 1) || (currentSeekMode == 2) || (currentSeekMode == 5) || (currentSeekMode == 6))) {
      performSeekWithCurrentState();
    }
    break;
  case 0x09:
    //         --------------------------------- (Buffered in $5939 (edit buffer)/ and $5059 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x09|-------------------------------|
    // 0xFDFE53|       Minutes/SectorNoM       |
    //         ---------------------------------
    // Minutes: Requested seek position for minutes, in BCD format.
    //          -If the lower digit of this number exceeds its maximum bounds (0x0-0x9), the remainder is carried
    //           into the higher place when calculating the effective address, so 0x0A is the same as 0x10, 0x0F is
    //           the same as 0x15, etc.
    //          -In CD mode, invalid values are handled differently. If any digit exceeds the BCD bounds, it is
    //           treated as 0, and a carry is generated into the higher digit.
    // SectorNoM: Middle data of seek sector number, in BCD format. See input register 0x08.
    // ##NOTE##
    // -When reg 0x06 U20=2, allowable range is 0x00-0x2B, 0x30-0x31, 0x9B-0x9F, 0xA1-0xCB, 0xD0-0xD1. No other
    //  related registers appear to have input value restrictions.
    if (seekEnabled && (currentDriveState = 0x5) && ((currentSeekMode == 1) || (currentSeekMode == 2) || (currentSeekMode == 5) || (currentSeekMode == 6))) {
      performSeekWithCurrentState();
    }
    break;
  case 0x0A:
    //         --------------------------------- (Buffered in $593A (edit buffer)/ and $505A (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x0A|-------------------------------| ##NOTE## Related to reg 0x0E? See 0x2960 and 0x295E.
    // 0xFDFE55|       Seconds/SectorNoL       |
    //         ---------------------------------
    // Seconds: Requested seek position for seconds, in BCD format.
    //          -If the lower digit of this number exceeds its maximum bounds (0x0-0x9), the remainder is carried
    //           into the higher place when calculating the effective address, so 0x0A is the same as 0x10, 0x0F is
    //           the same as 0x15, etc. If the seconds number as a whole exceeds its maximum bounds (0x59), the
    //           effective minutes field is incremented by the number, and the effective seconds position is
    //           calculated as the remainder.
    //          -In CD mode, invalid values are handled differently. If any digit exceeds the BCD bounds, it is
    //           treated as 0, and a carry is generated into the higher digit.
    // SectorNoL: Lower data of seek sector number, in BCD format. See input register 0x08.
    if (seekEnabled && (currentDriveState = 0x5) && ((currentSeekMode == 1) || (currentSeekMode == 2) || (currentSeekMode == 5) || (currentSeekMode == 6))) {
      performSeekWithCurrentState();
    }
    break;
  case 0x0B:
    //         --------------------------------- (Buffered in $593B (edit buffer)/ and $505B (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x08-0x0B related? See 0x292A.
    // Reg 0x0B|-------------------------------|
    // 0xFDFE57|            Frames             |
    //         ---------------------------------
    // Frames: Requested seek position for frames, in BCD format.
    //         -If the lower digit of this number exceeds its maximum bounds (0x0-0x9), the remainder is carried
    //          into the higher place when calculating the effective address, so 0x0A is the same as 0x10, 0x0F is
    //          the same as 0x15, etc.
    //         -If the frame number as a whole exceeds its maximum bounds (0x74), the effective seconds field is
    //          incremented by the number, and the effective frame position is calculated as the remainder.
    //         -In CD mode, the effective read position is always 1 more frame than requested in this register, so
    //          if this value is 0x00, the frame 0x01 will be requested.
    //         -In CD mode, invalid values are handled differently. If any digit exceeds the BCD bounds, it is
    //          treated as 0, and a carry is generated into the higher digit.
    if (seekEnabled && (currentDriveState = 0x5) && ((currentSeekMode == 1) || (currentSeekMode == 2) || (currentSeekMode == 5) || (currentSeekMode == 6))) {
      performSeekWithCurrentState();
    }
    break;
  case 0x0C:
    //         --------------------------------- (Buffered in $593C (edit buffer)/ and $505C (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0C|-------------------------------|
    // 0xFDFE59|PSC|*U6|HLD|*U4|*U3|VD |DM |*U0|
    //         ---------------------------------
    // ##NEW## 2025
    // *U6: This was seen to be set when getting the MegaLD bios working. From the CD player, when chapter skip
    //      was getting stuck in "Search" mode, pressing pause would do 0x0C=0x0C, rather than the usual 0x02=0x15.
    // ##OLD## Before 2025
    // PSC: Picture Stop Cancel. If this bit is set, the LD hardware is allowed to seek past
    //      picture stop codes in the LD video stream. If this bit is not set, when a picture stop
    //      code is encountered, the hardware automatically transitions to frame step mode.
    // HLD: Image hold. If this bit is set, the stored data in the frame buffer is output
    //      instead of the video data currently being decoded. Sound playback is unaffected.
    //      Setting this bit enables the digital memory light on the front of the LaserActive,
    //      regardless of the state of DM.
    // *U4: UNK12F sets the contents of U4 and VD based on the corresponding bits in d1.b. All
    //      other bits of d1 are ignored. The function of this register is currently unknown, but
    //      the digital memory light is set when this setting is enabled, regardless of the state
    //      of DM.
    // *U3: UNK12E sets or clears this based on d1.b input parameter. Reportedly controls overlay
    //      of VDP graphics with LD video. VDP graphics are enabled when d1 is set to 1, and
    //      disabled when it is set to 0 (default). Based on further testing, this seems to
    //      actually affect the current video sync. If the VDP graphics fader isn't set to make the
    //      image invisible, the VDP image rolls over the top of the LD video when this bit is
    //      cleared, and is only stable when it is set. The MegaLD video signal is still combined
    //      with the VDP output regardless.
    // VD:  Disables MegaLD video stream when set. The digital memory light on the unit turns off
    //      when this bit is set. UNK12F sets the contents of this bit and U4 based on the
    //      corresponding bits in d1.b. All other bits of d1 are ignored.
    // DM:  Enables digital memory when set, causing the light to illuminate on the front of the
    //      LaserActive. UNK12C sets this and UNK12D clears this.
    // *U0: UNK12C sets or clears this based on d1.b input parameter, then sets DM regardless. Reportedly
    //      selects which video field to output when DM is set. Our hardware testing has been unable to
    //      reproduce this as of yet, and instead we always seem to see one of the two fields shown
    //      whenever DM is enabled, regardless of the state of U0. With DM disabled, we see both fields
    //      interlaced. Actually, further testing has got this to work. When U76 in reg 1 is set to
    //      2 or 3, this bit selects which video field to display. Either DM or U3 must also be set to 1
    //      in order for this to work. Note that setting U4 to 1 does not make field selection work, and
    //      instead the full frame is still displayed in interlaced mode, even though setting U4 to 1
    //      also illuminates the digital memory light on the front of the unit.
    // Notes:
    // -U1 and U0 are or'd with the state of another value from $5B49, appears
    // to be some kind of status flags.
    break;
  case 0x0D:
    //         --------------------------------- (Buffered in $593D (edit buffer)/ and $505D (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0D|-------------------------------|
    // 0xFDFE5B|      *U74     | ? | ? |RE |LE |
    //         ---------------------------------
    // ##NEW## 2025
    // -New testing has shown setting both RE and LE attenuates equivalent to a reg 0x0F filter of 0x40.
    // -Under CD mode at least (LD not tested) only bits 6 and 7 seem to have any effect, with these results:
    //       -00 = Normal
    //       -01 = Normal
    //       -10 = No audio
    //       -11 = Audio plays full volume (reg 0x0F ignored)
    // ##OLD## Before 2025
    // *U74: Somehow affects audio selection. The following effects were observed in space berserker:
    //       -0x0: Background audio (digital)
    //       -0x1: Voice audio (analog)
    //       -0x2: Background audio (digital)
    //       -0x3: Voice audio (analog)
    //       -0x4: Background audio (digital)
    //       -0x5: Voice audio (analog)
    //       -0x6: Background audio (digital)
    //       -0x7: Voice audio (analog)
    //       -0x8: No audio
    //       -0x9: Voice audio (analog)
    //       -0xA: No audio
    //       -0xB: Voice audio (analog)
    //       -0xC: Background audio (digital) (Ignore input reg 0x0F - Full volume always)
    //       -0xD: Voice audio (analog)
    //       -0xE: Background audio (digital) (Ignore input reg 0x0F - Full volume always)
    //       -0xF: Voice audio (analog)
    // ##TODO## - Determine the relationship between U74 and U76 in input register 0x01. All the above results come from when that
    //            value is set to 1 I believe.
    // RE:  Digital audio right exclusive. Play right track in both speakers.  Half volume normal left/right output if combined with LE.
    // LE:  Digital audio left exclusive. Play left track in both speakers. Half volume normal left/right output if combined with RE.
    // ##OLD##
    // *Unknown: Set as a complete write by UNK138.
    digitalAudioRightExclusive = data.bit(1);
    digitalAudioLeftExclusive = data.bit(0);
    break;
  case 0x0E:
    //         --------------------------------- (Buffered in $593E (edit buffer)/ and $505E (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0E|-------------------------------|
    // 0xFDFE5D|MUT| ? | ? | ? | ? | ? |RE |LE |
    //         ---------------------------------
    // MUT: Mute analog audio output
    // RE:  Analog audio right exclusive. Play right track in both speakers. No analog audio output if combined with LE.
    // LE:  Analog audio left exclusive. Play left track in both speakers. No analog audio output if combined with RE.
    break;
  case 0x0F:
    //         --------------------------------- (Buffered in $593F (edit buffer)/ and $505F (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0F|-------------------------------|
    // 0xFDFE5F|       DigitalAudioFader       |
    //         ---------------------------------
    // ##NEW## 2025
    // -Note that despite the name "fader" this is a volume, IE, higher is louder.
    // ##OLD## Before 2025
    // DigitalAudioFader: This set the attenuation of the background audio in Space Berserker. Set as a complete write by UNK130. From TascoDLX:
    //           "I think I missed a call for you. 0130 [A0] should set the volume to the correct level."
    //           "SB is constantly setting the volume with this call, so I don't know if the value gets reset somewhere and this is how they counter it."
    currentDigitalAudioFader = data;
    break;
  case 0x10:
    //         --------------------------------- (Buffered in $5940 (edit buffer)/ and $5060 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x10|-------------------------------|
    // 0xFDFE61| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x11:
    //         --------------------------------- (Buffered in $5941 (edit buffer)/ and $5061 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x11|-------------------------------|
    // 0xFDFE63| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x12:
    //         --------------------------------- (Buffered in $5942 (edit buffer)/ and $5062 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x12|-------------------------------|
    // 0xFDFE65| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x13:
    //         --------------------------------- (Buffered in $5943 (edit buffer)/ and $5063 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x13|-------------------------------|
    // 0xFDFE67| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x14:
    //         --------------------------------- (Buffered in $5944 (edit buffer)/ and $5064 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 | ##NOTE## Regs 0x11-0x14 related? See 0x2550.
    // Reg 0x14|-------------------------------|
    // 0xFDFE69| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x15:
    //         --------------------------------- (Buffered in $5945 (edit buffer)/ and $5065 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x15|-------------------------------|
    // 0xFDFE6B| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x16:
    //         --------------------------------- (Buffered in $5946 (edit buffer)/ and $5066 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x16|-------------------------------|
    // 0xFDFE6D| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x17:
    //         --------------------------------- (Buffered in $5947 (edit buffer)/ and $5067 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x17|-------------------------------|
    // 0xFDFE6F| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x18:
    //         --------------------------------- (Buffered in $5948 (edit buffer)/ and $5068 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x18|-------------------------------|
    // 0xFDFE71| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x19:
    //         --------------------------------- (Buffered in $5949 (edit buffer)/ and $5069 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x19|-------------------------------|
    // 0xFDFE73| ? | ? | ? | ? | ? | ? | ? |*U0|
    //         ---------------------------------
    // *U0: UNK136 sets this and UNK137 clears this. When set, appears to make the video stream transparent at black, with the VDP graphics
    //      appearing below it? Also seems to slightly affect colour for VDP graphics, and image quality of the LD video stream. VDP graphics
    //      quality is better when this is set, and LD graphics quality is better when this is unset.
    //      -I believe this register sets which video stream is the "primary" video stream.
    //      -When the display is "rolling" (VDP graphics are enabled and bit 3 of input register 0x0C is cleared), when this bit is clear,
    //       the video stream is positioned correctly and appears mostly correct, except for the VDP video stream rolling over it. When this
    //       bit is set, the LD video stream is offset vertically, making the vertical blanking region visible in the picture area.
    //      -Latest testing shows the following behaviour: This bit controls whether black on the LD video stream is "transparent" or not. If
    //       this bit is set, any region on the LD video stream where the effective output is black or close to black, the LD video isn't
    //       displayed, and instead the VDP image is displayed at full intensity, regardless of the current setting of the VDP graphics fader
    //       in input register 0x1A. Note that the decision about the "black level" of the LD video is also independent of the LD graphics
    //       fader in input register 0x1B, and behaves the same way regardless of this setting. Also note that there's a "trailing" effect
    //       behind displayed regions of the LD image stream where this is used. If an area of the LD image stream is used, it appears the
    //       following 8-12 or so "pixels" of the LD image output following it will be displayed too, even if they are black. This is an
    //       unstable effect, and is affected by the intensity of the colour output prior to the transition to black. There is also a
    //       "lead-in" effect which is less extreme, where the intensity of the "black" preceding a colour, and the intensity of that colour,
    //       affect how long it takes before the LD image actually appears instead of the VDP image. It appears that even for high intensity
    //       transitions, at least 1-2 pixels are lost from the output image on average before the LD video stream gains priority. A "rainbow
    //       effect" is also often visible on the first lead-in pixel. All these issues make this mode most likely very difficult to use in a
    //       useful way for a real game, especially since the "black level" between the LD video stream and the VDP image is very different,
    //       with black from the LD stream being a very noticeable grey compared to the VDP image.
    // ##TODO## Do more testing and confirm the correct operation of this register
    break;
  case 0x1A:
    //         --------------------------------- (Buffered in $594A (edit buffer)/ and $506A (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1A|-------------------------------|
    // 0xFDFE75|   VDPGraphicsFader    | -   - |
    //         ---------------------------------
    // VDPGraphicsFader: Sets the intensity level of the VDP graphics overlay. A higher value gives a stronger VDP image.
    //                   Set as a register write by $134, with the lower 6 bits shifted up.
    //                   -Note that even if this fader is set to 0, the VDP overlay can still affect the output image. It
    //                    appears that if a non-transparent VDP pixel appears on the screen, its opacity level is taken into
    //                    account, but the opacity of the LD video stream is not, so a non-transparent VDP pixel will always
    //                    be combined with a full opacity LD video stream, even if the VDP graphics fader is set to 0. This
    //                    could allow the VDP graphics layer to act as a kind of highlight mask over a shadowed LD video
    //                    stream.
    //                   -The previous note is confusing. After new testing, it appears that each colour component is calculated
    //                    in the following manner:
    //                    r = (vr*va) + (lr*la) + ((1-((va+la)/2))*vr*lr)
    //                    where:
    //                    -r is the resulting colour value
    //                    -vr is the VDP colour value
    //                    -lr is the LD video colour value
    //                    -va is the VDP attenuation
    //                    -la is the LD video attenuation
    //                    and all values in this calculation are in the range 0.0-1.0.
    currentMdGraphicsFader = data.bit(2, 7);
    break;
  case 0x1B:
    //         --------------------------------- (Buffered in $594B (edit buffer)/ and $506B (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1B|-------------------------------|
    // 0xFDFE77|    LDGraphisFader     | -   - |
    //         ---------------------------------
    // LDGraphicsFader: Sets the intensity level of the LD video signal. A lower value gives a stronger LD image. Set as a register
    //                  write by $135, with the upper 2 bits masked. Note that this is in fact an error in the BIOS routines There's
    //                  contradictory information between the status read function $12B and the fader function $135. Hardware testing
    //                  has shown the lower 2 bits have no apparent effect, and the upper 6 bits are what has an effect. The
    //                  implementation of $135 appears to be in error.
    break;
  case 0x1C:
    //         --------------------------------- (Buffered in $594C (edit buffer)/ and $506C (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1C|-------------------------------|
    // 0xFDFE79| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x1D:
    //         --------------------------------- (Buffered in $594D (edit buffer)/ and $506D (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1D|-------------------------------|
    // 0xFDFE7B| ? | ? | ? | ? | ? | ? | ? | ? |
    //         ---------------------------------
    // ##NOTE## No observed effect
    break;
  case 0x1E:
    //         --------------------------------- (Buffered in $594E (edit buffer)/ and $506E (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1E|-------------------------------|
    // 0xFDFE7D| ? | ? | ? | ? |     *U30      |
    //         ---------------------------------
    // *U30: Some kind of audio mode flag:
    //       -0x0: ??
    //       -0x1: ??
    //       -0x2/0x03: Input register 0x1F provides attenuation level for left and right analog audio (lower is louder)
    //       -0x4: ??
    //       -0x5: ??
    //       -0x6: Input register 0x1F provides attenuation level for left analog audio (lower is louder)
    //       -0x7: Input register 0x1F provides attenuation level for right analog audio (lower is louder)
    //       -0x8: ??
    //       -0x9: ??
    //       -0xA/0x0B/0xE: Initiate analog audio fade out
    //       -0xC: ??
    //       -0xD: ??
    //       -0xF: Mute right analog audio
    // ##OLD##
    // *Unknown: Set as a complete write by UNK131. Testing has shown this is not a simple state register, rather, it seems to issue
    //           commands which affect audio. Simply changing this register back to a previous value is not enough to undo a state
    //           change here, a different command needs to be issued which undoes the operation. We have seen this register remove
    //           either the left or right channel of voice audio on space berserker, as well as trigger fade out operations, where
    //           the voice audio fades out over one or two seconds, possibly on a per-speaker basis. From TascoDLX:
    //           "And, I did mention the game's init sequence, including: 0131 [02] and 0132 [00]. That would be important to setup the digital audio."
    //           Also:
    //           "0131 may select the digital track (which that bit disables), and that may play in tandem with the video. Just tossing around ideas. Not completely sure of anything at this point."
    break;
  case 0x1F:
    //         --------------------------------- (Buffered in $594F (edit buffer)/ and $506F (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1F|-------------------------------|
    // 0xFDFE7F|        AnalogAudioFader       |
    //         ---------------------------------
    // AnalogAudioFader: Set as a complete write by UNK132. This set the attenuation of the voice audio in
    //                   space berserker. A higher value gives a lower volume. Actually, we just saw a lower
    //                   value giving a higher volume. Perhaps our previous test was incorrect. From TascoDLX:
    //           "And, I did mention the game's init sequence, including: 0131 [02] and 0132 [00]. That would be important to setup the digital audio."
    break;
  default:
    debug(unusual, "[MCD::LD::processInputRegisterWrite] reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
    break;
  }

  // Update the latched input register state
  inputRegs[regNum] = data;
}

auto MCD::LD::performSeekWithCurrentState() -> void {
  //##TODO## Respect the immediate vs delayed seek modes here
  //##TODO## Fully reverse engineer, document, and implement the 0x3 and 0x7 modes which implement automatic player stop
  //functionality.
  switch (currentSeekMode) {
  case 0x00: // Seek to sector
  case 0x04: // Unknown, but so far seems same as sector mode.
  case 0x06: // Seek to sector immediate
    if (inputRegs[0x07] != 0) {
      //##TODO## Handle peculiarities of invalid BCD value decoding, including differences between LDs and CDs.
      s32 lba = (BCD::decode(inputRegs[0x08]) * 100) + BCD::decode(inputRegs[0x07]);
      mcd.cdd.seekToSector(lba, targetPauseState);
      seekPerformedSinceLastFlagsRead = true;
    }
    break;
  case 0x01: // Seek to relative track time
    if (inputRegs[0x07] != 0) {
      mcd.cdd.seekToRelativeTime(BCD::decode(inputRegs[0x07]), BCD::decode(inputRegs[0x09]), BCD::decode(inputRegs[0x0A]), BCD::decode(inputRegs[0x0B]), targetPauseState);
      seekPerformedSinceLastFlagsRead = true;
    }
    break;
  case 0x02: // Seek to time
    mcd.cdd.seekToTime(BCD::decode(inputRegs[0x09]), BCD::decode(inputRegs[0x0A]), BCD::decode(inputRegs[0x0B]), targetPauseState);
    seekPerformedSinceLastFlagsRead = true;
    break;
  case 0x03: // Seek to last and stop
  case 0x07: // Unknown, but so far seems same as seek to last and stop.
    mcd.cdd.seekToTrack(mcd.cdd.getTrackCount(), true);
    seekPerformedSinceLastFlagsRead = true;
    currentPauseState = true;
    break;
  case 0x05: // Seek to track
    mcd.cdd.seekToTrack(BCD::decode(inputRegs[0x07]), targetPauseState);
    seekPerformedSinceLastFlagsRead = true;
    break;
  }
}

auto MCD::LD::power(bool reset) -> void {
  // Note we currently rely on our reset call happening after the cdd reset to get this to work
  mcd.cdd.hostClockEnable = true;
}
