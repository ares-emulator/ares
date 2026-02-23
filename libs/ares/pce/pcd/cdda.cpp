auto PCD::CDDA::load(Node::Object parent) -> void {
  stream = parent->append<Node::Audio::Stream>("CD-DA");
  stream->setChannels(2);
  stream->setFrequency(44100);
}

auto PCD::CDDA::unload(Node::Object parent) -> void {
  parent->remove(stream);
  stream.reset();
}

auto PCD::CDDA::clockSector() -> void {
  if(!drive->inCDDA()) return;
  if(drive->paused()) return;
  if(drive->stopped()) return;
  if(!drive->read()) return;

  sample.offset = 0;

  if(drive->inactive()) {
    if(playMode == PlayMode::Loop) {
      drive->seekPlay();
    }
    if(playMode == PlayMode::IRQ) {
      drive->setStopped();
      scsi->irq.completed.raise();  //unverified
    }
    if(playMode == PlayMode::Once) {
      drive->setStopped();
    }
  }
}

auto PCD::CDDA::clockSample() -> void {
  // Retrieve the next CD digital audio sample
  i16 digitalSampleLeft = 0;
  i16 digitalSampleRight = 0;
  if (drive->playing()) {
    if(drive->playingAudioTrack()) {
      digitalSampleLeft  |= drive->sector[sample.offset++] << 0;
      digitalSampleLeft  |= drive->sector[sample.offset++] << 8;
      digitalSampleRight |= drive->sector[sample.offset++] << 0;
      digitalSampleRight |= drive->sector[sample.offset++] << 8;
    } else if (Model::LaserActive()) {
      sample.offset += 4;
    }
  }
  sample.left = digitalSampleLeft;
  sample.right = digitalSampleRight;

  // Apply the CDDA fader
  digitalSampleLeft = (i16)(sample.left * fader->cdda());
  digitalSampleRight = (i16)(sample.right * fader->cdda());

  // If we're emulating a LaserActive, mix in analog audio, and apply additional fader settings.
  i16 combinedSampleLeft = digitalSampleLeft;
  i16 combinedSampleRight = digitalSampleRight;
  if (Model::LaserActive()) {
    // Determine the state of our overall audio mixing mode settings
    //##FIX## Note that we don't take the unusual "latching" behaviour of the digital audio mixing disabled state
    //persisting when switching into analog mixing mode 0 here, as described in the notes for input register 0x01. It
    //is highly, highly unlikely anything relies on this however. If there was a scenario however where only VDP
    //graphics were on the screen, and digital audio was intended to play, but was instead silent, it's possible this
    //is the culprit. Fixing this would require latching a mute state into a register, rather than evaluating the
    //input register state live like we do here.
    auto analogMixingMode = pcd.ld.inputRegs[0x01].bit(7, 6);
    auto audioMixingInputSelection = pcd.ld.inputRegs[0x0D].bit(7, 6);
    bool digitalAudioMixingDisabled = (analogMixingMode > 0) && ((audioMixingInputSelection == 2) || ((analogMixingMode == 1) && pcd.ld.inputRegs[0x0D].bit(4)));
    bool digitalAudioAttenuationDisabled = (analogMixingMode == 0) || (audioMixingInputSelection == 3);
    bool analogAudioMixingDisabled = (analogMixingMode == 0) || ((analogMixingMode == 1) && !pcd.ld.inputRegs[0x0D].bit(4));

    // Disable digital audio if it is turned off
    if (digitalAudioMixingDisabled) {
      digitalSampleLeft = 0;
      digitalSampleRight = 0;
    }

    // Attenuate the digital audio using the digital audio fader
    if (!digitalAudioAttenuationDisabled) {
      float digitalAudioFader = (float)pcd.ld.inputRegs[0x0F] / (float)((1 << 8) - 1);
      digitalSampleLeft = (int16_t)((float)digitalSampleLeft * digitalAudioFader);
      digitalSampleRight = (int16_t)((float)digitalSampleRight * digitalAudioFader);
    }

    // Take digital left/right exclusive register state into account
    if ((analogMixingMode != 0) && pcd.ld.inputRegs[0x0D].bit(0) && pcd.ld.inputRegs[0x0D].bit(1)) {
      digitalSampleLeft /= 2;
      digitalSampleRight /= 2;
    } else if (pcd.ld.inputRegs[0x0D].bit(0)) {
      digitalSampleRight = digitalSampleLeft;
    } else if (pcd.ld.inputRegs[0x0D].bit(1)) {
      digitalSampleLeft = digitalSampleRight;
    }

    // Retrieve the next analog audio sample
    i16 analogSampleLeft = 0;
    i16 analogSampleRight = 0;
    auto analogAudioSamplePos = (drive->lba * 2352) + sample.offset + (pcd.ld.analogAudioLeadingAudioSamples * 4);
    if ((analogAudioSamplePos + 3) < pcd.ld.analogAudioRawDataView.size()) {
      analogSampleLeft = (i16)((u16)pcd.ld.analogAudioRawDataView[analogAudioSamplePos + 0] | (u16)(pcd.ld.analogAudioRawDataView[analogAudioSamplePos + 1] << 8));
      analogSampleRight = (i16)((u16)pcd.ld.analogAudioRawDataView[analogAudioSamplePos + 2] | (u16)(pcd.ld.analogAudioRawDataView[analogAudioSamplePos + 3] << 8));
    }

    // Disable analog audio if it is turned off
    if (analogAudioMixingDisabled) {
      analogSampleLeft = 0;
      analogSampleRight = 0;
    }

    // Take analog mute and left/right exclusive register state into account
    if (pcd.ld.inputRegs[0x0E].bit(7) || (pcd.ld.inputRegs[0x0E].bit(0) && pcd.ld.inputRegs[0x0E].bit(1))) {
      analogSampleLeft = 0;
      analogSampleRight = 0;
    } else if (pcd.ld.inputRegs[0x0E].bit(0)) {
      analogSampleRight = analogSampleLeft;
    } else if (pcd.ld.inputRegs[0x0E].bit(1)) {
      analogSampleLeft = analogSampleRight;
    }

    // Attenuate analog audio by the current attenuation register settings. Note that as per the hardware, the
    // maximum attenuation value of 0xFF doesn't give total silence.
    //##FIX## We currently make the "fade to mute" flags apply immediately. This is technically incorrect, however
    //since most likely nothing relies on this, and it's a bit of a pain to do, we just make them take effect
    //immediately here. If we want to do this properly, we need to take accurate measurements on the hardware for
    //the time taken for the fade, then tie a process into the clock event to reduce a secondary attenuation value
    //by the correct amount to achieve that timing as the clock advances. The fact that triggering a fade for a
    //second channel applies immediately if the other channel has already been faded to mute, shows there's in
    //fact a single secondary attenuation counter/register behind the scenes which handles this, which both
    //channels share.
    unsigned int analogAudioVolumeScaleLeft = 0x100 - (unsigned int)pcd.ld.analogAudioAttenuationLeft;
    unsigned int analogAudioVolumeScaleRight = 0x100 - (unsigned int)pcd.ld.analogAudioAttenuationRight;
    analogAudioVolumeScaleLeft = (pcd.ld.analogAudioFadeToMutedLeft ? 0 : analogAudioVolumeScaleLeft);
    analogAudioVolumeScaleRight = (pcd.ld.analogAudioFadeToMutedRight ? 0 : analogAudioVolumeScaleRight);
    analogSampleLeft = (i16)(((int)analogSampleLeft * (int)analogAudioVolumeScaleLeft) >> 8);
    analogSampleRight = (i16)(((int)analogSampleRight * (int)analogAudioVolumeScaleRight) >> 8);

    // Audio is disabled in frame stepping mode
    if (pcd.ld.currentPlaybackMode == 0x02) {
      digitalSampleLeft = 0;
      digitalSampleRight = 0;
      analogSampleLeft = 0;
      analogSampleRight = 0;
    }

    // Mix analog and digital audio together
    combinedSampleLeft = (int16_t)std::clamp((int)digitalSampleLeft + (int)analogSampleLeft, (int)std::numeric_limits<int16_t>::min(), (int)std::numeric_limits<int16_t>::max());
    combinedSampleRight = (int16_t)std::clamp((int)digitalSampleRight + (int)analogSampleRight, (int)std::numeric_limits<int16_t>::min(), (int)std::numeric_limits<int16_t>::max());
  }

  // Output the combined sample
  auto finalSampleLeft = combinedSampleLeft / 32768.0;
  auto finalSampleRight = combinedSampleRight / 32768.0;
  stream->frame(finalSampleLeft, finalSampleRight);
}

auto PCD::CDDA::power() -> void {
  sample = {};
}
