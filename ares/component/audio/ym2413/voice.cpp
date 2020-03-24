auto YM2413::Voice::update(const uint8* data) -> void {
  if(data) {
    modulator.multiple    = data[0].bit(0,3);
    modulator.scaleRate   = data[0].bit(4);
    modulator.sustainable = data[0].bit(5);
    modulator.vibrato     = data[0].bit(6);
    modulator.tremolo     = data[0].bit(7);

    carrier.multiple    = data[1].bit(0,3);
    carrier.scaleRate   = data[1].bit(4);
    carrier.sustainable = data[1].bit(5);
    carrier.vibrato     = data[1].bit(6);
    carrier.tremolo     = data[1].bit(7);

    modulator.totalLevel        = data[2].bit(0,5);
    modulator.scaleLevel.bit(1) = data[2].bit(6);
    modulator.scaleLevel.bit(0) = data[2].bit(7);  //bits swapped

    feedback                  = data[3].bit(0,2);
    modulator.waveform        = data[3].bit(3);
    carrier.waveform          = data[3].bit(4);
  //unused                    = data[3].bit(5);
    carrier.scaleLevel.bit(1) = data[3].bit(6);
    carrier.scaleLevel.bit(0) = data[3].bit(7);  //bits swapped

    modulator.decay  = data[4].bit(0,3);
    modulator.attack = data[4].bit(4,7);

    carrier.decay  = data[5].bit(0,3);
    carrier.attack = data[5].bit(4,7);

    modulator.release = data[6].bit(0,3);
    modulator.sustain = data[6].bit(4,7);

    carrier.release = data[7].bit(0,3);
    carrier.sustain = data[7].bit(4,7);
  }

  modulator.block = block;
  modulator.fnumber = fnumber;
  if(modulator.audible) modulator.totalLevel = tone << 2;
  modulator.update();

  carrier.block = block;
  carrier.fnumber = fnumber;
  carrier.totalLevel = level << 2;
  carrier.update();
}
