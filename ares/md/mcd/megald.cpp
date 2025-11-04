// Pioneer PD6103A
auto MCD::LD::load(string location) -> void {
  video.outputFramebuffer.resize(video.FrameBufferWidth * (video.FrameBufferHeight + 1));

  //Load MMI file only if it has changed or is the first load
  //FIXME: calling mmi.close() during emulation crashes; we don't support changing .mmi file at present in the UI anyway so this doesn't matter (yet)
  if(mmi.location() != location) {
    if(mmi.location()) mmi.close();
    mmi.open(location);
  }

  //attempt to locate the requested media in the mmi archive
  string mediaName = mcd.disc->attribute("media");
  auto& mediaVec = mmi.media();
  //FIXME(stdc++): revisit this constructor with nicer code
  auto it = std::ranges::find_if(mediaVec, [mediaName](const auto& m){ return m.name == mediaName; });
  u32 mediaIndex = it == mediaVec.end() ? 0u : static_cast<u32>(it - mediaVec.begin());

  // Extract the stream information for analog video and audio
  string analogAudioFileName;
  string analogVideoFileName;
  video.isCLV = false;
  video.sideNo = 0;
  video.leadInFrameCount = 0;
  video.activeVideoFrameCount = 0;
  video.leadOutFrameCount = 0;
  if (mmi.media().size() > 0) {
    for(const auto& stream : mmi.media()[mediaIndex].streams) {
      if(stream.type == "Redbook") {
        mcd.fd = mcd.pak->read(stream.file);
        video.hasDigitalAudio = true;
      }
      if(stream.type == "RawAudio") analogAudioFileName = stream.file;
      if(stream.type == "RawVideo") {
        analogVideoFileName = stream.file;
        video.leadInFrameCount = stream.framesInLeadInRegion;
        video.activeVideoFrameCount = stream.framesInActiveRegion;
        video.leadOutFrameCount = stream.framesInLeadOutRegion;
        video.isCLV = mmi.media()[mediaIndex].format.endsWith("CLV");
        video.sideNo = mmi.media()[mediaIndex].sideNo;
      }
    }
  } else {
    mcd.fd = mcd.pak->read("cd.rom");
    video.hasDigitalAudio = true;
  }

  // Calculate the number of leading audio samples before the first frame
  analogAudioLeadingAudioSamples = (n32)((double)video.leadInFrameCount * (44100.0 / videoFramesPerSecond));

  // Retrieve the analog audio file
  auto analogAudioFile = mmi.archive().findFile(analogAudioFileName);
  if (analogAudioFile) {
    if (mmi.archive().isDataUncompressed(*analogAudioFile)) {
      analogAudioRawDataView = mmi.archive().dataViewIfUncompressed(*analogAudioFile);
    } else {
      auto tmp = mmi.archive().extract(*analogAudioFile);
      analogAudioDataBuffer.resize(tmp.size());
      if(!tmp.empty()) memcpy(analogAudioDataBuffer.data(), tmp.data(), tmp.size());
      analogAudioRawDataView = std::span<const u8>(analogAudioDataBuffer.data(), analogAudioDataBuffer.size());
    }
  }

  // Load and index our analog video data
  auto analogVideoFile = mmi.archive().findFile(analogVideoFileName);
  if (analogVideoFile) {
  // Retrieve the analog video file
    if (!mmi.archive().isDataUncompressed(*analogVideoFile)) {
      return;
    }
    std::span<const u8> analogVideoFileView = mmi.archive().dataViewIfUncompressed(*analogVideoFile);

    // Read the QON/QOI2 header information for the analog video
    if (!qon_decode_header(analogVideoFileView.data(), analogVideoFileView.size(), &video.videoFileHeader)) {
      return;
    }
    if ((video.videoFileHeader.flags & QON_FLAGS_USES_INTERFRAME_COMPRESSION) != 0) {
      return;
    }
    video.videoFrameHeader.width = video.videoFileHeader.width;
    video.videoFrameHeader.height = video.videoFileHeader.height;
    video.videoFrameHeader.channels = video.videoFileHeader.channels;
    video.videoFrameHeader.colorspace = video.videoFileHeader.colorspace;

    // Index the analog video frames
    const unsigned char* frameIndexBase = (const unsigned char*)analogVideoFileView.data() + QON_BARE_HEADER_SIZE;
    size_t frameIndexSize = video.videoFileHeader.frame_count * QON_INDEX_SIZE_PER_ENTRY;
    video.leadInFrames.resize(video.leadInFrameCount);
    video.leadOutFrames.resize(video.leadOutFrameCount);
    video.activeVideoFrames.resize(video.activeVideoFrameCount);
    for (size_t frameIndex = 0; frameIndex < video.videoFileHeader.frame_count; ++frameIndex) {
      // Retrieve the index entry for the next frame
      size_t frameOffsetAfterIndex;
      unsigned short frameFlags;
      qon_decode_index_entry(frameIndexBase, frameIndex, &frameOffsetAfterIndex, &frameFlags);

      // Add the frame to our own frame index in memory
      const unsigned char* frameBaseAddress = frameIndexBase + frameIndexSize + frameOffsetAfterIndex;
      if (frameIndex < video.leadInFrameCount) {
        video.leadInFrames[frameIndex] = frameBaseAddress;
      } else if (frameIndex < (video.leadInFrameCount + video.activeVideoFrameCount)) {
        video.activeVideoFrames[frameIndex - video.leadInFrameCount] = frameBaseAddress;
      } else if (frameIndex < (video.leadInFrameCount + video.activeVideoFrameCount + video.leadOutFrameCount)) {
        video.leadOutFrames[frameIndex - video.leadInFrameCount - video.activeVideoFrameCount] = frameBaseAddress;
      } else {
        debug(unusual, "[MCD::LD::load] Trailing frames in analog video file: ", analogVideoFileName);
      }
    }

    // Build position information to assist with fast retrieval of biphase coded data in the VBI regions. As per
    // IEC 60857:1986, the first bit is the "key" bit, and will always be logical level 1. We need to locate the key
    // bit in VBI lines, however since the exact offset of the coded data is allowed to, and does, vary in its exact
    // alignment vs the horizontal blanking region, we need to search for it. We can, however, be sure that it occurs
    // after horizontal blanking and color burst. Giving ourself a healthy margin, we start scanning from roughly 15%
    // of the way into the horizontal line, and continue to scan up to 25%. If we can't find it in that range, it's not
    // in there.
    video.vbiDataSearchStartPos = (size_t)((float)video.videoFrameHeader.width * 0.15f);
    video.vbiDataSearchEndPos = (size_t)((float)video.videoFrameHeader.width * 0.25f);

    // Build position offsets for each subsequent bit sample position for the biphase data after the key bit. As per
    // IEC 60857:1986 Figure 11, positive transitions in the centre of a bit cell represent a 1, negative transitions
    // represent a 0. This encoding scheme expects at least one transition each cell length therefore, and we need to
    // sample twice for each bit on either side of the cell centre position.
    double linesPerSecond = videoFramesPerSecond * (double)video.FrameBufferHeight;
    double microsecondsPerLine = (1.0 * 1000.0 * 1000.0) / linesPerSecond;
    double cellLengthInMicroseconds = 2.0;
    video.vbiDataBitCellLengthInPixels = (double)video.videoFrameHeader.width * (cellLengthInMicroseconds / microsecondsPerLine);
    double vbiDataBitQuarterCellLengthInPixels = video.vbiDataBitCellLengthInPixels / 4.0;
    size_t biphaseCodeTotalBitcount = 6 * 4;
    video.vbiDataBitSampleOffsets.resize(biphaseCodeTotalBitcount * 2);
    for (size_t i = 0; i < video.vbiDataBitSampleOffsets.size(); i += 2) {
      video.vbiDataBitSampleOffsets[i+0] = (size_t)((video.vbiDataBitCellLengthInPixels * (i / 2)) + vbiDataBitQuarterCellLengthInPixels);
      video.vbiDataBitSampleOffsets[i+1] = (size_t)((video.vbiDataBitCellLengthInPixels * (i / 2)) + (vbiDataBitQuarterCellLengthInPixels * 3.0));
    }
  } else {
    video.leadInFrames.clear();
    video.leadOutFrames.clear();
    video.activeVideoFrames.clear();
  }

  // Reset the mechanical drive state to closed
  currentDriveState = 0x02;
  targetDriveState = currentDriveState;

  // Spawn a background thread to prefetch video frames
  videoFramePrefetchTarget = nullptr;
  videoFramePrefetchPending.clear();
  videoFramePrefetchComplete.clear();
  videoFramePrefetchThreadStarted.clear();
  videoFramePrefetchThreadShutdownRequested.clear();
  videoFramePrefetchThreadShutdownComplete.clear();
  std::thread workerThread(std::bind(std::mem_fn(&MCD::LD::videoFramePrefetchThread), this));
  workerThread.detach();
  videoFramePrefetchThreadStarted.wait(false);
}

auto MCD::LD::unload() -> void {
  // Request the prefetch background thread to terminate, and wait for it to complete.
  if (videoFramePrefetchThreadStarted.test()) {
    videoFramePrefetchThreadShutdownRequested.test_and_set();
    videoFramePrefetchPending.test_and_set();
    videoFramePrefetchPending.notify_all();
    videoFramePrefetchThreadShutdownComplete.wait(false);
  }

  // Close the mmi file
  mmi.close();
}

auto MCD::LD::notifyDiscEjected() -> void {
  // Reset the mechanical drive state to opened
  currentDriveState = 0x01;
  targetDriveState = currentDriveState;
}

auto MCD::LD::read(n24 address) -> n8 {
  bool isOutput = (address & 0x80);
  u8 regNum = (address & 0x3f) >> 1;
//  ares::_debug.reset();
  //debug(unverified, "[MCD::readLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L));

  // Retrieve the current value of the target register
  n8 data = 0;
  if (!isOutput) {
    // Reading back the input registers always returns the last written value to that register
    data = areInputRegsFrozen ? inputFrozenRegs[regNum] : inputRegs[regNum];
  } else if (outputRegsWrittenCooldownTimer[regNum] > 0) {
    // If we're still in cooldown mode from an output register write, return the previously written state and decrement
    // the cooldown timer. This is an imperfect approximation of the observed behaviour that output registers retain
    // their previously written values for a short time, before actively asserted lines revert to their previous values.
    // The NEC PAC bios relies on this to boot.
    --outputRegsWrittenCooldownTimer[regNum];
    data = outputRegsWrittenData[regNum];
  } else if (areOutputRegsFrozen) {
    // Reading the output register block when frozen doesn't require us to update the values
    data = outputFrozenRegs[regNum];
  } else {
    // Our output register block isn't frozen, so we generate the data value based on the current system state.
    data = getOutputRegisterValue(regNum);
  }

  static bool includeReg0InputReadBack = false;
  if (!isOutput && ((regNum != 0x00) || includeReg0InputReadBack)) {
    //debug(unusual, "[MCD::readLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
  } else {
    //debug(unverified, "[MCD::readLD] reg=0x", hex(regNum, 2L), " = ", hex(data, 2L));
  }
  return data;
}

auto MCD::LD::write(n24 address, n8 data) -> void {
  static bool includeReg0DebugOutput = false;
  bool isOutput = (address & 0x80);
  u8 regNum = (address & 0x3f) >> 1;
  //ares::_debug.reset();
  //debug(unverified, "[MCD::writeLD] reg=0x", hex(regNum, 2L), " = ", hex(data, 2L));
  if ((regNum != 0x00) || includeReg0DebugOutput) {
    //debug(unverified, "[MCD::writeLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
    if (isOutput) {
      debug(unusual, "[MCD::writeLD] address=0x", hex(address, 8L), " output=", isOutput, " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
    }
  }

  if (!isOutput) {
    // Register 0x00 is special, as it contains the "frozen" input and output state selection bits. We
    // handle them here first before performing further register write handling.
    if (regNum == 0x00) {
      // Update this register in the frozen block first
      if (areInputRegsFrozen) {
        inputFrozenRegs[regNum] = data;
      }

      // Update the output reg frozen state
      auto previousOutputRegsFrozenState = areOutputRegsFrozen;
      areOutputRegsFrozen = data.bit(6);
      if (areOutputRegsFrozen && !previousOutputRegsFrozenState) {
        for (int i = 0; i < outputRegisterCount; ++i) {
          outputFrozenRegs[i] = outputRegs[i] = getOutputRegisterValue(i);
        }
      }

      // If we're changing the input register frozen state, perform the necessary updates now.
      auto previousInputRegsFrozenState = areInputRegsFrozen;
      areInputRegsFrozen = !data.bit(7);
      if (!areInputRegsFrozen && previousInputRegsFrozenState) {
        // If the input register block is being unfrozen, apply all the input registers now with their current values
        // from the frozen block. Note that we push all the seek registers through together first in a block. Since
        // these can trigger immediately on write, we need to make sure they're all set together.
        //##FIX# Ok, the way this all interacts is like this:
        //-Mechanical drive state is updated first
        //-If transitioning mechanical drive state from 3 to 5, if seek isn't enabled, the latched seek state will be reset
        // to the start of the disc and playing from there.
        //-If seek is enabled, the NEW seek register state is applied and seeked to. This means you can spin up a disc and
        // seek to a target location in one step.
        //-If mechanical drive state doesn't change, the new seek register state is latched, but seeking is only performed
        // if reg 0x00 bit 0 also changes from its previously latched state.
        //-Setting seek mode 3 to latch a new stop point always works. It ignores both seek enabled checks and reg 0x00 bit 0
        // transition checks.

        // Update the seek register state. If the mechanical drive state is correct to action this, a new seek target will
        // be latched here now. If not, and the mechanical drive state changes to make this seek action possible, it will
        // be handled below when we trigger the mechanical drive state update. Note that we need to take a backup of the
        // previous register state for the benefit of stop point latching, which doesn't trigger with seek target latching,
        // and relies on the previous register state.
        n8 seekRegsPreviousState[6];
        seekRegsPreviousState[0] = inputRegs[0x06];
        seekRegsPreviousState[1] = inputRegs[0x07];
        seekRegsPreviousState[2] = inputRegs[0x08];
        seekRegsPreviousState[3] = inputRegs[0x09];
        seekRegsPreviousState[4] = inputRegs[0x0A];
        seekRegsPreviousState[5] = inputRegs[0x0B];
        inputRegs[0x06] = inputFrozenRegs[0x06];
        inputRegs[0x07] = inputFrozenRegs[0x07];
        inputRegs[0x08] = inputFrozenRegs[0x08];
        inputRegs[0x09] = inputFrozenRegs[0x09];
        inputRegs[0x0A] = inputFrozenRegs[0x0A];
        inputRegs[0x0B] = inputFrozenRegs[0x0B];
        if (liveSeekRegistersContainsLatchableTarget()) {
          latchSeekTargetFromCurrentState();
        }
        // Update the mechanical drive state. This needs to be done before seeking is evaluated.
        processInputRegisterWrite(0x02, inputFrozenRegs[0x02], inputRegs[0x02], true);
        // Update the playback mode. This needs to be done before input reg 0x00 bit 0 changes are evaluated.
        processInputRegisterWrite(0x03, inputFrozenRegs[0x03], inputRegs[0x03], true);
        // Update remaining registers. Register 0x00 is quite important, as it may trigger a seek operation here itself,
        // and it relies on the mechanical drive state and seek registers being handled above before it can work
        // correctly.
        for (int i = 0; i < inputRegisterCount; ++i) {
          n8 previousData = ((i >= 0x06) && (i <= 0x0B) ? seekRegsPreviousState[i - 0x06] : inputRegs[i]);
          processInputRegisterWrite(i, inputFrozenRegs[i], previousData, true);
        }
      } else if (areInputRegsFrozen && !previousInputRegsFrozenState) {
        // If the input register block is being frozen, copy the current register state into the frozen register state.
        for (int i = 0; i < inputRegisterCount; ++i) {
          inputFrozenRegs[i] = inputRegs[i];
        }
      }
    }

    // Handle the register write
    if (areInputRegsFrozen) {
      // If the input registers are currently frozen, update the frozen state of the target register.
      inputFrozenRegs[regNum] = data;

      // Only trigger changes on a few limited registers while frozen. These are the analog mixing registers 0x19,
      // 0x1A, and 0x1B for video, and 0x1E and 0x1F for audio. Regs 0x1C and 0x1D currently have no known function.
      if (regNum >= 0x19) {
        processInputRegisterWrite(regNum, data, inputRegs[regNum], false);
      }
    } else {
      // Trigger any required behaviour in response to this input register write
      processInputRegisterWrite(regNum, data, inputRegs[regNum], false);
    }
  } else if (areOutputRegsFrozen) {
    // Perform writes to the output register block while the register output block is frozen. In this state, most
    // locations can be written to freely.
    if (regNum == 0x00) {
      // The upper bit of register 0x00 still updates when output registers are frozen
      outputFrozenRegs[regNum] = (data & 0x7F) | (outputRegs[regNum] & 0x80);
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
    outputRegsWrittenData[regNum] = data;
    outputRegsWrittenCooldownTimer[regNum] = 5;
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
    // 0xFDFE85|*U7|*U6|*U5|*U4| ? | ? | ? | ? |
    //         ---------------------------------
    // *U7: Set to 1 when an LD is in the tray
    // *U5: Set to 1 when a CD is in the tray
    // *U4: Set to 1 when the the drive tray is closed and empty
    // ##NEW## 2025
    // -This is clearly a more complex full media type value
    // 0xA5 when GGV1069 is spun up (mech request 0x04), 0x80 when it is loaded but not spinning. 0xA5 persisted after stop (mech request 0x03, then 0x02)
    // 0xC5 when Triad Stone is spun up (mech request 0x04), 0x80 when it is loaded but not spinning. 0xC5 persisted after stop (mech request 0x03, then 0x02)
    //   Actually seen to start as 0xC0, then go to 0xC4, then quickly transition to 0xC5.
    // 0xC6 when a CLV full size Laserdisc is spun up
    // 0x28 when a CD-V disc (PAL) is spun up
    // 0x20 when an audio CD is spun up

    //##FIX## Do this properly
    data = 0;
    if (currentDriveState < 0x02) {
      data = 0x00; // Tray open
    } else if (!mcd.cdd.isDiscLoaded()) {
      data.bit(4) = 1; // Drive empty
    } else if (mcd.cdd.isDiscLaserdisc()) {
      data = (currentDriveState >= 0x04) ? (mcd.cdd.isLaserdiscClv() ? 0xC6 : 0xC5) : 0x80; // Assume 30cm laserdisc
    } else {
      data = 0x20; // CD
    }
    break;
  case 0x03:
    //         --------------------------------- (Buffered in $5913)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x03|-------------------------------|
    // 0xFDFE87| *U67  | ? |*U4|*U3| ? | ? | ? |
    //         ---------------------------------
    // ##NEW## 2025
    // *U3: Set during spinup of 30/20cm CAV/CLV disc a bit after U6, and a CD without U6. Cleared when drive tray opened.
    // *U4: Set during spinup of 20cm disc a bit after U6, very slightly before U3 but almost same time. Cleared when drive tray opened.
    // *U67: Set during spinup of 30/20cm CAV/CLV disc. Cleared when drive tray opened.
    //       -Set to 01 for side A(1) of a Laserdisc
    //       -Set to 10 for side B(2) of a Laserdisc
    //       -When tested on a video Laserdisc title which had two double-sided discs in the set, for disc 2 side A,
    //        which is marked as side "3" on the label, it came up as side 1 in this register, with side 4 being side 2.
    //       -When tested on Myst B2 prototype, which has two identical "side A" sides pressed together, both sides
    //        reported as side A (01) as expected.
    // ##OLD## Pre 2025
    // *U4: Unknown. DRVINIT tests this, and only reads TOC info from the loaded disk if it is set to 0. It's possible this
    //      is an error in the bios routines however, and they meant to test U4 in the above output register 0x02. This would
    //      make sense.
    if (currentDriveState < 0x02) {
      // Clear all flags to 0 when tray open
      data = 0;
    } else if (currentDriveState < 0x04) {
      // Retain previous state when a disc is spun down, don't update anything.
    } else { // currentDriveState >= 0x04
      // Update disc present flag
      data.bit(3) = mcd.cdd.isDiscLoaded();
      // Update disc side flags
      if (mcd.cdd.isDiscLoaded() && mcd.cdd.isDiscLaserdisc())
      {
        data.bit(6, 7) = (video.sideNo > 1 ? 0b10 : 0b01);
      } else {
        data.bit(6, 7) = 0;
      }
    }
    break;
  case 0x04:
    //         --------------------------------- (Buffered in $5914)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x04|-------------------------------|
    // 0xFDFE89|*U7| ? |*U5|*U4|     *U30      |
    //         ---------------------------------
    // ##NEW## 2025
    // *U5: Always seen set to 1, unless an LD is currently spun up. Stays set to 1 when a CD is spun up.
    // *U4: When a laserdisc is being spun up (drive state 0x04) but that process is not complete, this briefly gets set
    //      to 1 for maybe a quarter of a second, at the same time as U5 is cleared. It is set to 0 afterwards and stays
    //      that way. This is while the spinup operation is still in progress, before the U7 drive busy flag in reg 0x06
    //      is cleared.
    // *U7: Set to 1 if bit 4 of input register 0x0D is set, unless a CD is in the drive, whether it is spun up or not.
    // *U30: Seen set to 0xE during laserdisc playback when the redbook digital audio is playing a data track, 0xC when
    //       it is playing an audio track. This was observed on Space Berserker. Also seen set to 0x2 when playing a CD
    //       data or audio track.
    // ##OLD## Before 2025
    // *U7: Set to 1 if bit 4 of input register 0x0D is set
    data = 0;
    data.bit(5) = !mcd.cdd.isDiscLoaded() || !mcd.cdd.isDiscLaserdisc() || (currentDriveState < 0x04);
    if (!mcd.cdd.isDiscLoaded() || mcd.cdd.isDiscLaserdisc() || (currentDriveState < 0x02)) {
      data.bit(7) = inputRegs[0x0D].bit(4);
    }
    if (mcd.cdd.isDiscLoaded() && mcd.cdd.isDiscLaserdisc() && (currentDriveState >= 0x04)) {
      data.bit(0, 3) = (mcd.cdd.isTrackAudio(mcd.cdd.getCurrentTrack()) ? 0x0C : 0x0E);
    } else if (mcd.cdd.isDiscLoaded() && !mcd.cdd.isDiscLaserdisc() && (currentDriveState >= 0x04)) {
      data.bit(0, 3) = 0x2;
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
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x06|-------------------------------|
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

    // If the drive state has changed since the last time this register was read, we pretend the drive is still
    // transitioning to the new state for a number of reads, then we swap it over. In the real hardware, most of these
    // transitions take many seconds. We put this small wait state in here not to try and obtain accurate timing, but
    // as a defensive measure in case any code out there malfunctions if the state changes instantly without going
    // into a busy state at least once. It's possible that this could be interpreted as an error or failure to apply
    // the change. This has now been confirmed to be true in at least one BIOS routine, making this code necessary for
    // games to boot.
    //##FIX## Tie this to a cycle counter to make it deterministic. This is a hack currently.
    if ((previousData.bit(0, 3) != currentDriveState) || (driveStateChangeDelayCounter > 0)) {
      if ((previousData.bit(7) != 1) || (driveStateChangeDelayCounter == 0)) {
        static const unsigned int ChangeDelayCount = 10;
        driveStateChangeDelayCounter = ChangeDelayCount;
      } else if (driveStateChangeDelayCounter == 1) {
        data.bit(0, 3) = currentDriveState;
        driveStateChangeDelayCounter = 0;
      } else {
        --driveStateChangeDelayCounter;
      }
    }

    // Update the drive busy and seek in progress flags. Note that some games (IE, Pyramid Patrol) perform unpaused seek
    // operations and spin in a busy loop waiting first for the seek to complete, then for the current frame number to
    // exactly match the target frame, so we need seek busy state to be accurate here.
    if ((data.bit(0, 3) == 5) && (mcd.cdd.io.status == CDD::Status::Seeking)) {
      data.bit(7) = 1;
      data.bit(5) = 1;
    } else {
      data.bit(7) = (driveStateChangeDelayCounter > 0);
      data.bit(5) = 0;
    }

    // Update the pause state
    data.bit(4) = currentPauseState || targetPauseState;
    break;
  case 0x07:
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
    data.bit(4, 7) = currentPlaybackMode;
    data.bit(3) = currentPlaybackDirection;
    data.bit(0, 2) = currentPlaybackSpeed;

    // Handle special cases
    if (reachedStopPoint) {
      // At a stop point, the hardware reports we're in single-frame step mode at speed 0. This is not a persistent change though,
      // as just toggling seek enable off and on again releases the stop point, with the playback mode and speed settings reverting
      // to the previous latched state.
      data = 0x20;
    } else if ((currentPlaybackMode == 0x02) && (currentPlaybackSpeed == 0x00)) {
      // Under single-frame frame step mode, the current playback speed is only reported as 0x01 until the frame step has occurred,
      // after which it becomes 0x00. That's handled on a frame update, with the actual playback speed being modified. Under
      // playback speed 0x00 however, either when set manually or when transitioned to from a frame step with speed 0x01, the
      // direction bit is always reported as 0 on the output register. We emulate that here.
      data.bit(3) = false;
    } else if ((currentPlaybackMode == 0x01) && (currentPlaybackSpeed == 0x01) && (video.frameSkipCounter == -1)) {
      // Under single-frame frame skip mode, the direction bit is inverted from its current value once the frame skip has occurred.
      // This means if it was originally clear, it will be set, while if it is originally set, it will be cleared. That is handled
      // elsewhere. When re-triggering by toggling bit 0 of input reg 0x00 however, the direction bit gets toggled again when it
      // gets re-latched, so it does not indicate whether the frame as been latched or not from its originally set value. Also note
      // that the direction bit is seen to rapdily flicker between 0 and 1 when a seek is being performed while this mode is set,
      // most likely as it continuously latches a new base frame during the seek operation, which would happen with our current
      // implementation, if we reported updates to seek position during the seek latency countdown. At any rate, there's nothing to
      // do here, since this case is all handled elsewhere.
    }
    break;
  case 0x08:
    //         --------------------------------- (Buffered in $5918)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x08|-------------------------------|
    // 0xFDFE91|*U7|*U6| ? |*U4| ? | ? | ? |*U0|
    //         ---------------------------------
    //##NEW## 2025
    // *U7: Set when an LD starts playing, IE not set at drive state 0x04. Cleared when the disc is spun down. Not set for CD.
    // *U6: For a CD, set when a CD is detected when the drive is closed, IE at drive state 0x02. Retained until the tray is
    //      opened. For an LD, set when the disc is loaded and spinning. Cleared when the disc is spun down.
    // *U4: Observed to be set (along with U7 and U6) at the very end of side 2 of JB Harold BCB when the track 2 BCD timecode
    //      rolled over to 59:59:28, and locked there, while the redbook track counter kept on counting. Most likely this gets
    //      set when we hit the lead-out frames.
    // *U0: Set when a CD or LD is loaded and spinning. Retained until drive tray is opened.
    //##OLD## 2025
    // *U7: Observed to be set when testing bad seeking operations
    // *U6: Observed to be set when a CD was detected when the drive tray was closed. The disk has not been spun up at this point.
    // *U4: Observed to be set when testing bad seeking operations
    // *U0: Unknown. DRVINIT tests this.
    //##FIX## Retain bit U0 until the drive tray is opened
    data.bit(7) = mcd.cdd.isDiscLoaded() && mcd.cdd.isDiscLaserdisc() && (currentDriveState >= 5);
    data.bit(6) = mcd.cdd.isDiscLoaded() && ((!mcd.cdd.isDiscLaserdisc() && (currentDriveState >= 2)) || (mcd.cdd.isDiscLaserdisc() && (currentDriveState >= 4)));
    data.bit(0) = mcd.cdd.isDiscLoaded() && (currentDriveState >= 4);
    break;
  case 0x09:
    //         --------------------------------- (Buffered in $5919)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x09|-------------------------------|
    // 0xFDFE93| ? | ? | ? |*U4| ? |*U2|*U1|*U0|
    //         ---------------------------------
    // ##NEW## 2025
    // *U4: Expanding on the below, this is very much a "last operation failed" type flag. When either register 0x02 or 0x03 are set,
    // they first clear this flag, then set it if there's an error. This means if changing reg 0x03 has flagged an error for example,
    // then you do a valid write to 0x02 that doesn't trigger an error, the flag will be cleared, despite 0x03 being invalid. It's
    // not a live effective error state.
    // *U0: This is also seen to be set when a CD is loaded and trying to seek to track 0x00. Note that when an LD is loaded, seeking
    //      to track 0x00 works and is treated the same as seeking to track 0x01.
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
    data.bit(1) = operationErrorFlag2;
    data.bit(0) = operationErrorFlag3;
    break;
  case 0x0A:
    //         --------------------------------- (Buffered in $591A)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0A|-------------------------------|
    // 0xFDFE95| -   -   -   -   - |    SM     |
    //         ---------------------------------
    // -Returns the current state of input register 0x06. See notes on that register for more info.
    // -Note that due to direct byte value comparisons made in the subcpu bios with an infinite delay loop, we know this register must
    //  be a direct literal match for input register 0x06 on all bits.
    data = inputRegs[0x06];
    break;
  case 0x0B:
    //         --------------------------------- (Buffered in $591B)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0B|-------------------------------|
    // 0xFDFE97|           Track No            |
    //         ---------------------------------
    // Returns the current state of input register 0x07. See notes on that register for more info.
    data = inputRegs[0x07];
    break;
  case 0x0C:
    //         --------------------------------- (Buffered in $591C)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0C|-------------------------------|
    // 0xFDFE99|           SectorNoU           |
    //         ---------------------------------
    // Returns the current state of input register 0x08. See notes on that register for more info.
    data = inputRegs[0x08];
    break;
  case 0x0D:
    //         --------------------------------- (Buffered in $591D)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0D|-------------------------------|
    // 0xFDFE9B|        Minutes/SectorNoM      |
    //         ---------------------------------
    // Returns the current state of input register 0x09. See notes on that register for more info.
    data = inputRegs[0x09];
    break;
  case 0x0E:
    //         --------------------------------- (Buffered in $591E)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0E|-------------------------------|
    // 0xFDFE9D|        Seconds/SectorNoL      |
    //         ---------------------------------
    // Returns the current state of input register 0x0A. See notes on that register for more info.
    data = inputRegs[0x0A];
    break;
  case 0x0F:
    //         --------------------------------- (Buffered in $591F)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0F|-------------------------------|
    // 0xFDFE9F|            Frames             |
    //         ---------------------------------
    // Returns the current state of input register 0x0B. See notes on that register for more info.
    data = inputRegs[0x0B];
    break;
  case 0x10:
    //         --------------------------------- (Buffered in $5920)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x10|-------------------------------|
    // 0xFDFEA1|      Track Info Selection     |
    //         ---------------------------------
    // Returns the current state of input register 0x05. See notes on that register for more info.
    data = inputRegs[0x05];
    break;
  case 0x11: {
    //         --------------------------------- (Buffered in $5921)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x11|-------------------------------|
    // 0xFDFEA3|RedbookControl |   DataValid   |
    //         ---------------------------------
    //##FIX## We know the "DataValid" info here is wrong. Pyramid Patrol track no 0x01 lookup returns 0x44 for this register.
    // RedbookControl:
    //    CDs: If input register 0x05 is set to 0x00 or 0xFF, reports the 4-bit track bitflags as per CONTROL in the Q
    //         channel subcode data for current location, or TOC data depending on input reg 0x05. For other values of
    //         input register 0x05, see the notes on that register.
    //    LDs: As per CDs, if the LD has a digital (redbook) track in the current playback area. If there is no redbook
    //         data encoded (IE, no digital audio track), this is set to 0x00.
    // DataValid: If the data in this register and the following output regs 0x12-0x14 are reporting valid control and
    //            timecode information, this is set to 0x1, otherwise it is set to 0x0. This applies for input reg 0x05
    //            values 0xFF, and 0x00-0x99, where this is supposed to give control and timecode information, otherwise
    //            this whole register is set according to the notes in input reg 0x05.
    // 
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
      mcd.cdd.getTrackTocData(trackToQuery, flags, minute, second, frame);
      data.bit(4, 7) = flags;
      data.bit(0, 3) = 0x01;
    }
    break;}
  case 0x12:
    //         --------------------------------- (Buffered in $5922)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x12|-------------------------------|
    // 0xFDFEA5|        RedbookMinutesA        |
    //         ---------------------------------
    // RedbookMinutesA:
    //    CDs: If input register 0x05 is set to 0x00 or 0xFF, reports the absolute minutes as per AMIN in the Q channel
    //         subcode data for current location, or TOC data depending on input reg 0x05. For other values of input
    //         register 0x05, see the notes on that register.
    //    LDs: As per CDs, if the LD has a digital (redbook) track in the current playback area. In this case, this data
    //         is fetched from the redbook track subcode data, and reports in CD time format with 75 frames per second.
    //         If there is no redbook data encoded (IE, no digital audio track), this is set to 0x00.
    if ((selectedTrackInfo == 0xA0) || (selectedTrackInfo == 0xB0)) {
      data = BCD::encode(mcd.cdd.session.firstTrack);
    } else if ((selectedTrackInfo == 0xA1) || (selectedTrackInfo == 0xB1)) {
      mcd.cdd.getLeadOutTimecode(minute, second, frame);
      data = BCD::encode(minute);
    } else if (((selectedTrackInfo == 0) || (selectedTrackInfo == 0xFF)) && (mcd.cdd.getTrackCount() > 0)) {
      mcd.cdd.getCurrentTimecode(minute, second, frame);
      data = BCD::encode(minute);
    } else if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0xFF;
    } else {
      mcd.cdd.getTrackTocData(selectedTrackInfo, flags, minute, second, frame);
      data = BCD::encode(minute);
    }
    break;
  case 0x13:
    //         --------------------------------- (Buffered in $5923)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x13|-------------------------------|
    // 0xFDFEA7|        RedbookSecondsA        |
    //         ---------------------------------
    // RedbookSecondsA:
    //    CDs: If input register 0x05 is set to 0x00 or 0xFF, reports the absolute seconds as per ASEC in the Q channel
    //         subcode data for current location, or TOC data depending on input reg 0x05. For other values of input
    //         register 0x05, see the notes on that register.
    //    LDs: As per CDs, if the LD has a digital (redbook) track in the current playback area. In this case, this data
    //         is fetched from the redbook track subcode data, and reports in CD time format with 75 frames per second.
    //         If there is no redbook data encoded (IE, no digital audio track), this is set to 0x00.
    if ((selectedTrackInfo == 0xA0) || (selectedTrackInfo == 0xB0)) {
      data = BCD::encode(mcd.cdd.getTrackCount());
    } else if ((selectedTrackInfo == 0xA1) || (selectedTrackInfo == 0xB1)) {
      mcd.cdd.getLeadOutTimecode(minute, second, frame);
      data = BCD::encode(second);
    } else if (((selectedTrackInfo == 0) || (selectedTrackInfo == 0xFF)) && (mcd.cdd.getTrackCount() > 0)) {
      mcd.cdd.getCurrentTimecode(minute, second, frame);
      data = BCD::encode(second);
    } else if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0xFF;
    } else {
      mcd.cdd.getTrackTocData(selectedTrackInfo, flags, minute, second, frame);
      data = BCD::encode(second);
    }
    break;
  case 0x14:
    //         --------------------------------- (Buffered in $5924)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x14|-------------------------------|
    // 0xFDFEA9|        RedbookFramesA         |
    //         ---------------------------------
    // RedbookFramesA:
    //    CDs: If input register 0x05 is set to 0x00 or 0xFF, reports the absolute frames as per AFRAME in the Q channel
    //         subcode data for current location, or TOC data depending on input reg 0x05. For other values of input
    //         register 0x05, see the notes on that register.
    //    LDs: As per CDs, if the LD has a digital (redbook) track in the current playback area. In this case, this data
    //         is fetched from the redbook track subcode data, and reports in CD time format with 75 frames per second.
    //         If there is no redbook data encoded (IE, no digital audio track), this is set to 0x00.
    if ((selectedTrackInfo == 0xA0) || (selectedTrackInfo == 0xB0)) {
      data = 0x00;
    } else if ((selectedTrackInfo == 0xA1) || (selectedTrackInfo == 0xB1)) {
      mcd.cdd.getLeadOutTimecode(minute, second, frame);
      data = BCD::encode(frame);
    } else if (((selectedTrackInfo == 0) || (selectedTrackInfo == 0xFF)) && (mcd.cdd.getTrackCount() > 0)) {
      mcd.cdd.getCurrentTimecode(minute, second, frame);
      data = BCD::encode(frame);
    } else if (selectedTrackInfo > mcd.cdd.getTrackCount()) {
      data = 0xFF;
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
    // -This register is set to 0x01 when the disc is spun down (drive state 0x03 or 0x02), and is set to 0x00 when
    //  the drive tray is ejected (drive state 0x01). If the drive tray is empty it remains at 0x00 in drive state
    //  0x02-0x03.
    // -The following regs 0x16-0x19 all get set to 0x00 when the disc is spun down (drive state 0x03).
    if (mcd.cdd.isDiscLoaded() && (currentDriveState >= 5)) {
      data = BCD::encode(mcd.cdd.getCurrentTrack());
    } else if (mcd.cdd.isDiscLoaded() && (currentDriveState >= 2)) {
      data = 0x01;
    } else {
      data = 0x00;
    }
    break;
  case 0x16:
    //         --------------------------------- (Buffered in $5926)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x16|-------------------------------|
    // 0xFDFEAD|     Current Hour/FrameNoU     |
    //         ---------------------------------
    // Current Hour/FrameNoU:
    //    -CAV LDs: Upper digit of current frame number in BCD format, IE X in "X????".
    //    -CLV LDs: Hours counter of current time code in BCD format, IE X in "X??????".
    //    -CDs: The current subdivision number within the track, IE, "X" from the Q channel subcode data.
    data = 0;
    if (mcd.cdd.isDiscLoaded() && (currentDriveState >= 5)) {
      if (mcd.cdd.isDiscLaserdisc()) {
        auto frameNumber = zeroBasedFrameIndexFromLba(mcd.cdd.getCurrentSector()) + 1;
        if (mcd.cdd.isLaserdiscClv()) {
          data = BCD::encode((frameNumber / (60 * 60 * 30)) % 60);
        } else {
          data = BCD::encode((frameNumber / (100 * 100)) % 100);
        }
      } else {
        //##FIX## Pull the subdivision number out of the subcode buffers
        data = 0x01;
      }
    }
    break;
  case 0x17:
    //         --------------------------------- (Buffered in $5927)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x17|-------------------------------|
    // 0xFDFEAF|    Current Minute/FrameNoM    |
    //         ---------------------------------
    // Current Minute/FrameNoM:
    //    -CAV LDs: Digits 2-3 of current frame number in BCD format, IE X in "?XX??".
    //    -CLV LDs: Minutes counter of current time code in BCD format, IE X in "?XX????".
    //    -CDs: Minutes counter of current relative track time in BCD format. IE, each track begins again at 0.
    data = 0;
    if (mcd.cdd.isDiscLoaded() && (currentDriveState >= 5)) {
      if (mcd.cdd.isDiscLaserdisc()) {
        auto frameNumber = zeroBasedFrameIndexFromLba(mcd.cdd.getCurrentSector()) + 1;
        if (mcd.cdd.isLaserdiscClv()) {
          data = BCD::encode((frameNumber / (60 * 30)) % 60);
        } else {
          data = BCD::encode((frameNumber / 100) % 100);
        }
      } else {
        mcd.cdd.getCurrentTrackRelativeTimecode(minute, second, frame);
        data = BCD::encode(minute);
      }
    }
    break;
  case 0x18:
    //         --------------------------------- (Buffered in $5928)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x18|-------------------------------|
    // 0xFDFEB1|    Current Second/FrameNoL    |
    //         ---------------------------------
    // Current Second/FrameNoL:
    //    -CAV LDs: Digits 4-5 of current frame number in BCD format, IE X in "???XX".
    //    -CLV LDs: Seconds counter of current time code in BCD format, IE X in "???XX??".
    //    -CDs: Seconds counter of current relative track time in BCD format. IE, each track begins again at 0.
    data = 0;
    if (mcd.cdd.isDiscLoaded() && (currentDriveState >= 5)) {
      if (mcd.cdd.isDiscLaserdisc()) {
        auto frameNumber = zeroBasedFrameIndexFromLba(mcd.cdd.getCurrentSector()) + 1;
        if (mcd.cdd.isLaserdiscClv()) {
          data = BCD::encode((frameNumber / 30) % 60);
        } else {
          data = BCD::encode(frameNumber % 100);
        }
      } else {
        mcd.cdd.getCurrentTrackRelativeTimecode(minute, second, frame);
        data = BCD::encode(second);
      }
    }
    break;
  case 0x19:
    //         --------------------------------- (Buffered in $5929)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x19|-------------------------------|
    // 0xFDFEB3|         Current Frame         |
    //         ---------------------------------
    // Current Frame:
    //    -CAV LDs: This is set to 0x00.
    //    -CLV LDs: Frames counter of current time code in BCD format, IE X in "?????XX".
    //    -CDs: Frames counter of current relative track time in BCD format. IE, each track begins again at 0.
    data = 0;
    if (mcd.cdd.isDiscLoaded() && (currentDriveState >= 5)) {
      if (mcd.cdd.isDiscLaserdisc()) {
        if (mcd.cdd.isLaserdiscClv()) {
          auto frameNumber = zeroBasedFrameIndexFromLba(mcd.cdd.getCurrentSector()) + 1;
          data = BCD::encode(frameNumber % 30);
        } else {
          data = 0x00;
        }
      } else {
        mcd.cdd.getCurrentTrackRelativeTimecode(minute, second, frame);
        data = BCD::encode(frame);
      }
    }
    break;
  case 0x1A:
    //         --------------------------------- (Buffered in $592A)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1A|-------------------------------|
    // 0xFDFEB5|          StopChapter          |
    //         ---------------------------------
    // StopChapter: Reports the target chapter number to stop at, as a BCD number. Set to 0xFF if no stop chapter is
    //              latched, including if the corresponding input register is set to 0x00.
    // -Note that this output is only driven when a stop location is set, at the exact time it is latched. After this
    //  point, the output register can be freely written to again as it is not driven. Changes from writing to the
    //  output register do not change the actual stop location however, and the player will only stop at the actual time
    //  that was latched. This applies for all the following stop location output registers too.
    // -This register is also driven to 0xFF when the stop time is reached. This applies for all the following stop
    //  location output registers too.
   [[fallthrough]];
  case 0x1B:
    //         --------------------------------- (Buffered in $592B)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1B|-------------------------------|
    // 0xFDFEB7|           StopFrame           |
    //         ---------------------------------
    // StopFrame:
    //    -CAV LDs: This is set to 0x00.
    //    -CLV LDs: Frames counter of target stop point in BCD format, IE X in "?????XX".
    //    -CDs: Frames counter of target stop point in BCD format
   [[fallthrough]];
  case 0x1C:
    //         --------------------------------- (Buffered in $592C)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1C|-------------------------------|
    // 0xFDFEB9|     StopSeconds/FrameNoL      |
    //         ---------------------------------
    // StopSeconds/FrameNoL:
    //    -CAV LDs: Digits 4-5 of target stop point in BCD format, IE X in "???XX".
    //    -CLV LDs: Seconds counter of target stop point in BCD format, IE X in "???XX??".
    //    -CDs: Seconds counter of target stop point in BCD format
   [[fallthrough]];
  case 0x1D:
    //         --------------------------------- (Buffered in $592D)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1D|-------------------------------|
    // 0xFDFEBB|     StopMinutes/FrameNoM      |
    //         ---------------------------------
    // StopMinutes/FrameNoM:
    //    -CAV LDs: Digits 2-3 of target stop point in BCD format, IE X in "?XX??".
    //    -CLV LDs: Minutes counter of target stop point in BCD format, IE X in "?XX????".
    //    -CDs: Minutes counter of target stop point in BCD format
   [[fallthrough]];
  case 0x1E:
    //         --------------------------------- (Buffered in $592E)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1E|-------------------------------|
    // 0xFDFEBD|      StopHours/FrameNoH       |
    //         ---------------------------------
    // StopHours/FrameNoH:
    //    -CAV LDs: Upper digit of target stop point in BCD format, IE X in "X????".
    //    -CLV LDs: Hours counter of target stop point in BCD format, IE X in "X??????".
    //    -CDs: This is set to 0x00.
    // -Note that only the lower digit of the source hours/frame value is latched, IE, if the source register is
    //  set to 0x33, this output register will read 0x03.
    // -Despite only the lower four bits of this register being set based on the input, all 8 bits are driven, so
    //  if the source register is set to 0x33 the register will read 0x03 even if the output register was manually
    //  written as 0xFF prior.
    // -The same applies to when the register is driven to 0xFF when the stop point is reached. That value will
    //  still be set, not 0x0F.
   [[fallthrough]];
  case 0x1F:
    //         --------------------------------- (Buffered in $592F)
    // Output  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1F|-------------------------------|
    // 0xFDFEBF|       StopPointLatched        |
    //         ---------------------------------
    // StopPointLatched: This output is driven to 0x01 when a stop point has been latched, and reset to 0x00 when the
    //       stop time is reached and the player stop is triggered. Note that all lines are actually driven when the
    //       output is updated, so all bits will be reset even though only bit 0 has an identified change in value.
    break;
  default:
    break;
  }

  // Record the updated data value in the output register block
  outputRegs[regNum] = data;

  // Return the calculated value to the caller
  return data;
}

auto MCD::LD::processInputRegisterWrite(int regNum, n8 data, n8 previousData, bool wasDeferredRegisterWrite) -> void
{
  // Trigger any required changes based on the updated input register
  switch (regNum) {
  case 0x00:
    //         --------------------------------- (Buffered in $5930 (edit buffer)/ and $5050 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x00|-------------------------------|
    // 0xFDFE41|*U7|*U6| -   -   -   -   - |*U0|
    //         ---------------------------------
    // ##NEW## 2025-07
    // *U0:
    //      -Changing the value of this bit while input registers are unfrozen, or the effective value compared to
    //       the previously latched value when input registers are being unfrozen, triggers a seek operation.
    //      -It is only the transition that matters for this behaviour, IE, it was 0 but is being set to 1, or it
    //       was 1 but is being set to 0.
    //      -There is no detected difference between 0 and 1 for this register bit yet, however we expect there may
    //       be a difference to discover with further testing, now that we know there's a connection with seeking.
    // ##ENDNEW##
    // *U7: Apply register writes. When this bit is cleared, most register writes are not latched, and do not take
    //      effect until this register is modified to set this bit to true, at which point, those register writes
    //      take effect. Note that the last written data can still be read back by reading the current values of the
    //      input registers, they just don't take effect on the hardware immediately.
    //      -Note that the way this is implemented, when this register is set with this value set to true, all
    //       affected registers simply get applied immediately, whether they've changed or not. If seeking is
    //       enabled, this also means that a seek operation will be performed immediately at this point, whether
    //       any seeking registers were modified while register writes were disabled or not. ##NOTE## As of 2025-07,
    //       we now know this note is incorrect. We do not see this immediate application with seeking being
    //       performed, rather, we now believe this was mistakenly being seen due to us changing U0 at the same time,
    //       which we now know can trigger a seek if its value is different after being unfrozen than it was before
    //       being frozen.
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
      outputRegs[0] = (outputRegs[0] & 0x3E) | (data & 0xC1);
    }

    // If the state of U0 has changed since the last time it was latched, check for seek and stop point operations
    // to perform.
    //##TODO## Do more testing around this behaviour
    if (previousData.bit(0) != data.bit(0)) {
      // Clear the stop point triggered flag, regardless of whether seeking is enabled or not. This has been
      // confirmed through hardware tests, and Ghost Rush relies on this when moving around the mansion.
      reachedStopPoint = false;

      // Trigger a seek if required, based on the seek update flag.
      if (seekEnabled) {
        if (liveSeekRegistersContainsLatchableTarget()) {
          // Note that if latching fails because the seek target isn't valid (IE, invalid timecode), no seek operation
          // occurs, not even to the previously valid seek target.
          //##TODO## Note that we've seen that when latching a seek target fails in this manner, it leaves the
          //previously latched target seek location in an inconsistent state. The data buffers are apparently
          //partially updated, so triggering a seek operation to the previously valid seek location from this
          //point on will seek to a consistent, but different address. This behaviour should be probed more, as
          //it should be possible to discover the sequence and method of decoding based on the actual seek
          //locations from the resulting latched seek target when failed updates occur.
          if (latchSeekTargetFromCurrentState()) {
            performSeekWithLatchedState();
          }
        } else {
          // A seek will stll be performed here, but it will go to the last valid latched seek target. Note that this
          // may not correlate in any way with the current input register state.
          performSeekWithLatchedState();
        }
      }

      // As per hardware testing, when input reg 0x00 bit 0 is toggled from either 1 to 0, or 0 to 1, while input
      // register changes are not frozen or are being unfrozen, and while current live input register 0x03 state is
      // valid, and while either single-frame frame skip or step operations are set, it will cause the frame to latch
      // or advance again. This means just inverting bit 0 of reg 0x00 is enough to trigger a new frame to latch, or a
      // new frame to step. The "direction" bit of output reg 0x07 doesn't
      auto livePlaybackMode = inputRegs[0x03].bit(4, 7);
      auto livePlaybackSpeed = inputRegs[0x03].bit(0, 2);
      auto livePlaybackDirection = inputRegs[0x03].bit(3);
      if ((livePlaybackMode == 0x01) && (currentPlaybackMode == 0x01) && (livePlaybackSpeed == 0x01) && (currentPlaybackSpeed == 0x01) && (video.frameSkipCounter == -1)) {
        // Trigger single-frame frame skip mode to re-latch on the next frame
        video.frameSkipBaseFrame = 0;
        video.frameSkipCounter = 0;
      } else if ((livePlaybackMode == 0x02) && (currentPlaybackMode == 0x02) && (livePlaybackSpeed == 0x01) && (currentPlaybackSpeed == 0x00)) {
        // Trigger single-frame frame step mode to re-latch on the next frame
        currentPlaybackSpeed = 0x01;
      }
    }
    break;
  case 0x01:
    //         --------------------------------- (Buffered in $5931 (edit buffer)/ and $5051 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x01|-------------------------------|
    // 0xFDFE43| *U76  | -   - |     *U30      |
    //         ---------------------------------
    //##NEW## 2025:
    // *U76: Analog mixing mode selection. The following modes are defined:
    //       -0x0: No video mixing, no audio mixing. VDP graphics are shown, digital audio is played. Analog audio is
    //             disabled, and analog video is hidden. The digital memory light on the front of the unit remains lit
    //             in this mode. Input reg 0x0C has no apparent effect under this mode, including bit 7 to set PSC.
    //             Picture stop is enabled under this mode and cannot be turned off. Input regs 0x0D and 0x0F also have
    //             no effect on digital audio under this mode. Digital audio is played in stereo at full volume. Note
    //             that actually, this isn't entirely true. If mixing mode was set to 0x1 or higher, and reg 0x0D bits
    //             4-7 was set to a mode that disabled digital audio, digital audio remains disabled when switching
    //             directly to mixing mode 0x0, however changing register 0x0D no longer has any effect, meaning
    //             digital audio remains disabled, until the mixing mode is changed back and reg 0x0D is modified. Note
    //             that this is different to input regs 0x0D and 0x0F, which do not remain in effect when mixing mode
    //             0x0 is selected.
    //       -0x1: Video mixing enabled without field selection, audio mixing set to single source. Under this mode,
    //             input reg 0x0C is mostly effective. VDP and analog video can be mixed, however reg 0x0C bit 0 to
    //             select fields has no effect. Input reg 0x0D bits 4-7 have the following effects on audio under this
    //             mode:
    //                  -0x0: Digital audio enabled
    //                  -0x1: Analog audio enabled
    //                  -0x2: Digital audio enabled
    //                  -0x3: Analog audio enabled
    //                  -0x4: Digital audio enabled
    //                  -0x5: Analog audio enabled
    //                  -0x6: Digital audio enabled
    //                  -0x7: Analog audio enabled
    //                  -0x8: No audio
    //                  -0x9: Analog audio enabled
    //                  -0xA: No audio
    //                  -0xB: Analog audio enabled
    //                  -0xC: Digital audio enabled (Input reg 0x0F ignored - Digital audio full volume always)
    //                  -0xD: Analog audio enabled
    //                  -0xE: Digital audio enabled (Input reg 0x0F ignored - Digital audio full volume always)
    //                  -0xF: Analog audio enabled
    //       -0x2,0x3: Full video and audio mixing. Under this mode, input reg 0x0C is fully effective, including
    //             field selection. Analog and digital audio sources can be played simultaneously and mixed together.
    //             Input reg 0x0D bits 4-7 have the following effects on audio under this mode:
    //                  -0x0: Analog and digital audio enabled
    //                  -0x1: Analog and digital audio enabled
    //                  -0x2: Analog and digital audio enabled
    //                  -0x3: Analog and digital audio enabled
    //                  -0x4: Analog and digital audio enabled
    //                  -0x5: Analog and digital audio enabled
    //                  -0x6: Analog and digital audio enabled
    //                  -0x7: Analog and digital audio enabled
    //                  -0x8: Analog audio enabled
    //                  -0x9: Analog audio enabled
    //                  -0xA: Analog audio enabled
    //                  -0xB: Analog audio enabled
    //                  -0xC: Analog and digital audio enabled (Input reg 0x0F ignored - Digital audio full volume always)
    //                  -0xD: Analog and digital audio enabled (Input reg 0x0F ignored - Digital audio full volume always)
    //                  -0xE: Analog and digital audio enabled (Input reg 0x0F ignored - Digital audio full volume always)
    //                  -0xF: Analog and digital audio enabled (Input reg 0x0F ignored - Digital audio full volume always)
    // *U30: Set by UNK11F based on a value stored at 0x1A81. No observed effect from changing this yet.
    break;
  case 0x02: {
    //         --------------------------------- (Buffered in $5932 (edit buffer)/ and $5052 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x02|-------------------------------|
    // 0xFDFE45|*U7| ? |*U5|*U4|     *U30      |
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
    // -We've confirmed operationErrorFlag3 is cleared when the seek mode changes too
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

    // If no actual change has occurred to the "important" bits in this register, don't do anything. We do this because
    // it's important that the error flags are not cleared if no real change has taken place, nor is a seek operation
    // performed when writing a value of 0x25 after writing a value of 0x65 for example. It appears that changes to the
    // lower six bits cause things to effectively change/re-run, while writes to the upper two bits do not. LDRom2 games
    // rely on this, to ensure the error flags are not cleared after reaching a stop point just because register writes
    // are suspended then restored for example, but the same property is true and valid here with just writing to the
    // register directly when writes are active.
    if (data.bit(0, 5) == previousData.bit(0, 5)) {
      break;
    }

    // Update the seek enable state
    seekEnabled = data.bit(5);

    // Clear the operation error flags before we do anything
    operationErrorFlag1 = false;
    operationErrorFlag2 = false;
    operationErrorFlag3 = false;

    // Update the pause state if required. Note that hardware tests have shown that the pause flag is only effective
    // when it changes state at the same time as drive state 0x05 or 0x07 requests are being issued, otherwise it is
    // ignored. Note that this does mean if the flag changes state along with a different drive state being issued, the
    // pause flag will have to be set back to its old value again, then a second register write made along with a drive
    // state of 0x05 or 0x07 in order for it to be effective.
    auto newDriveState = data.bit(0, 3);
    auto pauseFlag = data.bit(4);
    auto previousPauseFlag = previousData.bit(4);
    if (pauseFlag != previousPauseFlag) {
      if ((newDriveState == 0x0) || (newDriveState == 0x5) || (newDriveState == 0x7)) {
        targetPauseState = pauseFlag;
        currentPauseState = targetPauseState;
      }
    }

    // Update the target drive state
    targetDriveState = newDriveState;

    // Apply the new requested drive state
    switch (targetDriveState) {
    case 0x01: // Open tray
      mcd.cdd.eject();
      currentDriveState = 0x01;
      resetSeekTargetToDefault();
      break;
    case 0x02: // Close tray
      mcd.cdd.stop();
      currentDriveState = 0x02;
      resetSeekTargetToDefault();
      break;
    case 0x03: // Unload disc
      mcd.cdd.stop();
      currentDriveState = 0x03;
      resetSeekTargetToDefault();
      break;
    case 0x04: // Load disc
      // If no disc is present, flag an error and abort.
      if (!mcd.cdd.isDiscLoaded()) {
        operationErrorFlag1 = true;
        operationErrorFlag2 = true;
        break;
      }
      // If the disc isn't already loaded, insert it and seek to the start, otherwise do nothing.
      if (currentDriveState <= 3) {
        mcd.cdd.insert();
        resetSeekTargetToDefault();
        mcd.cdd.seekToTrack(1, true);
        seekPerformedSinceLastFrameUpdate = true;
        currentPauseState = true;
      }
      // If the disc is already loaded, the drive state doesn't change back to 0x04 when we set it here, instead an
      // error is flaggged.
      if (currentDriveState < 0x04) {
        currentDriveState = 0x04;
      } else {
        operationErrorFlag1 = true;
      }
      break;
    case 0x05: { // Load and play disc
      // If no disc is present, flag an error and abort.
      if (!mcd.cdd.isDiscLoaded()) {
        operationErrorFlag1 = true;
        operationErrorFlag2 = true;
        break;
      }
      // Clear the stop point triggered flag if seeking is enabled. It has been confirmed through hardware tests that
      // the stop point is only cleared when seeking is enabled, and only when setting drive state 0x05, not 0x06 or
      // 0x07. Ghost Rush relies on this when entering the tutorial from new character creation.
      if (seekEnabled) {
        reachedStopPoint = false;
      }
      // If the disc isn't already loaded, insert it and seek to the start.
      bool performedLoadOfDisc = false;
      if (currentDriveState <= 3) {
        mcd.cdd.insert();
        resetSeekTargetToDefault();
        mcd.cdd.seekToTrack(1, true);
        seekPerformedSinceLastFrameUpdate = true;
        currentPauseState = true;
        performedLoadOfDisc = true;
      }
      // Update the pause and drive state. Note that we have to update the drive state before calling our seek
      // function below.
      currentPauseState = targetPauseState;
      currentDriveState = 0x05;
      // If seeking is enabled, and this wasn't a deferred register write during a frozen input register block update,
      // perform the currently active seek operation.
      if (seekEnabled && (performedLoadOfDisc || !wasDeferredRegisterWrite)) {
        //##TODO## Confirm what happens in this case if we have a valid seek mode, but the target is invalid. What do the error flags do? What does seeking do?
        //##TODO## Further testing needed on seek failure responses. We had mixed results here.
        if (liveSeekRegistersContainsLatchableTarget()) {
          if (latchSeekTargetFromCurrentState()) {
            performSeekWithLatchedState();
          }
        } else {
          performSeekWithLatchedState();
        }
      }
      // Either play or pause the disc depending on the pause flag and whether we've reached an
      // active stop point
      if (currentPauseState || reachedStopPoint) {
        mcd.cdd.pause();
      } else {
        mcd.cdd.play();
      }
      break; }
    case 0x00: // Toggle pause from running state
    case 0x07:
      // If no disc is present, flag an error if target state is 0x07, and abort for either 0x00 or 0x07.
      if (!mcd.cdd.isDiscLoaded()) {
        if (targetDriveState == 0x07) {
          operationErrorFlag1 = true;
          operationErrorFlag2 = true;
        }
        break;
      }
      // Note that as this is a command rather than an actual state change, we don't update the current drive state
      // here.
      if (currentDriveState == 0x05) {
        if (targetPauseState || reachedStopPoint) {
          mcd.cdd.pause();
        } else {
          mcd.cdd.play();
        }
      }
      break;
    default: // Invalid mode
      operationErrorFlag1 = true;
      if (!mcd.cdd.isDiscLoaded()) {
        operationErrorFlag2 = true;
      }
      break;
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
    // -With mode 0x1 for frame skipping and a rate of 0x01, IE, when 0x11 is written to this register, output reg 0x07
    //  uses the "reverse" bit to indicate whether the single frame skip has been completed. This means writing 0x11 or
    //  0x19 will initially have output reg 0x07 reporting 0x11, then changing to 0x19 when completed.
    // -With mode 0x2 for frame stepping and a rate of 0x00, IE, when 0x21 or 0x29 is written to this register, output
    //  reg 0x07 will report the same value as this input register before the frame step is complete. After it is
    //  complete and a single new frame has been shown, both the reverse bit and the rate will be set to 0x0, so the
    //  output reg will read 0x20 if either 0x21 or 0x29 is written once the step has been processed, even though the
    //  reverse bit is effective in controlling the step direction.
    // -Toggling bit 0 of input reg 0x00 will cause the frame skip/step to be run again for rate 0x01. In the case of
    //  frame skip, this causes the output register to toggle its "reverse" bit again, so if it was previously on 0x19,
    //  after processing the frame skip again, it will now be 0x11, even though input reg 0x03 hasn't changed. In the
    //  case of frame step mode, it will apply just as before, with the current effective value of input reg 0x03 being
    //  output to output reg 0x07, until the step has been processed, at which time it will revert to 0x20.
    // -Freezing then unfreezing the input register block does not cause skipping/stepping to occur again, unless input
    //  reg 0x00 bit 0 has also had its effective value changed.
    // -Note that the live state of input reg 0x03 is used when toggling bit 0 of input reg 0x00. If the last attempted
    //  write to this register set an invalid playback mode, toggling bit 0 of input reg 0x00 will not cause frame skip
    //  or step at rate 1 to advance while invalid data is in input reg 0x03.
    // -Note that "bouncing" back and forth between 0x11 and 0x19 will cause the frame skip mode to advance by 1, with
    //  the direction bit in output reg 0x07 toggling, just like if we toggled bit 0 of input reg 0x00.
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
    //       -0x7: ~0.33r FPS instead of 30 (30 seconds for 10 frames) - Display frame 90x
    //       -0x6: 1 FPS instead of 30 (45 seconds for 45 frames) - Display frame 30x
    //       -0x5: 1.875 FPS instead of 30 (48 seconds for 90 frames) - Display frame 16x
    //       -0x4: 3.75 FPS instead of 30 (24 seconds for 90 frames) - Display frame 8x
    //       -0x3: 7.5 FPS instead of 30 (24 seconds for 180 frames) - Display frame 4x
    //       -0x2: 15 FPS instead of 30 (12 seconds for 180 frames) - Display frame 2x
    //       -0x1: 1 frame only. The image will not update after the initial frame. Note that under frame step mode, output register
    //             0x07 will report this step speed as 0x1 only until the single frame step has been performed, after which, the output
    //             register will now state a value of 0x0. Also note that under frame skip mode, the output register will output 0x19 for
    //             an input register state of 0x11, in other words, the "direction" bit is set to "reverse" when a 1-frame frame skip mode
    //             is activated.
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

    // If no actual change has occurred to this register, don't do anything. We do this because it's important that the
    // error flags are not cleared if no real change has taken place. LDRom2 games rely on this, to ensure the error
    // flags are not cleared after reaching a stop point just because register writes are suspended then restored for
    // example.
    if (data == previousData) {
      break;
    }

    // Clear the operation error flags before we do anything
    operationErrorFlag1 = false;
    operationErrorFlag2 = false;
    operationErrorFlag3 = false;

    // Latch the new playback mode settings. Note that this does work for CDs and well as LDs.
    auto newPlaybackMode = data.bit(4, 7);
    if (newPlaybackMode >= 0x04) {
      operationErrorFlag1 = true;
    } else {
      // Latch the new playback mode settings
      auto newPlaybackSpeed = data.bit(0, 2);
      auto newPlaybackDirection = data.bit(3);
      switch (newPlaybackMode) {
      case 0:
        newPlaybackSpeed = 0;
        break;
      case 1:
        // Playback direction is fixed at 0. For a speed of 1, indicating single skip, this will be set to 1 when
        // complete. When we bounce back and forth between 0x11 and 0x19, it toggles, like it does when toggling bit 0
        // of input register 0x00.
        newPlaybackDirection = 0;
        break;
      case 2:
        // Playback direction is set to 0 for a speed of 0, or for a speed of 1 indicating a single frame, it is
        // retained, then cleared later when the step is complete.
        newPlaybackDirection = (newPlaybackSpeed == 0) ? 0 : newPlaybackDirection;
        break;
      case 3:
        if (newPlaybackSpeed == 0x07) {
          newPlaybackSpeed = 0x06;
        }
        break;
      }
      currentPlaybackMode = newPlaybackMode;
      currentPlaybackSpeed = newPlaybackSpeed;
      currentPlaybackDirection = newPlaybackDirection;

      // Any valid writes to this register which actually change the data reset the stop point hit state, and start
      // playback resuming if we're in the right player state. Writes with an invalid playback mode are ignored. Ghost
      // Rush relies on this.
      reachedStopPoint = false;

      // Wake the player up from being stopped at a stop point when a valid change occurs to the playback mode. Back to
      // the Edo relies on this when getting an answer wrong. Note that this does not resume playback however if the
      // player is genuinely in a paused state.
      if (currentDriveState == 0x05) {
        if (currentPauseState) {
          mcd.cdd.pause();
        } else {
          mcd.cdd.play();
        }
      }
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
    // -Values 0x01-0x99 return TOC track information in output registers 0x11-0x14, specifically the CONTROL and PMIN/PSEC/PFRAME
    //  values from the TOC Q channel subcode data.
    // -If a track number past the last track is specified, output reg 0x11 is set to 0x00 and 0x12-0x14 are set to 0xFF.
    // -Value 0xA0 returns the first valid track number in output reg 0x12, and the last valid track number in 0x13. Used by bios.
    // -Value 0xA1 appears to returns the start of leadout time in the min/sec/frame regs
    // -Values 0xB0 and 0xB1 appear identical to 0xAx counterparts for a SegaCD game, but must be able to be something different.
    // -All these values are used by the bios
    // -Note that this all uses the redbook digital track information, including on LDs. If an LD has no digital track, 0xA0/0xB0
    //  return 0x01 for the first track and 0x99 for the last, and 0x00 for all outputs in 0xA1/0xB1.
    // -Despite notes below, this is a BCD value, it's just that 0x0A=10, 0x0B=11, etc, so 0x0A and 0x10 will give the same output.
    // -Data reported for TOC information is retained even when the disc is stopped (drive state 0x02) until it is unloaded (0x01).
    // -Data reported for currently playing track info is also retained even when stopped like the TOC information.
    // ##OLD## Before 2025
    // ##NOTE##
    // -If an invalid track number is selected (>0x99), output registers 0x11-0x14 actually report internal data instead. It appears
    //  that somewhere, there's an internal data buffer, which the TOC information is loaded into. This isn't the only info in this
    //  buffer however. Upper register values have been observed to contain the internal state of data buffers which are directed to
    //  other output registers, as well as lots of unidentified data, some of which changes constantly (possibly running counters or
    //  timers of some kind). The correct content for these upper register values will take quite awhile to map out.
    // -Entering a value of 0xFF returns the exact same data as a value of 0x00. This may be because the actual current counter data
    //  is stored in the internal memory at this location, rather than this being a properly supported value.
    // ##TODO## Document the internal upper register values accessed in this data block
    selectedTrackInfo = (data < 0x99 ? BCD::decode(data) : (u8)data);
    break;
  case 0x06: {
    //##NEW## 2025
    //         --------------------------------- (Buffered in $5936)
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x06|-------------------------------|
    // 0xFDFE4D|RP | -   -   -   - |TF |  SM   |
    //         ---------------------------------
    //SM - Seek Mode: (Defaults to 0x1, Saved state reset when disc stopped/drive state 0x03)
    //	0x0 - No change (use last set seek mode)
    //	0x1 - Seek to track relative
    //	0x2 - Seek to absolute
    //	0x3 - Set stop point
    //TF - Time Format: (Defaults to 0x1, Saved state reset when disc stopped/drive state 0x03)
    //RP - Repeat:
    //	0x0 - Stop when stop point reached and clear stop point.
    //	0x1 - Trigger seek (last successfully latched) when stop point reached and don't clear stop point.
    //
    //
    //Seek mode 4D/reg 0x06:
    //-Seeking is performed even if jumping directly from state 0x01 to 0x25.
    //-Seek mode and targets are latched! They do not use the live register state from input regs 0x06-0x0B. If you do a seek to track 0x02 for example with seek mode 0x01 or 0x02, then change to seek mode 0x03 which doesn't trigger a seek, then switch to 0x05 then back to 0x25, the player will trigger the previously set seek to track 0x02 with the latched seek mode that was applied, even though the input regs now say something else entirely.
    //
    //##NOTE##
    //-Writing to the seek registers may cause immediate seeking, but it will NOT resume from a pause state. If the player has paused by hitting a stop point, it will stay stopped after modifying the seek registers and the player seeking to the target location. Unlike modifying the seek registers though, writing to reg 0x02 to set the mechanical drive state to something else (IE, 0x26 or 0x07 then back to 0x25) will cause a seek and playback to resume. Changing to 0x05 however will NOT cause a seek and a resume, the seek needs to be triggered to cause the resume.
    //
    //-0x00/0x04: Seek to track no
    //	CD:
    //	-Sets output reg 0x09 U4 (operationErrorFlag1) and U0 (seekError) when used while a CD is playing
    //	-Errors when a CD is loaded are set even when seeking is disabled (45=0x05)
    //	-Does NOT perform a seek immediately when the seek mode is set to this value
    //	-Seeking DOES NOT happen instantly when any of the input regs 0x07-0x0B are written to
    //	CAV LD (no CDD):
    //	-Just seeks to the start of the target video track indicated by input reg 0x07. Other registers do nothing.
    //	-If the target track no is invalid (past the end of the track count), sets output reg 0x09 U4 (operationErrorFlag1) and U0 (seek Error) without attempting seeking or interrupting playback.
    //	CAV LD (with CDD):
    //	-Same as CAV LD no CDD.
    //	-Setting target track 0x00 does perform a seek and works the same as track 0x01. May be the same for CD and no CDD, untested.
    //	CLV LD:
    //	-We've actually confirmed that this sets output reg 0x09 U4 (operationErrorFlag1) and U0 (seekError) always, regardless of the current drive state, or whether seeking is enabled or not. Well, not when the drive state is changed to 0x05 or 0x25, that will perform the seek to chapter. Rather, when the seek mode is changed to 0x00 or 0x04, or any of the following regs 0x07-0x0B are written to when seek mode is 0x00 or 0x04, reg 0x09 will be set to 0x11.
    //	-As per the above, only chapter seeking works. No effect from other regs.
    //	-##FIX## 0x04 just worked with seeking to a start time from a track beginning!
    //	-Maybe it picks up the last valid seek point?
    //	-Correction to above: Chapter 0x00 is the same as seeking to the FIRST chapter. On a multi-disc setup the first chapter number may not be 0.
    //	-Additionally, if seeking is enabled, and a valid seek target is not set for seeking to with this mode, when trying to transition from drive state 0x04 to 0x05, the attempt will fail with output reg 0x09 set to 0x11, and the player will remain paused. Actually, this only happens if the drive has previously been at state 0x04 or above, with the TOC loaded. If you attempt this from a drive state of 0x01 for example, or a drive state of 0x03 or less when the TOC has never been loaded, the attempt to play will succeed, the drive will transition to state 0x05, but it will play from the beginning.
    //	-Ok, to be clear, under CLV LD, there are definitely two different modes here:
    //	-0x04: Seek to absolute time
    //	-Wait, this is also wrong. What 0x00 and 0x04 do is act as the last valid seek target. They do not change the current seek mode at all.
    //	-Note that 
    //	-The following input regs 0x07-0x0B will be applied when the seek is performed based on their current state, so if we last seeked to a given time, then we disable seeking, switch to mode 0x00 or 0x04 and change the target time, then perform seeking, the new time will be used, not the previous one.
    //	-This means the initial seek mode is 0x01 or 0x05, to perform track based seeking.
    //	-Now we're back to not being able to reproduce this properly
    //
    //-0x01/0x05: Seek to track relative time
    //	CD:
    //	-Will perform a seek to start of indicated CD track number, plus the relative time within the track.
    //	-Does NOT perform a seek immediately when the seek mode is set to this value
    //	-Seeking happens instantly when any of the input regs 0x07-0x0B are written to
    //	-Reg 0x08 has no effect on the seek, but writing to it will still trigger seeking.
    //	-Sets output reg 0x09 U4 (operationErrorFlag1) and U0 (seekError) when an invalid track number is given, and seeking is currently enabled. No effect if it is disabled.
    //	-If the target time is invalid (past the end of the target track), sets output reg 0x09 U4 (operationErrorFlag1) and U0 (seekError) without attempting seeking or interrupting playback.
    //	CAV LD (no CDD):
    //	-Seeks to the beginning of the target video track.
    //	-Input regs 0x07-0x0B trigger seeking when written, but have no effect.
    //	CAV LD (with CDD):
    //	-0x01 and 0x05 behave differently!
    //		0x01:
    //		-Seeks to a relative time from the beginning of the digital audio track.
    //		-Input regs 0x09-0x0B are MM:SS:FF time for the CD tracks, IE, 75 frames per second.
    //		-Input reg 0x08 triggers seeking but has no effect
    //		0x05:
    //		-Like CAV LD without CDD, seeks to start of target track, no effect from other regs.
    //	CLV LD (with CDD):
    //	-0x01 as per CAV LD (with CDD), seeks based on CD tracks using MM:SS:FF from regs 0x09-0x0B, no effect from 0x08.
    //	-0x05 as per CAV LD (with CDD) and CAV LD (no CDD), seeks to start of track only.
    //	-Note that the relative time CAN be past the end of the track, it will just seek to the target location however far ahead it is.
    //
    //-0x02/0x06: Seek to absolute time
    //	CD:
    //	-Will perform a seek to indicated absolute CD time
    //	-Performs seek immediately when the seek mode is set to this value, unlike other modes.
    //	-Seeking happens instantly when any of the input regs 0x07-0x0B are written to
    //	-Regs 0x07 and 0x08 have no effect on the seek, but writing to them will still trigger seeking.
    //	-If the target time is invalid (past the end of the disc), sets output reg 0x09 U4 (operationErrorFlag1) and U0 (seekError) without attempting seeking or interrupting playback.
    //	CAV LD (no CDD):
    //	-Seeks to an absolute video time (even though this is CAV) in MM:SS:FF. Reg 0x08 with hours doesn't have any effect, although writing to it does trigger seeking immediately.
    //	-Input regs 0x07-0x0B trigger seeking when written, but have no effect.
    //	-If the target time is invalid (past the end of the disc), player attempts to perform the seek, stops at the start of lead-out, and sets output reg 0x09 to 0x07.
    //	-0x02 and 0x06 behave differently!
    //		0x02: Seek to absolute video time
    //		0x06: Seek to absolute video frame
    //	-Behaviour is otherwise the same as described above. Reg 0x0B "frames" register does nothing, but performs seek when written to.
    //	CAV LD (with CDD):
    //	-0x02: Seeks based on absolute CD time
    //	-0x06: Seeks based on absolute video frame
    //	CLV LD (with CDD):
    //	-0x02: Seeks based on absolute CD time
    //	-0x06: Seeks based on absolute video time
    //
    //-0x03/0x07: Set stop point
    //	-Latches the following regs 0x07-0x0B to set an automatic stop point
    //	-If a chapter number is set, that applies. If it is 0, the following MM:SS:FF values apply.
    //	-If chapter number is 0xFF, all output regs are driven to 0xFF, and output reg 0x1F is driven to 0x01.
    //	-If chapter number is 0x00, output reg is driven to 0xFF.
    //	-No seeking triggered when this mode is set, just flags a stop point
    //	-Latches the values immediately when the seek mode is set to this value, unlike other modes.
    //	-Latches stop point register values live, IE, changing the regs immediately updates.
    //	-Latching works even with seeking disabled (45=0x05)
    //	-Latching only works when drive state is 0x04 or higher, otherwise ignored.
    //	-Latching triggers when drive state changed from below 0x04 to 0x04 or higher.
    //	CAV LD (no CDD):
    //	-Sets stop point as a frame number
    //	-If a chapter number is set, that applies instead, and frame number is ignored.
    //	-Definitely works the same on 0x03/0x07. No MM:SS:FF stop point here.
    //	-"Frames" field is ignored, and its output is driven to 0x00.
    //	CAV LD (with CDD):
    //	-Exactly the same as no CDD
    //	CLV LD (with CDD):
    //	-Other behaviour the same as above, except that time is HH:MM:SS:FF based on video time.
    //	-Definitely no difference between 0x03/0x07.

    // Update the current seek mode
    bool seekModeUpdated = false;
    auto targetSeekMode = data.bit(0, 1);
    if (((targetSeekMode == 1) || (targetSeekMode == 2)) && (currentSeekMode != targetSeekMode)) {
      currentSeekMode = targetSeekMode;
      seekModeUpdated = true;
    }

    // Update the current seek mode time format
    auto targetSeekModeTimeFormat = data.bit(2);
    if (((targetSeekMode == 1) || (targetSeekMode == 2)) && (currentSeekModeTimeFormat != targetSeekModeTimeFormat)) {
      currentSeekModeTimeFormat = targetSeekModeTimeFormat;
      seekModeUpdated = true;
    }

    // Either update the stop point or trigger a seek if required
    if (targetSeekMode == 3) {
      // Hardware tests have shown that the stop point is only latched again if an actual effective register state
      // change occurs to the lower three bits. This is true when changing the register directly, or when restoring
      // the input register block from a frozen state.
      if (previousData.bit(0, 2) != data.bit(0, 2)) {
        updateStopPointWithCurrentState();
      }
    } else if (seekModeUpdated && seekEnabled && (currentDriveState == 0x5) && (((currentSeekMode == 1) && (currentSeekModeTimeFormat == 1)) || (currentSeekMode == 2))) {
      if (latchSeekTargetFromCurrentState()) {
        performSeekWithLatchedState();
      }
    }

    break;}
  case 0x07:
    //         --------------------------------- (Buffered in $5937 (edit buffer)/ and $5057 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x07|-------------------------------|
    // 0xFDFE4F|           Track No            |
    //         ---------------------------------
    //##NEW## 2025
    // Track No: When a CD is loaded, seeking to track number 0x00 is invalid, and flags both U0 and U4 of output reg 0x09 if
    //           attempted, without affecting current playback.
    // -In seek mode 0x03, sets the chapter number to stop at when reached. If this is set to 0x00, the following
    //  frame/time registers will set a time-based stop point instead, and the output register will be set to 0xFF. If
    //  this is set to 0xFF, latching a stop point is disabled, and all stop point output registers will be driven to
    //  0xFF. Interestingly however, output register 0x1F will still be driven to 0x01 when this is done, I suppose to
    //  confirm the change has taken effect? This means that flag is probably a "stop point processed" flag.

    //##OLD## Before 2025
    // Track No: Requested track number for seeking. Note that 0x01 is the first track on the disk, 0x02 is the second, etc.
    // ##NOTE##
    // -Seek sets this to 0x00
    // -In CD mode, this was observed to be a BCD value. A value of 0x0A was the same as 0x10, 0x0B as 0x11, etc.
    [[fallthrough]];
  case 0x08:
    //         --------------------------------- (Buffered in $5938 (edit buffer)/ and $5058 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x08|-------------------------------|
    // 0xFDFE51|        Hours/FrameNoU         |
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
    [[fallthrough]];
  case 0x09:
    //         --------------------------------- (Buffered in $5939 (edit buffer)/ and $5059 (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x09|-------------------------------|
    // 0xFDFE53|       Minutes/FrameNoM        |
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
    [[fallthrough]];
  case 0x0A:
    //         --------------------------------- (Buffered in $593A (edit buffer)/ and $505A (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0A|-------------------------------|
    // 0xFDFE55|       Seconds/FrameNoL        |
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
    [[fallthrough]];
  case 0x0B: {
    //         --------------------------------- (Buffered in $593B (edit buffer)/ and $505B (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
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
    inputRegs[regNum] = data;
    auto targetSeekMode = inputRegs[0x06].bit(0, 1);
    if ((targetSeekMode == 3) && (previousData != data)) {
      updateStopPointWithCurrentState();
    }
    if (!wasDeferredRegisterWrite && seekEnabled && (currentDriveState == 0x5) && (((currentSeekMode == 1) && (currentSeekModeTimeFormat == 1)) || (currentSeekMode == 2))) {
      if (latchSeekTargetFromCurrentState()) {
        performSeekWithLatchedState();
      }
    }
    break;}
  case 0x0C:
    //         --------------------------------- (Buffered in $593C (edit buffer)/ and $505C (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x0C|-------------------------------|
    // 0xFDFE59|PSC| - |HLD|*U4|*U3|VD |DM |*U0|
    //         ---------------------------------
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
    // *U74: The way these bits interact with the "Analog mixing mode selection" bits U76 in input reg 0x01 is
    //       complex, but these bits affect audio track selection and operation. See notes on input reg 0x01 for
    //       a full description of how this operates.
    // -New testing has shown setting both RE and LE attenuates equivalent to a reg 0x0F filter of 0x40.
    // -Under CD mode at least (LD not tested) only bits 6 and 7 seem to have any effect, with these results:
    //       -00 = Normal
    //       -01 = Normal
    //       -10 = No audio
    //       -11 = Audio plays full volume (reg 0x0F ignored)
    // ##OLD## Before 2025
    // RE:  Digital audio right exclusive. Play right track in both speakers.  Half volume normal left/right output if combined with LE.
    // LE:  Digital audio left exclusive. Play left track in both speakers. Half volume normal left/right output if combined with RE.
    // ##OLD##
    // *Unknown: Set as a complete write by UNK138.
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
    // 0xFDFE7D| ? | ? | ? | ? |*U3|*U2|*U1|*U0|
    //         ---------------------------------
    // -This register mutes or applies attenuation to one or both analog audio channels, using input register 0x1F to
    // provide an attenuation value. 
    // -Note that changes to this register are effective even when the input register block is frozen.
    // *U3: Operation type
    //       -0x0: Attenuate the target analog audio channel(s) using the attenuation value in input register 0x1F
    //       -0x1: Fade to mute. Quickly over about .5 of a second, attenuates the target analog audio channel(s) until
    //             they are truly muted (IE, better than setting attenuation to 0xFF). The fade out characteristic is
    //             most likely to avoid popping. Note that as an unusual characteristic, this fade out behaviour only
    //             applies to the first channel muted, in the case that two channels are muted separately. If one
    //             channel is muted, it will fade out, then if the second channel is muted at any time later without
    //             restoring the first channel by setting its attenuation (even to 0xFF), the second channel will be 
    //             instantly muted with no fade out.
    // *U2: Single channel select
    //       -0x0: Operate on both left and right channels
    //       -0x1: Select left or right channel to operate on based on U0
    // *U1: Operation enable (0 = do nothing, 1 = apply)
    //       -0x0: Disabled. If this bit is 0, changes to this register or input register 0x1F have no effect.
    //       -0x1: Enabled. Writing to this register will apply the indicated operation. Changes to input register 0x1F
    //             will affect attenuation if this register is configured to use it.
    // *U0: Channel selection (0 = left, 1 = right)
    //       -0x0: Left channel. If U2 is set to 1, the operation will only affect the left analog audio channel.
    //       -0x0: Right channel. If U2 is set to 1, the operation will only affect the right analog audio channel.
    inputRegs[regNum] = data;
    if (inputRegs[0x1E].bit(1) && inputRegs[0x1E].bit(3)) {
      if (!inputRegs[0x1E].bit(2)) {
        analogAudioFadeToMutedLeft = true;
        analogAudioFadeToMutedRight = true;
      } else if (!inputRegs[0x1E].bit(0)) {
        analogAudioFadeToMutedLeft = true;
      } else {
        analogAudioFadeToMutedRight = true;
      }
    }
    [[fallthrough]];
  case 0x1F:
    //         --------------------------------- (Buffered in $594F (edit buffer)/ and $506F (last written))
    // Input   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    // Reg 0x1F|-------------------------------|
    // 0xFDFE7F|        AnalogAudioFader       |
    //         ---------------------------------
    // -Note that changes to this register are effective even when the input register block is frozen.
    // AnalogAudioFader: Sets the attenuation of the target analog audio channel(s) identified by input register 0x1E.
    //                   A lower value gives a higher volume. Setting this to 0xFF almost, but not quite, mutes the
    //                   target channel. If no valid attenuation target is currently defined by input register 0x1E,
    //                   changes to this register have no effect.
    if (inputRegs[0x1E].bit(1) && !inputRegs[0x1E].bit(3)) {
      if (!inputRegs[0x1E].bit(2)) {
        analogAudioAttenuationLeft = data;
        analogAudioAttenuationRight = data;
        analogAudioFadeToMutedLeft = false;
        analogAudioFadeToMutedRight = false;
      } else if (!inputRegs[0x1E].bit(0)) {
        analogAudioAttenuationLeft = data;
        analogAudioFadeToMutedLeft = false;
      } else {
        analogAudioAttenuationRight = data;
        analogAudioFadeToMutedRight = false;
      }
    }
    break;
  default:
    debug(unusual, "[MCD::LD::processInputRegisterWrite] reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
    break;
  }

  // Update the latched input register state
  inputRegs[regNum] = data;
}

auto MCD::LD::updateStopPointWithCurrentState() -> void {
  if (inputRegs[0x07] == 0xFF) {
    // Clear the stop point
    outputRegs[0x1A] = 0xFF;
    outputRegs[0x1B] = 0xFF;
    outputRegs[0x1C] = 0xFF;
    outputRegs[0x1D] = 0xFF;
    outputRegs[0x1E] = 0xFF;
    outputRegs[0x1F] = 0x01; // Strange as it is, yes, this gets set to 0x01. Probably a hardware bug, really should be 0x00.
    mcd.cdd.stopPointEnabled = false;
    debug(unverified, "Disabled stoppoint");
  } else {
    // Latch the stop point regs
    stopPointRegs[(int)SeekPointReg::Chapter] = (inputRegs[0x07] == 0x00) ? (n8)0xFF : inputRegs[0x07];
    stopPointRegs[(int)SeekPointReg::HoursOrFrameH] = (!mcd.cdd.isDiscLaserdisc() ? 0x00 : (inputRegs[0x08] & 0x0F)); // Only the lower 4 bits of this reg are accepted
    stopPointRegs[(int)SeekPointReg::MinutesOrFrameM] = inputRegs[0x09];
    stopPointRegs[(int)SeekPointReg::SecondsOrFrameL] = inputRegs[0x0A];
    stopPointRegs[(int)SeekPointReg::Frames] = (mcd.cdd.isDiscLaserdisc() && !mcd.cdd.isLaserdiscClv()) ? (n8)0x00 : inputRegs[0x0B];
    outputRegs[0x1A] = stopPointRegs[(int)SeekPointReg::Chapter];
    outputRegs[0x1B] = stopPointRegs[(int)SeekPointReg::Frames];
    outputRegs[0x1C] = stopPointRegs[(int)SeekPointReg::SecondsOrFrameL];
    outputRegs[0x1D] = stopPointRegs[(int)SeekPointReg::MinutesOrFrameM];
    outputRegs[0x1E] = stopPointRegs[(int)SeekPointReg::HoursOrFrameH];
    outputRegs[0x1F] = 0x01;

    // Convert the stop point to an LBA address
    //##FIX## This is incorrect for LDs. They should be using the actual frame data to set stop points, not the CD track
    //data.
    s32 stopLba;
    if (!mcd.cdd.isDiscLaserdisc()) {
      // CDs set stop points using MM:SS:FF
      stopLba = mcd.cdd.lbaFromTime(0, BCD::decode(stopPointRegs[(int)SeekPointReg::MinutesOrFrameM]), BCD::decode(stopPointRegs[(int)SeekPointReg::SecondsOrFrameL]), BCD::decode(stopPointRegs[(int)SeekPointReg::Frames]));
    } else if (!mcd.cdd.isLaserdiscClv()) {
      // CAV LDs set stop points using frame numbers
      s32 frameNumber = ((s32)BCD::decode(stopPointRegs[(int)SeekPointReg::HoursOrFrameH]) * 10000) + ((s32)BCD::decode(stopPointRegs[(int)SeekPointReg::MinutesOrFrameM]) * 100) + (s32)BCD::decode(stopPointRegs[(int)SeekPointReg::SecondsOrFrameL]);
      // Convert the frame number to zero-based, then to an LBA.
      stopLba = lbaFromZeroBasedFrameIndex(frameNumber - 1);
    } else {
      // CLV LDs set stop points using HH:MM:SS:FF
      u8 videoTimeHours = BCD::decode(stopPointRegs[(int)SeekPointReg::HoursOrFrameH]);
      u8 videoTimeMinutes = BCD::decode(stopPointRegs[(int)SeekPointReg::MinutesOrFrameM]);
      u8 videoTimeSeconds = BCD::decode(stopPointRegs[(int)SeekPointReg::SecondsOrFrameL]);
      u8 videoTimeFrames = BCD::decode(stopPointRegs[(int)SeekPointReg::Frames]);
      VideoTimeToRedbookTime(videoTimeHours, videoTimeMinutes, videoTimeSeconds, videoTimeFrames);
      stopLba = mcd.cdd.lbaFromTime(videoTimeHours, videoTimeMinutes, videoTimeSeconds, videoTimeFrames);
    }

    // Apply the stop point to playback control
    mcd.cdd.targetStopPoint = stopLba;
    mcd.cdd.stopPointEnabled = true;
    reachedStopPointPreviously = false;
    debug(unverified, "Latched stoppoint: lba:", stopLba, " frame:", zeroBasedFrameIndexFromLba(stopLba, true) + 1, " 0x07=", hex(inputRegs[0x07]), " 0x08=", hex(inputRegs[0x08]), " 0x09=", hex(inputRegs[0x09]), " 0x0A=", hex(inputRegs[0x0A]), " 0x0B=", hex(inputRegs[0x0B]));

    // As a special case, if we're currently stopped at our own stop point, and a register write has caused
    // the stop point to be latched again, it's immediately hit again here. Note that this is truly a special
    // case, as even if the updated register state indicates a target with a new different location, it will
    // still immediately trip as though we're there already, even though it's the previous location we're at.
    // The character creation screen of Ghost Rush relies on this.
    if (reachedStopPoint) {
      handleStopPointReached(mcd.cdd.io.sector);
    }
  }
}

auto MCD::LD::resetSeekTargetToDefault() -> void {
  //##TODO## Confirm and document this
  currentSeekMode = 0x1;
  currentSeekModeTimeFormat = 0x1;

  seekPointRegs[(int)SeekPointReg::Chapter] = 0x00;
  seekPointRegs[(int)SeekPointReg::HoursOrFrameH] = 0x00;
  seekPointRegs[(int)SeekPointReg::MinutesOrFrameM] = 0x00;
  seekPointRegs[(int)SeekPointReg::SecondsOrFrameL] = 0x00;
  seekPointRegs[(int)SeekPointReg::Frames] = 0x00;
  activeSeekMode = (u8)SeekMode::SeekToRedbookRelativeTime;
}

auto MCD::LD::liveSeekRegistersContainsLatchableTarget() const -> bool {
  // Get the target seek settings based on the live register state
  auto targetSeekMode = inputRegs[0x06].bit(0, 1);
  auto targetSeekModeTimeFormat = inputRegs[0x06].bit(2);

  // If the target seek mode is set to 0 or 3, don't attempt to latch the current seek registers as the new seek target.
  if ((targetSeekMode == 3) || (targetSeekMode == 0)) {
    return false;
  }
  return true;
}

auto MCD::LD::latchSeekTargetFromCurrentState() -> bool {
  // If the live seek registers aren't latchable, abort any further processing.
  if (!liveSeekRegistersContainsLatchableTarget()) {
    return false;
  }

  // Get the target seek settings based on the live register state
  auto targetSeekMode = inputRegs[0x06].bit(0, 1);
  auto targetSeekModeTimeFormat = inputRegs[0x06].bit(2);

  // Update the current seek mode settings
  currentSeekMode = targetSeekMode;
  currentSeekModeTimeFormat = targetSeekModeTimeFormat;

  // Reset the error flags before we do anything further, but only if we haven't hit a stop point. Hardware tests show
  // that the operation error flags are retained here if we're currently stopped at a stop point. Note that new seek
  // settings can still be latched, and seeking can still be performed in these conditions, but the player remains
  // stopped, and the error flags remain in their current state (output reg 0x09 remains at 0x03), unless an invalid
  // seek mode setting is entered, such as an invalid track number in seek mode 1. In this case, the error flags change,
  // and the stop point is released. The player doesn't start playing back immediately, but the next seek operation
  // triggered will resume playback. This has been confirmed through hardware testing. Note that the same behaviour
  // occurs when a picture stop code has been hit. Updating seek registers does clear these flags when the current
  // errors are only asserted from a previous invalid seek operation being attempted though, which shows that internal
  // state is used, not only the live state of these flags.
  if (!reachedStopPoint) {
    operationErrorFlag1 = false;
    operationErrorFlag2 = false;
    operationErrorFlag3 = false;
  }

  // If the player isn't in a valid state to accept seek requests, abort any further processing.
  if (!mcd.cdd.isDiscLoaded() || (currentDriveState <= 0x04) || (targetDriveState != 0x05)) {
    operationErrorFlag1 = true;
    operationErrorFlag2 = false;
    operationErrorFlag3 = true;
    return false;
  }

  //##TODO## Handle peculiarities of invalid BCD value decoding, including differences between LDs and CDs.

  // Seek to track relative time
  if (currentSeekMode == 1) {
    auto targetTrack = inputRegs[0x07];
    auto targetTrackAsInt = (mcd.cdd.isDiscLaserdisc() && (targetTrack == 0x00)) ? (n8)mcd.cdd.getFirstTrack() : (n8)BCD::decode(targetTrack);
    if ((targetTrack > 0x99) || (targetTrackAsInt < mcd.cdd.getFirstTrack()) || (targetTrackAsInt > mcd.cdd.getLastTrack())) {
      operationErrorFlag1 = true;
      operationErrorFlag2 = false;
      operationErrorFlag3 = true;
      reachedStopPoint = false;
      return false;
    }
    bool allowSeekToTime = !mcd.cdd.isDiscLaserdisc() || (!currentSeekModeTimeFormat && mcd.cdd.isLaserdiscDigitalAudioPresent());
    seekPointRegs[(int)SeekPointReg::Chapter] = targetTrack;
    seekPointRegs[(int)SeekPointReg::HoursOrFrameH] = 0x00;
    seekPointRegs[(int)SeekPointReg::MinutesOrFrameM] = (allowSeekToTime ? inputRegs[0x09] : (n8)0x00);
    seekPointRegs[(int)SeekPointReg::SecondsOrFrameL] = (allowSeekToTime ? inputRegs[0x0A] : (n8)0x00);
    seekPointRegs[(int)SeekPointReg::Frames] = (allowSeekToTime ? inputRegs[0x0B] : (n8)0x00);
    activeSeekMode = (u8)SeekMode::SeekToRedbookRelativeTime;
    //debug(unverified, "Latched SeekToRedbookRelativeTime: 0x07=", inputRegs[0x07], " 0x08=", inputRegs[0x08], " 0x09=", inputRegs[0x09], " 0x0A=", inputRegs[0x0A], " 0x0B=", inputRegs[0x0B]);
  }

  // Seek to absolute time
  if (currentSeekMode == 2) {
    //##TODO## Flag an error and don't perform seek if it's past the end of the disc

    // Determine what time format to use. Can be redbook timecode, video timecode, or video frame number. These
    // conditions below should be mutually exclusive.
    bool seekToRedbookTimecode = !mcd.cdd.isDiscLaserdisc() || (!currentSeekModeTimeFormat && mcd.cdd.isDiscLaserdisc());
    bool seekToVideoFrame = mcd.cdd.isDiscLaserdisc() && currentSeekModeTimeFormat && !mcd.cdd.isLaserdiscClv();
    bool seekToVideoTime = mcd.cdd.isDiscLaserdisc() && ((!mcd.cdd.isLaserdiscClv() && !mcd.cdd.isLaserdiscDigitalAudioPresent()) || (mcd.cdd.isLaserdiscClv() && mcd.cdd.isLaserdiscDigitalAudioPresent()));
    if (seekToRedbookTimecode) {
      seekPointRegs[(int)SeekPointReg::Chapter] = 0x00;
      seekPointRegs[(int)SeekPointReg::HoursOrFrameH] = 0x00;
      seekPointRegs[(int)SeekPointReg::MinutesOrFrameM] = inputRegs[0x09];
      seekPointRegs[(int)SeekPointReg::SecondsOrFrameL] = inputRegs[0x0A];
      seekPointRegs[(int)SeekPointReg::Frames] = inputRegs[0x0B];
      activeSeekMode = (u8)SeekMode::SeekToRedbookTime;
      //debug(unverified, "Latched SeekToRedbookTime: 0x07=", inputRegs[0x07], " 0x08=", inputRegs[0x08], " 0x09=", inputRegs[0x09], " 0x0A=", inputRegs[0x0A], " 0x0B=", inputRegs[0x0B]);
    } else if (seekToVideoFrame) {
      seekPointRegs[(int)SeekPointReg::Chapter] = 0x00;
      seekPointRegs[(int)SeekPointReg::HoursOrFrameH] = inputRegs[0x08];
      seekPointRegs[(int)SeekPointReg::MinutesOrFrameM] = inputRegs[0x09];
      seekPointRegs[(int)SeekPointReg::SecondsOrFrameL] = inputRegs[0x0A];
      seekPointRegs[(int)SeekPointReg::Frames] = 0x00;
      activeSeekMode = (u8)SeekMode::SeekToVideoFrame;
      //debug(unverified, "Latched SeekToVideoFrame: 0x07=", inputRegs[0x07], " 0x08=", inputRegs[0x08], " 0x09=", inputRegs[0x09], " 0x0A=", inputRegs[0x0A], " 0x0B=", inputRegs[0x0B]);
    } else if (seekToVideoTime) {
      seekPointRegs[(int)SeekPointReg::Chapter] = 0x00;
      seekPointRegs[(int)SeekPointReg::HoursOrFrameH] = inputRegs[0x08] & 0x0F;
      seekPointRegs[(int)SeekPointReg::MinutesOrFrameM] = inputRegs[0x09];
      seekPointRegs[(int)SeekPointReg::SecondsOrFrameL] = inputRegs[0x0A];
      seekPointRegs[(int)SeekPointReg::Frames] = inputRegs[0x0B];
      activeSeekMode = (u8)SeekMode::SeekToVideoTime;
      //debug(unverified, "Latched SeekToVideoTime: 0x07=", inputRegs[0x07], " 0x08=", inputRegs[0x08], " 0x09=", inputRegs[0x09], " 0x0A=", inputRegs[0x0A], " 0x0B=", inputRegs[0x0B]);
    }
  }
  return true;
}

auto MCD::LD::performSeekWithLatchedState() -> void {
  // Performing a seek operation during frameskip mode clears the latched frame. In single-frame frameskip mode, this
  // causes it to show the frame we end up seeking to. In other modes, it causes the seek target to be shown, and re-bases
  // the skipping sequence from that frame. We emulate that here.
  video.frameSkipBaseFrame = 0;
  video.frameSkipCounter = 0;

  // Perform the latched seek operation
  switch ((SeekMode)activeSeekMode) {
  case SeekMode::SeekToRedbookTime:
    debug(unverified, "SeekToRedbookTime: ", BCD::decode(seekPointRegs[(int)SeekPointReg::MinutesOrFrameM]), ":", BCD::decode(seekPointRegs[(int)SeekPointReg::SecondsOrFrameL]), ":", BCD::decode(seekPointRegs[(int)SeekPointReg::Frames]), " paused=", targetPauseState && !reachedStopPoint);
    mcd.cdd.seekToTime(0, BCD::decode(seekPointRegs[(int)SeekPointReg::MinutesOrFrameM]), BCD::decode(seekPointRegs[(int)SeekPointReg::SecondsOrFrameL]), BCD::decode(seekPointRegs[(int)SeekPointReg::Frames]), targetPauseState && !reachedStopPoint);
    seekPerformedSinceLastFrameUpdate = true;
    break;
  case SeekMode::SeekToRedbookRelativeTime:
    debug(unverified, "SeekToRedbookRelativeTime: ", "Chapter:", BCD::decode(seekPointRegs[(int)SeekPointReg::Chapter]), " ", BCD::decode(seekPointRegs[(int)SeekPointReg::MinutesOrFrameM]), ":", BCD::decode(seekPointRegs[(int)SeekPointReg::SecondsOrFrameL]), ":", BCD::decode(seekPointRegs[(int)SeekPointReg::Frames]), " paused=", targetPauseState && !reachedStopPoint);
    mcd.cdd.seekToRelativeTime(BCD::decode(seekPointRegs[(int)SeekPointReg::Chapter]), BCD::decode(seekPointRegs[(int)SeekPointReg::MinutesOrFrameM]), BCD::decode(seekPointRegs[(int)SeekPointReg::SecondsOrFrameL]), BCD::decode(seekPointRegs[(int)SeekPointReg::Frames]), targetPauseState && !reachedStopPoint);
    seekPerformedSinceLastFrameUpdate = true;
    break;
  case SeekMode::SeekToVideoFrame: {
    //##FIX## This should work on the video stream frame numbers
    s32 frameNumber = ((s32)BCD::decode(seekPointRegs[(int)SeekPointReg::HoursOrFrameH]) * 10000) + ((s32)BCD::decode(seekPointRegs[(int)SeekPointReg::MinutesOrFrameM]) * 100) + (s32)BCD::decode(seekPointRegs[(int)SeekPointReg::SecondsOrFrameL]);
    // Convert the frame number to zero-based, then to an LBA.
    auto lba = lbaFromZeroBasedFrameIndex(std::max(1, frameNumber) - 1);
    debug(unverified, "SeekToVideoFrame: ", frameNumber, " lba:", lba, " paused=", targetPauseState && !reachedStopPoint);
    mcd.cdd.seekToSector(lba, targetPauseState && !reachedStopPoint);
    seekPerformedSinceLastFrameUpdate = true;
    break;}
  case SeekMode::SeekToVideoTime: {
    // Note that hardware tests have shown the LaserActive seems to have a +/- 1 frame accuracy with seeking to CLV
    // timecodes. The player may stop and report back a frame one before or after the target frame number when seeking
    // in still-frame mode.
    u8 videoTimeHours = BCD::decode(seekPointRegs[(int)SeekPointReg::HoursOrFrameH]);
    u8 videoTimeMinutes = BCD::decode(seekPointRegs[(int)SeekPointReg::MinutesOrFrameM]);
    u8 videoTimeSeconds = BCD::decode(seekPointRegs[(int)SeekPointReg::SecondsOrFrameL]);
    u8 videoTimeFrames = BCD::decode(seekPointRegs[(int)SeekPointReg::Frames]);
    debug(unverified, "SeekToVideoTime: ", videoTimeHours, ":", videoTimeMinutes, ":", videoTimeSeconds, ":", videoTimeFrames, " paused=", targetPauseState && !reachedStopPoint);
    VideoTimeToRedbookTime(videoTimeHours, videoTimeMinutes, videoTimeSeconds, videoTimeFrames);
    mcd.cdd.seekToTime(videoTimeHours, videoTimeMinutes, videoTimeSeconds, videoTimeFrames, targetPauseState && !reachedStopPoint);
    seekPerformedSinceLastFrameUpdate = true;
    break; }
  }
}

// Return the current video frame number as a ZERO-BASED index, IE, the first frame is 0.
// Note that this is a bit of a hack. It assumes the digital data tracks start precisely at frame 0. This should be the
// case, but it's technically not guaranteed. In a final implementation, we should be decoding the frame numbers
// straight from the VBI coded data from the currently displayed frame, that way we'll know that we're actually showing
// the exact intended frames. This functioned to get us started though.
auto MCD::LD::zeroBasedFrameIndexFromLba(s32 lba, bool processLeadIn) -> s32 {
  // If we're in the lead-in and not asked to handle lead-in values, return 0.
  if (!processLeadIn && (lba < 0)) {
    return 0;
  }

  // Turn the lba sector number into a frame number. Since there are 30 frames of video per second, and 75 sectors of CD
  // data per second, this will work well enough.
  auto frameIndex = (s32)std::round(((double)lba / 75.0) * videoFramesPerSecond);
  frameIndex = (processLeadIn && (lba < 0)) ? (-frameIndex) - 1 : frameIndex;
  return frameIndex;
}

// Convert the ZERO-BASED frame number to an LBA.
auto MCD::LD::lbaFromZeroBasedFrameIndex(s32 frameIndex) -> s32 {
  auto lba = (s32)std::round(((double)frameIndex / videoFramesPerSecond) * 75.0);
  return lba;
}

auto MCD::LD::VideoTimeToRedbookTime(u8& hours, u8& minutes, u8& seconds, u8& frames) -> void {
  auto videoTimeTotalFrames = ((((((s32)hours * 60) + (s32)minutes) * 60) + (s32)seconds) * 30) + (s32)frames;
  auto redbookTimeTotalFrames = (s32)std::round(((double)videoTimeTotalFrames / videoFramesPerSecond) * 75.0);
  frames = redbookTimeTotalFrames % 75;
  redbookTimeTotalFrames /= 75;
  seconds = redbookTimeTotalFrames % 60;
  redbookTimeTotalFrames /= 60;
  minutes = redbookTimeTotalFrames % 60;
  redbookTimeTotalFrames /= 60;
  hours = redbookTimeTotalFrames;
}

auto MCD::LD::handleStopPointReached(s32 lba) -> void {
  // Hardware tests have shown that as a special case, stop points are ignored in still-frame stepping mode. The stop
  // points will remain active and not be latched in this case. Note that this is only if the "true" playback mode is
  // set to this, not when a stop point has been hit and the output registers claim we're in still-frame stepping mode.
  if ((currentPlaybackMode == 2) && (currentPlaybackSpeed <= 1)) {
    return;
  }

  // Trigger the stop point, pausing playback or triggering repeat mode as necessary. Note that hardware tests have
  // shown that input reg 0x6 bit 7 is indeed used live - it isn't latched at the time of the last seek operation, or
  // at the point when the stop point is set.
  if (inputRegs[0x06].bit(7)) {
    // Repeat mode
    // Note that repeating isn't instant on the hardware, there's a small seek time which depends on the distance to the
    // loop start point, and at the beginning of that process, the stop point frame is visible on screen. With a target
    // seek point 9 frames backwards, we measured around 130ms of time in which the stop frame appears on screen,
    // followed by the frame we seeked to being visible for around an extra 100ms in addition to its intended display
    // time. This is basically hidden at step speed of 0x4 and slower (heavily used by Myst), and not very noticeable at
    // step speed 0x03. Faster and realtime modes would be noticeable if a continuous animation was attempted, however if
    // slight pauses of motion for 250ms or more are acceptable in a looping animation, the seek time can be hidden.
    // When increasing to 100 frames looping, the 130ms time holding the stop point roughly doubled to around 250ms,
    // while the 100ms at the end of the seek remained the same. Increasing to 1000 frames looping roughly doubled again
    // to around 500ms stop frame hold time.
    //##TODO## There are some initial seek latency numbers in the CDD section to handle LaserActive seek latency, but
    //more precise measurements should be taken to tune this more accurately.
    debug(unverified, "Hit stoppoint - repeat mode: lba:", lba, " frame:", zeroBasedFrameIndexFromLba(lba, true) + 1);
    //##TODO## Test if the 0x1F output reg gets re-driven here to 0x01
    reachedStopPoint = false;
    mcd.cdd.io.status = CDD::Status::Playing;
    performSeekWithLatchedState();
  } else {
    // Stop mode
    debug(unverified, "Hit stoppoint - stop mode: lba:", lba, " frame:", zeroBasedFrameIndexFromLba(lba, true) + 1);
    outputRegs[0x1A] = 0xFF;
    outputRegs[0x1B] = 0xFF;
    outputRegs[0x1C] = 0xFF;
    outputRegs[0x1D] = 0xFF;
    outputRegs[0x1E] = 0xFF;
    outputRegs[0x1F] = 0x00;
    reachedStopPointPreviously = true;
    operationErrorFlag1 = false;
    operationErrorFlag2 = true;
    operationErrorFlag3 = true;
    reachedStopPoint = true;
    mcd.cdd.io.status = CDD::Status::Paused;
    mcd.cdd.stopPointEnabled = false;
  }
}

auto MCD::LD::updateCurrentVideoFrameNumber(s32 lba) -> void {
  // Detect and clear the state on whether we've displayed the first frame after a seek operation. This is to handle
  // frameskip mode base frame latching behaviour. The way this works is that, if a seek is performed at the same time
  // as frameskip mode is entered, or even if there's a pending seek operation still in progress, the first frame of
  // that seek operation is always displayed first, then on the subsequent frame, that's when frameskip mode will latch
  // the base frame.
  bool justCompletedSeekFirstFrame = seekPerformedSinceLastFrameUpdate;
  seekPerformedSinceLastFrameUpdate = false;

  // Calculate the new video frame index
  auto newVideoFrameIndex = zeroBasedFrameIndexFromLba(lba, true);
  bool newVideoFrameLeadIn = (lba < 0);
  bool newVideoFrameLeadOut = (newVideoFrameIndex >= video.activeVideoFrameCount);
  if (newVideoFrameLeadOut) {
    newVideoFrameIndex = newVideoFrameIndex - video.activeVideoFrameCount;
  }

  // Limit the new video frame index to the set of video data that's available
  if (newVideoFrameLeadIn) {
    newVideoFrameIndex = (newVideoFrameIndex >= video.leadInFrameCount) ? (video.leadInFrameCount > 0 ? video.leadInFrameCount - 1 : 0) : newVideoFrameIndex;
  } else if (newVideoFrameLeadOut) {
    newVideoFrameIndex = (newVideoFrameIndex >= video.leadOutFrameCount) ? (video.leadOutFrameCount > 0 ? video.leadOutFrameCount - 1 : 0) : newVideoFrameIndex;
  }

  // If the video frame index hasn't changed, abort any further processing, unless the player is actively looping on the
  // one frame. We need the exception to handle cases where games switch from one field to another on the same frame.
  // Rocket Coaster does this in its main menu when transitioning to the vehicle selection screen.
  bool playerIsLoopingOnSingleFrame = (currentDriveState = 0x05) && !currentPauseState && (currentPlaybackMode == 0x2) && (currentPlaybackSpeed == 0x0);
  bool newFrameIsSameAsLastFrame = (newVideoFrameIndex == video.currentVideoFrameIndex) && (newVideoFrameLeadIn == video.currentVideoFrameLeadIn) && (newVideoFrameLeadOut == video.currentVideoFrameLeadOut);
  if (!playerIsLoopingOnSingleFrame && newFrameIsSameAsLastFrame) {
    return;
  }

  // Handle frameskip mode. Note that we do this processing before we handle the image hold bit, as we need to handle
  // our base frame latching even if the image hold bit is set. Myst 02-110 relies on this for the open Myst book intro
  // when selecting to start a new game.
  if (!justCompletedSeekFirstFrame && (currentPlaybackMode == 0x01)) {
    // Decode the current target frameskip counter
    s32 newFrameSkipCounter = 0;
    switch (currentPlaybackSpeed) {
    case 0x00:
      //-0x0: 0 frames. This pauses playback in frame stepping mode, and performs a normal playback in frame skipping mode.
      newFrameSkipCounter = 1;
      break;
    case 0x01:
      //-0x1: 1 frame only. No further updates will appear. Since we're latching the frame here, we also need to invert
      // the current playback direction bit, so that the output register state is correct. The hardware inverts this bit
      // once the frame has been latched.
      newFrameSkipCounter = -1;
      currentPlaybackDirection = !currentPlaybackDirection;
      break;
    case 0x02:
      //-0x2: 15 FPS instead of 30 (12 seconds for 180 frames) - Display frame 2x
      newFrameSkipCounter = 2;
      break;
    case 0x03:
      //-0x3: 7.5 FPS instead of 30 (24 seconds for 180 frames) - Display frame 4x
      newFrameSkipCounter = 4;
      break;
    case 0x04:
      //-0x4: 3.75 FPS instead of 30 (24 seconds for 90 frames) - Display frame 8x
      newFrameSkipCounter = 8;
      break;
    case 0x05:
      //-0x5: 1.875 FPS instead of 30 (48 seconds for 90 frames) - Display frame 16x
      newFrameSkipCounter = 16;
      break;
    case 0x06:
      //-0x6: 1 FPS instead of 30 (45 seconds for 45 frames) - Display frame 30x
      newFrameSkipCounter = 30;
      break;
    case 0x07:
      //-0x7: ~0.33r FPS instead of 30 (30 seconds for 10 frames) - Display frame 90x
      newFrameSkipCounter = 90;
      break;
    }

    // Perform frameskip, aborting the following update of the current video frame if required.
    if ((video.frameSkipBaseFrame == 0) || (video.frameSkipCounter != newFrameSkipCounter)) {
      // When transitioning to frameskip mode, the way it works is that the next frame to be shown after frameskip is
      // activated is latched as the base frame, and shown on the screen. After that, a skip counter will be running
      // which will not show subsequent frames until it expires, at which point it will be reloaded again. IE, if input
      // reg 0x03 is currently 0x20, with frame number 300 shown, and input reg 0x03 is changed to 0x20, frame number 301
      // will be shown, then the next frame to be shown will be frame number 391 (3 seconds later). Note that the base
      // frame number is reset when the frame skip rate is changed too, however it is NOT reset when input reg 0x03 bit 3
      // is changed to modify the direction, which is ignored in frameskip mode.
      video.frameSkipBaseFrame = newVideoFrameIndex;
      video.frameSkipCounter = newFrameSkipCounter;
    } else {
      int frameDeltaFromBase = std::abs((int)newVideoFrameIndex - (int)video.frameSkipBaseFrame);
      if ((frameDeltaFromBase % video.frameSkipCounter) != 0) {
        // This frame is being skipped, so we don't need to update the current frame info. Abort any further processing.
        return;
      }
    }
  } else if ((currentPlaybackMode == 0x02) && (currentPlaybackSpeed == 0x01)) {
    // When a single frame frame-step is processed, and a new frame is now being shown, clear the playback speed so that
    // we stop advancing. From hardware testing, this is the effect seen in the output registers, and nothing seems to
    // invalidate this being the way the player knows the step operation has been processed. We have already detected
    // above that a legitimate frame advance has occurred for us to reach this point.
    currentPlaybackSpeed = 0;
  } else {
    video.frameSkipBaseFrame = 0;
    video.frameSkipCounter = 0;
  }

  // We only update the displayed video frame if the image hold bit isn't set. If it is, abort any further processing.
  // Hardware tests have shown this really does retain the last image in the buffer. Changes to field selection, or
  // starting playback at a new location, do not cause any change in the displayed image.
  // -Note some important interaction with the player paused state, as from setting bit 4 of input reg 0x02. Putting the
  // player into a pause state blanks the display, which is handled dynamically in the line output function, however,
  // interestingly setting the image hold bit as well while in a paused state causes the previous frame in the buffer to
  // be shown, even if the pause bit is set. This is because testing has shown that setting the pause mode flag turns
  // off the digital memory buffer (light goes off on the front of the unit, even when input reg 0x0C bit 1 is set),
  // while turning on the image hold bit re-enables the buffer, and the image appears again. Setting input reg 0x0C bit
  // 4 doesn't have this effect, and the digital memory light stays off with the blank image. Only setting the hold
  // bit (input reg 0x0C bit 5) has the effect of overriding blanking caused by the player pause state.
  // -We can seek while the player is paused and image hold is on, and the latched frame will not update. If we flick
  // image hold off and on again after a seek however, the frame at the new location will be latched, and the updated
  // frame will be shown when we turn the image hold bit back on again. This shows that we do need to latch new frames
  // when the player is paused, even if the output is normally blanked in pause mode.
  // -Note that there is additional interaction with input reg 0x0C bit 2, the video disable flag. If image hold is off
  // when the video is disabled, then image hold is turned on, with video disable still active, a new frame cannot be
  // latched yet. If we then perform a seek for example, then turn off the video disable bit, the next frame to be shown
  // will be latched and held. This shows us that while image hold can latch a new frame during pause, it cannot latch a
  // new frame when the video disable bit is set, and remains in a "pending" state until the next frame arrives. Note
  // that Triad Stone relies on this to get the title screen to appear when a button is pressed to skip the intro video.
  // It writes 0x94, 0xBC, 0xBF, 0x9F, 0xBF, 0xBB to this register in sequence, performing a seek operation just before
  // the write of 0xBB. It expects a new frame will be latched when 0xBB is written. The act of writing 0x9F to turn off
  // image hold while the video disable bit is set, then turning image hold back on while video disable is still set, is
  // enough to cause the frame to be latched when the video disable bit is later cleared by the 0xBB write.
  // -Note that merely toggling the video disable bit on and off doesn't clear the image held in the digital memory if
  // image hold is still on. If image hold is turned off while video disable is set however, that will cause the digital
  // memory to be cleared.
  // -Note that toggling digital memory off then back on doesn't clear the held image either.
  // -Note that enabling image hold when no playback is occurring, but a previously valid frame is still latched in the
  // digital framebuffer, will cause that previous image to be latched and used. Vajra relies on this when starting a
  // new level to not show the "Pioneer" video data during loading when the image of the boss you're going to fight is
  // on the screen.
  //##TODO## Determine how this interacts with picture stop codes once we go to implement them
  //##TODO## Confirm if disabling digital memory in the various different ways with image hold off causes the current
  //frame to be cleared from memory, and trigger a new latch again on the next frame after being re-enabled, like
  //happens with the video disable flag.
  if ((inputRegs[0x0C].bit(5) && video.digitalMemoryFrameLatched) || inputRegs[0x0C].bit(2)) {
    // If the video stream has been disabled, and image hold is off, any currently latched frame in the memory buffer is
    // cleared.
    if (inputRegs[0x0C].bit(2) && !inputRegs[0x0C].bit(5)) {
      video.digitalMemoryFrameLatched = false;
    }
    return;
  }

  // Determine whether the digital memory buffer is active, and if so, which field to latch in the buffer. Hardware
  // tests have shown bits 1 or 3 of input reg 0x0C make field selection active, as long as the analog mixing mode
  // allows it.
  int buildIndex = video.drawIndex ^ 0x01;
  auto analogMixingMode = inputRegs[0x01].bit(7, 6);
  bool newFieldSelectionEnabledState = (analogMixingMode >= 2) && (inputRegs[0x0C].bit(1) || inputRegs[0x0C].bit(3));
  bool newFieldSelectionEvenField = (newFieldSelectionEnabledState ? inputRegs[0x0C].bit(0) : false);
  bool fieldSelectionChanged = false;
  if ((newFieldSelectionEnabledState != video.currentVideoFrameFieldSelectionEnabled[buildIndex]) || (newFieldSelectionEvenField != video.currentVideoFrameFieldSelectionEvenField[buildIndex])) {
    fieldSelectionChanged = true;
    video.currentVideoFrameFieldSelectionEnabled[buildIndex] = newFieldSelectionEnabledState;
    video.currentVideoFrameFieldSelectionEvenField[buildIndex] = newFieldSelectionEvenField;
  }

  // Mark a frame as latched in the digital memory buffer
  video.digitalMemoryFrameLatched = true;

  // At the end of all the above state updates, if we're still showing the same frame (but not necessarily the same
  // field), abort any further processing, since the correct full frame is already latched.
  if (!fieldSelectionChanged && newFrameIsSameAsLastFrame) {
    return;
  }

  // Update the current video frame index
  video.currentVideoFrameIndex = newVideoFrameIndex;
  video.currentVideoFrameLeadIn = newVideoFrameLeadIn;
  video.currentVideoFrameLeadOut = newVideoFrameLeadOut;

  // If the video disable bit is set, clear the buffer, otherwise load the displayed video frame into the buffer.
  loadCurrentVideoFrameIntoBuffer();

  //##TODO## Implement picture stop codes. This is where we should check the lines in the VBI region and stop the player
  //if a picture stop code is detected, unless picture stop cancel is active (Input reg 0x0C bit 7).
  //##TODO## The way picture stop codes work, is when they're encountered, currentPlaybackMode goes to 0x02, and
  //currentPlaybackSpeed goes to 0x00, as reported by output register 0x07, IE, the player switches into still-frame
  //mode. Additionally, operationErrorFlag3 at output reg 0x09 bit 0 gets set to true when this is hit. This is
  //basically what happens when stop points get hit, except that operationErrorFlag2 is set to false, not true.
  //When we implement picture stop codes, we need to compare behaviour with stop points more closely.
  //##TODO## Check if the same playback mode exceptions to step through picture stop codes apply as per stop points. It
  //seems they must, or we probably couldn't step through it on the player, but do tests to confirm.
  if (!video.isCLV && !inputRegs[0x0C].bit(7) && ((currentPlaybackMode != 2) || (currentPlaybackSpeed > 1))) {
    // As per IEC 60857:1986, picture stop codes are encoded into lines 16 & 17 of the frame for the even field, and
    // lines 279 and 280 of the frame for the odd field. We try and take the first one from each, falling back to the
    // second line if decoding fails.
    auto biphaseCodeEvenField = decodeBiphaseCodeFromScanline(16-1);
    if (biphaseCodeEvenField == 0) {
      biphaseCodeEvenField = decodeBiphaseCodeFromScanline(17-1);
    }
    auto biphaseCodeOddField = decodeBiphaseCodeFromScanline(279-1);
    if (biphaseCodeOddField == 0) {
      biphaseCodeOddField = decodeBiphaseCodeFromScanline(280-1);
    }

    // If we have a picture stop code detected, process it now.
    if ((biphaseCodeEvenField == 0x82CFFF) || (biphaseCodeOddField == 0x82CFFF)) {
      //##TODO## Shouldn't we clear the build framebuffer here to stop the display buffer rolling over to this one? As
      //per the spec, the picture stop code is applied on the field AFTER the one you want the player to stop at.
      //##TODO## Can we stop at the second field in a frame? IE, hold on the first field? This doesn't seem like it
      //would be very sensible/logical. The player has to loop on the one frame surely? Maybe it stops before the frame
      //if it's on the first field, and on the frame if it's on the second field? Check some discs and see if we can
      //find a real world case to test behaviour.
      //##TODO## Confirm the exact frame the real player stops on when encountering picture stop codes.
      reachedStopPointPreviously = true;
      operationErrorFlag1 = false;
      operationErrorFlag2 = false;
      operationErrorFlag3 = true;
      reachedStopPoint = true;
      mcd.cdd.io.status = CDD::Status::Paused;
      //##TODO## Check on the hardware if a stop point is cleared when a picture stop code is triggered
      mcd.cdd.stopPointEnabled = false;
    }
  }
}

auto MCD::LD::loadCurrentVideoFrameIntoBuffer() -> void {
  // Locate the new video frame in the source file
  const unsigned char* videoFrameCompressed = nullptr;
  if (video.currentVideoFrameLeadIn) {
    videoFrameCompressed = (video.currentVideoFrameIndex < video.leadInFrameCount ? video.leadInFrames[video.currentVideoFrameIndex] : nullptr);
  } else if (video.currentVideoFrameLeadOut) {
    videoFrameCompressed = (video.currentVideoFrameIndex < video.leadOutFrameCount ? video.leadOutFrames[video.currentVideoFrameIndex] : nullptr);
  } else {
    videoFrameCompressed = (video.currentVideoFrameIndex < video.activeVideoFrameCount ? video.activeVideoFrames[video.currentVideoFrameIndex] : nullptr);
  }
  if (videoFrameCompressed == nullptr) {
    return;
  }

  // Allocate memory for the video frame buffer if it's currently empty
  int buildIndex = video.drawIndex ^ 0x01;
  auto& buildFrameBuffer = video.videoFrameBuffers[buildIndex];
  if (buildFrameBuffer.empty()) {
    buildFrameBuffer.resize(video.videoFrameHeader.width * video.videoFrameHeader.height * 3);
  }

  // If we were prefetching the wrong frame, or if no prefetch was active, load the required frame immediately here into
  // the build framebuffer.
  bool loadedDirectly = false;
  if ((videoFramePrefetchTarget == nullptr) || (videoFramePrefetchTarget != videoFrameCompressed)) {
    size_t frameSizeCompressed = qon_decode_frame_size(videoFrameCompressed);
    qoi2_decode_data(videoFrameCompressed + QON_FRAME_SIZE_SIZE, frameSizeCompressed, &video.videoFrameHeader, nullptr, buildFrameBuffer.data(), 3);
    loadedDirectly = true;
  }

  // Wait for any pending frame prefetch request to be latched, and the prefetch operation to be completed, even if we
  // were prefetching the wrong frame. We need to make sure any pending prefetch request has been latched by the worker
  // thread, so we can queue our next prefetch operation below. We also need to wait for the previous prefetch operation
  // to complete, even if we were prefetching the wrong frame and loaded the correct one directly above, as while we can
  // theoretically queue another prefetch operation after the first one has been latched, the worker thread will mark
  // the prefetch operation as complete when the original prefetch request is processed, leaving it marked as complete
  // when it starts the new one, as it is our responsibility to clear the prefetch complete state. This means if we
  // don't wait for the original prefetch to complete here, it would trigger a race condition for the load of the
  // following frame.
  videoFramePrefetchPending.wait(true);
  if (videoFramePrefetchTarget != nullptr) {
    videoFramePrefetchComplete.wait(false);
    videoFramePrefetchComplete.clear();
  }

  // If the prefetch operation is for the correct frame, we've just waited for it to complete above, so we now swap the
  // prefetch buffer with the build frame buffer. Note that this will exchange memory buffer pointers and not copy the
  // contents themselves, so it's a quick constant-time operation.
  if (!loadedDirectly) {
    buildFrameBuffer.swap(videoFramePrefetchBuffer);
  }

  // Calculate the likely next frame to be shown, assuming no changes to playback state.
  s32 newVideoFrameIndex;
  bool newVideoFrameLeadIn = false;
  if (currentPlaybackMode != 0x03) {
    newVideoFrameIndex = video.currentVideoFrameIndex + 1;
  } else {
    // We're in fast forward mode. Take playback speed and direction into account to calculate the next sector that will
    // be requested.
    i32 sectorAdvanceOffset = 1;
    switch (currentPlaybackSpeed) {
    case 0x01:
      sectorAdvanceOffset = 2;
      break;
    case 0x02:
      sectorAdvanceOffset = 3;
      break;
    case 0x03:
      sectorAdvanceOffset = 8;
      break;
    case 0x04:
      sectorAdvanceOffset = 14;
      break;
    case 0x05:
      sectorAdvanceOffset = 20;
      break;
    }
    sectorAdvanceOffset = (currentPlaybackDirection ? (i32)(-sectorAdvanceOffset) : sectorAdvanceOffset);

    // Calculate the next likely video frame index, based on the current fast forward settings.
    auto lba = mcd.cdd.io.sector + sectorAdvanceOffset;
    newVideoFrameIndex = zeroBasedFrameIndexFromLba(lba, true);
    newVideoFrameLeadIn = (lba < 0);
    if (newVideoFrameIndex == video.currentVideoFrameIndex) {
      newVideoFrameIndex += (newVideoFrameIndex == 0 ? 0 : (currentPlaybackDirection ? -1 : 1));
    }
  }
  bool newVideoFrameLeadOut = (newVideoFrameIndex >= video.activeVideoFrameCount);
  if (newVideoFrameLeadOut) {
    newVideoFrameIndex = newVideoFrameIndex - video.activeVideoFrameCount;
  }

  // Queue a prefetch for the likely next video frame. Note that if we prefetched the wrong frame last time, it's
  // possible that a prefetch operation is still being performed. This is ok, as we can queue up another prefetch
  // operation here once the original request has been latched, which we've already tested for above.
  if (newVideoFrameLeadIn) {
    videoFrameCompressed = (newVideoFrameIndex < video.leadInFrameCount ? video.leadInFrames[newVideoFrameIndex] : nullptr);
  } else if (newVideoFrameLeadOut) {
    videoFrameCompressed = (newVideoFrameIndex < video.leadOutFrameCount ? video.leadOutFrames[newVideoFrameIndex] : nullptr);
  } else {
    videoFrameCompressed = (newVideoFrameIndex < video.activeVideoFrameCount ? video.activeVideoFrames[newVideoFrameIndex] : nullptr);
  }
  if (videoFrameCompressed == nullptr) {
    return;
  }
  videoFramePrefetchTarget = videoFrameCompressed;
  videoFramePrefetchPending.test_and_set();
  videoFramePrefetchPending.notify_all();
}

auto MCD::LD::videoFramePrefetchThread() -> void {
  // Trigger a notification that this worker thread has started
  videoFramePrefetchThreadStarted.test_and_set();
  videoFramePrefetchThreadStarted.notify_all();

  // Perform prefetch requests as they arrive, and terminate the thread when requested.
  while (!videoFramePrefetchThreadShutdownRequested.test()) {
    // Wait for a prefetch request to arrive
    videoFramePrefetchPending.wait(false);

    // If this thread has been requested to terminate, break out of the prefetch loop.
    if (videoFramePrefetchThreadShutdownRequested.test()) {
      break;
    }

    // Trigger a notification that a prefetch request is no longer pending
    videoFramePrefetchPending.clear();
    videoFramePrefetchPending.notify_all();

    // Allocate memory for the prefetch frame buffer if it's currently empty
    if (videoFramePrefetchBuffer.empty()) {
      videoFramePrefetchBuffer.resize(video.videoFrameHeader.width * video.videoFrameHeader.height * 3);
    }

    // Decode the QOI2 compressed video frame into the buffer
    size_t frameSizeCompressed = qon_decode_frame_size(videoFramePrefetchTarget);
    qoi2_decode_data(videoFramePrefetchTarget + QON_FRAME_SIZE_SIZE, frameSizeCompressed, &video.videoFrameHeader, nullptr, videoFramePrefetchBuffer.data(), 3);

    // Trigger a notification that the prefetch operation is complete
    videoFramePrefetchComplete.test_and_set();
    videoFramePrefetchComplete.notify_all();
  }

  // Trigger a notification that this worker thread has shut down
  videoFramePrefetchThreadShutdownComplete.test_and_set();
  videoFramePrefetchThreadShutdownComplete.notify_all();
}

auto MCD::LD::decodeBiphaseCodeFromScanline(int lineNo) -> u32 {
  // Retrieve the frame currently in the build framebuffer
  int buildIndex = video.drawIndex ^ 0x01;
  auto& buildFrameBuffer = video.videoFrameBuffers[buildIndex];
  if (buildFrameBuffer.empty()) {
    return 0;
  }

  // Attempt to locate the start of the 24-bit biphase coded signal. As per IEC 60857:1986, the first bit is the "key"
  // bit, and will always be logical level 1. This is represented by a transition from IRE 0 to IRE 100 at the mid-point
  // of the "bit cell". We attempt to locate the transition in key bit here, indicating the mid-point of the bit cell at
  // the start of the biphase code. Since the exact offset of this video data is allowed to, and does, vary in its exact
  // alignment vs the horizontal blanking region, we do need to search for it, using fixed pixel offsets is not
  // sufficient. We can be sure however that it occurs after horizontal blanking and color burst. Giving ourself a
  // healthy margin, we start scanning from roughly 15% of the way into the horizontal line. We'll continue to scan up
  // to 25%. We look for a string of consecutive pixels a quarter of the bit cell length in width, with all colour
  // channels set to 0x80 or above. We use a quarter of the bit cell length, as per IEC 60857:1986, logic level
  // transitions may only persist half the bit cell length, and we have rise and fall times that mean we won't get a
  // perfect half bit cell length with a clear value, so we look for a quarter length period with a clear value. If we
  // don't locate the key bit in this region of video, it's not there, or the video is completely out of spec, rendering
  // the codes unreadable in practice anyway.
  size_t linePos = video.videoFrameHeader.width * lineNo * 3;
  size_t lineWidth = video.videoFrameHeader.width;
  size_t searchCellLengthInPixels = (size_t)(video.vbiDataBitCellLengthInPixels / 4.0);
  bool foundStartPos = false;
  size_t biphaseCodeStartPixelNo = video.vbiDataSearchStartPos;
  while (!foundStartPos && (biphaseCodeStartPixelNo < video.vbiDataSearchEndPos)) {
    bool cellVerificationFailed = false;
    for (size_t i = 0; i < searchCellLengthInPixels; ++i) {
      size_t checkPixelPos = linePos + ((biphaseCodeStartPixelNo + i) * 3);
      if ((buildFrameBuffer[checkPixelPos + 0] < 0x80) || (buildFrameBuffer[checkPixelPos + 1] < 0x80) || (buildFrameBuffer[checkPixelPos + 2] < 0x80)) {
        cellVerificationFailed = true;
        break;
      }
    }
    if (!cellVerificationFailed) {
      foundStartPos = true;
      break;
    }
    ++biphaseCodeStartPixelNo;
  }
  if (!foundStartPos) {
    return 0;
  }

  // Since we found the mid-point of the first bit cell, back off half a cell width to align correctly with the true
  // start of the biphase code.
  biphaseCodeStartPixelNo -= (size_t)(video.vbiDataBitCellLengthInPixels / 2.0);

  // Read out the biphase code from the target line
  u32 biphaseCode = 0;
  for (size_t i = 0; i < video.vbiDataBitSampleOffsets.size(); i += 2) {
    size_t leadingSamplePixelPos = linePos + ((biphaseCodeStartPixelNo + video.vbiDataBitSampleOffsets[i+0]) * 3);
    size_t trailingSamplePixelPos = linePos + ((biphaseCodeStartPixelNo + video.vbiDataBitSampleOffsets[i+1]) * 3);
    bool leadingSampleValue = (buildFrameBuffer[leadingSamplePixelPos + 0] >= 0xC0) && (buildFrameBuffer[leadingSamplePixelPos + 1] >= 0xC0) && (buildFrameBuffer[leadingSamplePixelPos + 2] >= 0xC0);
    bool trailingSampleValue = (buildFrameBuffer[trailingSamplePixelPos + 0] >= 0xC0) && (buildFrameBuffer[trailingSamplePixelPos + 1] >= 0xC0) && (buildFrameBuffer[trailingSamplePixelPos + 2] >= 0xC0);

    // If no transition was found, the biphase data is corrupted, so we return 0.
    if (leadingSampleValue == trailingSampleValue) {
      return 0;
    }

    // Add this bit value to the decoded biphase code
    biphaseCode = (biphaseCode << 1) | (trailingSampleValue ? 1 : 0);
  }
  return biphaseCode;
}

auto MCD::LD::power(bool reset) -> void {
  // Note we currently rely on our reset call happening after the cdd reset to get this to work
  mcd.cdd.hostClockEnable = true;

  // Zero all our registers
  for (auto& data : inputRegs) data = 0x0;
  for (auto& data : inputFrozenRegs) data = 0x0;
  for (auto& data : outputRegs) data = 0x0;
  for (auto& data : outputFrozenRegs) data = 0x0;
  for (auto& data : outputRegsWrittenCooldownTimer) data = 0x0;
  areInputRegsFrozen = false;
  areOutputRegsFrozen = false;
  operationErrorFlag1 = false;
  operationErrorFlag2 = false;
  operationErrorFlag3 = false;
  seekEnabled = false;
  currentSeekMode = 0x0;
  currentSeekModeTimeFormat = 0x0;
  currentSeekModeRepeat = false;
  for (auto& data : stopPointRegs) data = 0x0;
  reachedStopPoint = false;
  reachedStopPointPreviously = false;
  activeSeekMode = 0x0;
  currentPlaybackMode = 0x0;
  currentPlaybackSpeed = 0x0;
  currentPlaybackDirection = 0;
  for (auto& data : seekPointRegs) data = 0x0;
  targetDriveState = 0x0;
  currentDriveState = 0x0;
  targetPauseState = false;
  currentPauseState = false;
  seekPerformedSinceLastFrameUpdate = false;
  driveStateChangeDelayCounter = 0x0;
  selectedTrackInfo = 0x0;

  // Set the few registers that start with initial values
  currentDriveState = 0x02; // 0x02 = Drive door closed
  targetDriveState = currentDriveState;
  inputRegs[0x1A] = 0xFF;
  inputRegs[0x1B] = 0xFF;
  inputRegs[0x1C] = 0xFF;
  inputRegs[0x1D] = 0xFF;

  // Clear any currently latched video frame
  video.currentVideoFrameIndex = -99999999;
  video.drawIndex = 0;
  video.videoFrameBuffers[0].clear();
  video.videoFrameBuffers[1].clear();
  video.currentVideoFrameBlanked = false;
  video.digitalMemoryFrameLatched = false;
}

auto MCD::LD::scanline(u32 vdpPixelBuffer[1495], u32 vcounter) -> void {
  // Convert the VDP vcounter to a linear scanline index. We only have to worry about NTSC V28 mode, as that's all the
  // LaserActive supported. If someone wanted to make a hypothetical PAL version of the system, they'd have to mess with
  // all the numbers in this function to handle alignment and composition with PAL line counts and border sizes. We
  // start line 0 here from the beginning of the bottom blanking region, which aligns with our raw frame data.
  u32 targetScanLine = 0;
  targetScanLine = (vcounter >= 0x1E5 ? (vcounter - 0x1E5) + 3 : (vcounter >= 0x0E8 ? vcounter - 0x0E8 : vcounter + (0x200 - 0x1E5) + 3));

  // Calculate offsets/margins for VDP video stream
  size_t vdpActiveRegionLeftOffset = 0;
  size_t vdpActiveRegionTopOffset = 0;
  if (!vdp.screen->overscan()) {
    // Omit 19 lines of leading video unless overscan is active. You may expect it would be 30 as follows:
    // -Bottom blanking:   3
    // -Vertical sync:     3
    // -Top blanking:      13
    // -Top border:        11
    // However "VDP::frame()" currently expects the border to be drawn, and offsets the viewport by 11 pixels
    // vertically. We therefore remove the 11-pixel top border, giving us 19 lines.
    vdpActiveRegionTopOffset = (3 + 3  + 13 + 11) - 11;
    // Omit the 13 pixel VDP left border. This is 13 pixels in all modes, but we need to account for H32/H40 mode
    // affecting the pixel "width" here.
    vdpActiveRegionLeftOffset = (vdp.h32() ? (13 * 5) : (13 * 4));
  } else {
    vdpActiveRegionLeftOffset = 0;
    vdpActiveRegionTopOffset = 0;
  }
  // VDP horizontal lines are currently "trimmed" by the VDP core, and don't contain the full 1710 dots we'd expect if
  // we were generating pixels at an effective MCLK/2 rate (5 pixels in H32, 4 pixels in H40), instead there are 1495
  // dots, leaving 215 dots unrepresented, and the lines are fairly arbitrarily centered. We account for the offset here
  // in the input data from the VDP, and we also have to apply it on the way out again so that the modified lines match
  // alignment with unmodified lines.
  size_t vdpBorderLeftOffset = (vdp.h32() ? (8 * 5) : (17 * 4));

  // If we're at the start of a new frame, handle blanking, swapping the video buffers, even/odd field selection for
  // interlace mode.
  if (targetScanLine == 0) {
    // Determine if we're currently outputting an interlaced image
    //##FIX## Actually output interlaced images in 480i mode here, not just as 240p like we do currently.
    video.currentVideoFrameInterlaced = !inputRegs[0x0C].bit(5) && !inputRegs[0x0C].bit(3) && !inputRegs[0x0C].bit(1);

    // Advance the pending frame into the current frame buffer
    int buildIndex = video.drawIndex ^ 0x01;
    if (!video.videoFrameBuffers[buildIndex].empty()) {
      // Note that we're relying on the characteristic here of std::vector (guaranteed in the standard) that clear()
      // doesn't actually release the memory (IE, doesn't change capacity), it just sets the number of valid elements to
      // zero. This means we don't incur heap allocation penalties from clearing and resizing each frame. The buffer
      // will size itself once, then we're just changing the valid element count to indicate whether it contains a frame
      // or not, and zeroing out the buffer.
      video.videoFrameBuffers[video.drawIndex].clear();
      video.currentVideoFrameFieldSelectionEnabled[video.drawIndex] = video.currentVideoFrameFieldSelectionEnabled[buildIndex];
      video.currentVideoFrameFieldSelectionEvenField[video.drawIndex] = video.currentVideoFrameFieldSelectionEvenField[buildIndex];
      video.drawIndex = buildIndex;

      // Reset the odd/even field selection to the odd field when advancing to a new frame
      video.currentVideoFrameOnEvenField = false;
    } else if (video.currentVideoFrameInterlaced) {
      // Invert the even/odd frame if we're not advancing to a new frame, and an interlaced output is enabled. Yes, this
      // does mean it's impossible to get 60hz interlaced output with digital memory enabled, which also means it's
      // impossible to get a stable mixed VDP/analog video output at 240p. It's possible this works with the Sega PAC
      // hardware at least with the interlaced VDP output mode, however this would need to be tested.
      video.currentVideoFrameOnEvenField = !video.currentVideoFrameOnEvenField;
    }

    // Determine if the frame is currently blanked
    video.currentVideoFrameBlanked = inputRegs[0x0C].bit(2) || (currentPauseState && !inputRegs[0x0C].bit(5));
  }

  // Don't process the top vsync+vblank+border region unless overscan is active. We could, but it would be wasted
  // effort, since that part of the buffer won't be drawn.
  if (!vdp.screen->overscan() && (targetScanLine < vdpActiveRegionTopOffset)) {
    return;
  }

  // If analog video mixing is disabled, don't modify the scanline, and abort any further processing. This is the
  // correct behaviour here based on hardware tests. Under this mode, VDP graphics are passed through unaltered, and
  // the VDP graphics fader and other mixing features have no effect.
  auto analogMixingMode = inputRegs[0x01].bit(7, 6);
  if (analogMixingMode == 0) {
    vdp.screen->clearOverrideLineDraw(targetScanLine - (!vdp.screen->overscan() ? vdpActiveRegionTopOffset : 0));
    return;
  }

  // Choose which field of the input video to use. We toggle between even and odd fields on successive frames
  // by default for interlace mode. If the register block has manual field selection enabled however, we use
  // the target field indicated by the registers.
  bool useEvenField = video.currentVideoFrameFieldSelectionEnabled[video.drawIndex] ? (bool)video.currentVideoFrameFieldSelectionEvenField[video.drawIndex] : (bool)video.currentVideoFrameOnEvenField;

  // These offset adjustments are based on visual comparisons on a physical player. The positioning was adjustable via
  // calibration, so there's no one fixed, correct positioning settings. These ones get correct alignment for the H32
  // in-game screens from Space Berserker, using the alignment of video in the HUD frames as a reference, based on
  // comparisons with a real player. Further testing Pyramid Patrol, which uses H40, was used to confirm alignment.
  // Testing was also done on the Myst protos, which have precise overlay requirements with individual frames, and also
  // seem to confirm the alignment. Note however that the video on both Myst protos is shifted and out of alignment, in
  // different ways, but we confirmed though hardware tests that they are "off" by the correct amount with this
  // alignment as they are on the real hardware.
  const int VideoFrameLeftBorderWidth = 118;
  const int VideoFrameRightBorderWidth = 0;
  size_t channelCount = 3;
  size_t targetLineInSourceImage = std::min((targetScanLine + (useEvenField ? 263 : 0)), (u32)524);
  size_t sourceLinePos = ((targetLineInSourceImage * video.videoFrameHeader.width) + VideoFrameLeftBorderWidth) * channelCount;
  size_t pixelsInSourceLine = std::max(2, (int)video.videoFrameHeader.width - (VideoFrameLeftBorderWidth + VideoFrameRightBorderWidth));
  size_t targetLinePos = targetScanLine * video.FrameBufferWidth;

  // Retrieve the line of analog video data we're processing here. If there's no frame data present, or the video
  // stream is currently disabled, we force the input analog data to black and proceed with mixing. Note that this
  // includes when the player is paused, as this disables digital memory and blanks the display, unless the image
  // hold bit is set, in which case, digital memory comes back on and the previously captured frame appears again.
  const unsigned char* analogVideoFrameData = nullptr;
  if (video.currentVideoFrameBlanked || video.videoFrameBuffers[video.drawIndex].empty()) {
    video.dummyBlankLineBuffer.resize(pixelsInSourceLine * channelCount, 0);
    analogVideoFrameData = video.dummyBlankLineBuffer.data();
    sourceLinePos = 0;
  } else {
    analogVideoFrameData = video.videoFrameBuffers[video.drawIndex].data();
  }

  // Define some 16.16 fixed point helper functions
  static constexpr uint32_t OneIn1616FixedPoint = 1 << 16;
  // Below are a commented out set of 16.16 routines which are more technically accurate in terms of rounding.
  // This only makes differences of +/- 1 on the output values however, and from profiling they slow down
  // calculations by 30% over the more lossy versions below. Due to the performance critical nature of this
  // code, we have chosen not to implement them at this time, however they are kept here for reference.
  //auto convert6BitUnsignedToNormalized1616FixedPoint = [](uint8_t val) { return ((static_cast<uint32_t>(val) << 10) | (static_cast<uint32_t>(val) << 4) | (static_cast<uint32_t>(val) >> 2)) + ((static_cast<uint32_t>(val) >> 1) & 0x00000001u); };
  //auto convert8BitUnsignedToNormalized1616FixedPoint = [](uint8_t val) { return (((uint16_t)val << 8) | (uint16_t)val) + (((uint16_t)val >> 7) & 0x00000001u); };
  //auto convert1616NormalizedFixedPointTo8BitUnsigned = [](uint32_t val) { auto temp = ((val + 1) * 0xFFu); return (uint32_t)(temp >> 16) + ((temp >> 15) & 0x00000001u); };
  //auto mul1616FixedPoint = [](uint32_t a, uint32_t b) { auto temp = (uint64_t)a * (uint64_t)b; return (uint32_t)(temp >> 16) + ((temp >> 15) & 0x00000001u); };
  auto convert6BitUnsignedToNormalized1616FixedPoint = [](uint8_t val) { return ((uint32_t)val << 10) | ((uint32_t)val << 4) | ((uint32_t)val >> 2); };
  auto convert8BitUnsignedToNormalized1616FixedPoint = [](uint8_t val) { return ((uint32_t)val << 8) | (uint32_t)val; };
  auto convert1616NormalizedFixedPointTo8BitUnsigned = [](uint32_t val) { return (uint8_t)(((val + 0x80) - ((val + 0x80) >> 8)) >> 8); };
  auto mul1616FixedPoint = [](uint32_t a, uint32_t b) { return (uint32_t)(((uint64_t)a * (uint64_t)b) >> 16); };

  // Calculate our linear interpolation coefficients if they haven't already been calculated
  if (video.lineResamplingData.empty()) {
    // Allocate the buffer for our coefficients
    video.lineResamplingData.resize(video.FrameBufferWidth);

    // Calculate the coefficients to use in our linear resampling code below. We convert input sample values into a
    // range of 0.0-1.0, attenuate the value of each input pixel based on the proportion of that pixel that maps to the
    // output pixel value, and finally calculate an overall conversion factor to convert the summed, weighted input
    // pixel values to an output sample in the range 0x00-0xFF. This conversion approach will give the best possible
    // results, regardless of the difference in resolution between input and output.
    float imageWidthConversionRatio = (float)pixelsInSourceLine / (float)video.FrameBufferWidth;
    float firstSamplePointX = 0.0f;
    float lastSamplePointX = 0.0f;
    float totalDomainInverse = 1.0f / imageWidthConversionRatio;
    for (size_t i = 0; i < video.FrameBufferWidth; ++i) {
      auto& entry = video.lineResamplingData[i];
      firstSamplePointX = lastSamplePointX;
      lastSamplePointX = (float)std::min(i + 1, video.FrameBufferWidth - 1) * imageWidthConversionRatio;
      size_t firstSamplePosX = (size_t)firstSamplePointX;
      size_t lastSamplePosX = (size_t)lastSamplePointX;
      for (unsigned int plane = 0; plane < 3; ++plane) {
        for (unsigned int currentSampleX = firstSamplePosX; currentSampleX <= lastSamplePosX; ++currentSampleX) {
          static constexpr float sampleConversion = 1.0f / (float)0xFF;
          float sampleStartPointX = (currentSampleX == firstSamplePosX) ? (firstSamplePointX - (float)firstSamplePosX) : 0.0f;
          float sampleEndPointX = (currentSampleX == lastSamplePosX) ? (lastSamplePointX - (float)lastSamplePosX) : 1.0f;
          float sampleWeightX = sampleEndPointX - sampleStartPointX;
          entry.sampleWeightX.push_back(sampleConversion * sampleWeightX);
        }
      }
      entry.firstSamplePosX = firstSamplePosX;
      entry.lastSamplePosX = lastSamplePosX;
      entry.conversionFactor = totalDomainInverse * (float)0xFF;
    }
  }

  // Mix the analog video stream from the laserdisc with the video output from the Mega Drive VDP
  for (size_t i = 0; i < video.FrameBufferWidth; ++i) {
    // Calculate the pixel value for the laserdisc analog video, using a linear resampling of the source video line to
    // match the screen output resolution.
    auto& entry = video.lineResamplingData[i];
    float convertedSamplesAsFloat[3] = {};
    size_t sampleWeightIndex = 0;
    for (unsigned int currentSampleX = entry.firstSamplePosX; currentSampleX <= entry.lastSamplePosX; ++currentSampleX)
    {
      auto pixelValuePosInSourceFrame = analogVideoFrameData + sourceLinePos + (currentSampleX * channelCount);
      convertedSamplesAsFloat[0] += (*(pixelValuePosInSourceFrame + 0)) * entry.sampleWeightX[sampleWeightIndex];
      convertedSamplesAsFloat[1] += (*(pixelValuePosInSourceFrame + 1)) * entry.sampleWeightX[sampleWeightIndex];
      convertedSamplesAsFloat[2] += (*(pixelValuePosInSourceFrame + 2)) * entry.sampleWeightX[sampleWeightIndex];
      ++sampleWeightIndex;
    }
    convertedSamplesAsFloat[0] *= entry.conversionFactor;
    convertedSamplesAsFloat[1] *= entry.conversionFactor;
    convertedSamplesAsFloat[2] *= entry.conversionFactor;
    auto ldr = (u8)std::min((convertedSamplesAsFloat[0] + 0.5f), (float)0xFF);
    auto ldg = (u8)std::min((convertedSamplesAsFloat[1] + 0.5f), (float)0xFF);
    auto ldb = (u8)std::min((convertedSamplesAsFloat[2] + 0.5f), (float)0xFF);

    // Retrieve the output VDP color for this pixel location
    u32 currentPixelValue = ((vdpPixelBuffer != nullptr) && (i >= vdpBorderLeftOffset)) ? vdpPixelBuffer[i - vdpBorderLeftOffset] : 0;
    auto mdColorPacked = vdp.screen->lookupPalette(currentPixelValue);
    auto mdr = (mdColorPacked >> 16) & 0xFF;
    auto mdg = (mdColorPacked >> 8) & 0xFF;
    auto mdb = mdColorPacked & 0xFF;

    // Extract layer/status flags for this pixel to determine how we're going to mix it
    bool backdrop = (currentPixelValue >> 11) != 0;

    // Composite the digital VDP graphics with the analog video track
    //##TODO## Implement input reg 0x19 bit 0 properly
    unsigned int mdGraphicsFader = convert6BitUnsignedToNormalized1616FixedPoint((backdrop ? (inputRegs[0x1B] >> 2) : (inputRegs[0x1A] >> 2)));
    auto ldNormalizedR = convert8BitUnsignedToNormalized1616FixedPoint(ldr);
    auto ldNormalizedG = convert8BitUnsignedToNormalized1616FixedPoint(ldg);
    auto ldNormalizedB = convert8BitUnsignedToNormalized1616FixedPoint(ldb);
    auto mdNormalizedR = convert8BitUnsignedToNormalized1616FixedPoint(mdr);
    auto mdNormalizedG = convert8BitUnsignedToNormalized1616FixedPoint(mdg);
    auto mdNormalizedB = convert8BitUnsignedToNormalized1616FixedPoint(mdb);
    uint32_t oneMinusMdGraphicsFader = OneIn1616FixedPoint - mdGraphicsFader;
    uint32_t combinedNormalizedR = mul1616FixedPoint(mdNormalizedR, mdGraphicsFader) + mul1616FixedPoint(ldNormalizedR, oneMinusMdGraphicsFader);
    uint32_t combinedNormalizedG = mul1616FixedPoint(mdNormalizedG, mdGraphicsFader) + mul1616FixedPoint(ldNormalizedG, oneMinusMdGraphicsFader);
    uint32_t combinedNormalizedB = mul1616FixedPoint(mdNormalizedB, mdGraphicsFader) + mul1616FixedPoint(ldNormalizedB, oneMinusMdGraphicsFader);

    // Write the composited pixel value to the output framebuffer
    u32 rf = convert1616NormalizedFixedPointTo8BitUnsigned(combinedNormalizedR);
    u32 gf = convert1616NormalizedFixedPointTo8BitUnsigned(combinedNormalizedG);
    u32 bf = convert1616NormalizedFixedPointTo8BitUnsigned(combinedNormalizedB);
    u32 af = 0xFF;
    video.outputFramebuffer[targetLinePos + i] = ((u32)af << 24) | ((u32)rf << 16) | ((u32)gf << 8) | (u32)bf;
  }

  // Override the line draw for this line of video with our composited line buffer
  if (vdp.screen->overscan()) {
    // Draw the whole line
    vdp.screen->overrideLineDraw(targetScanLine, &video.outputFramebuffer[targetLinePos]);
  } else {
    // Draw the display region of the line only, with line 0 starting from the first active line. Note that the current
    // VDP core draws its own framebuffer with extra left and right padding under H40 mode. See VDP::frame() and
    // VDP::pixels(). We compensate for that here with "pixelOffsetInOutputLine", which will cancel out the border
    // offset for H32 mode, and leave the correct 13 pixel offset we need for H40 mode.
    size_t pixelOffsetInOutputLine = 13 * 5;
    //##FIX## We apply a fudge factor here. There's apparently some of the math wrong, and this offset is needed to fix
    //things under H40 mode without overscan visible.
    pixelOffsetInOutputLine += (vdp.h32() ? 0 : 15);
    vdp.screen->overrideLineDraw(targetScanLine - vdpActiveRegionTopOffset, &video.outputFramebuffer[targetLinePos] + vdpBorderLeftOffset + vdpActiveRegionLeftOffset - pixelOffsetInOutputLine);
  }
}
