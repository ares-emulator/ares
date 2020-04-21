MSU1 msu1;
#include "serialization.cpp"

auto MSU1::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("MSU1");
  stream->setChannels(2);
  stream->setFrequency(44100);
}

auto MSU1::unload() -> void {
  stream = {};
  dataFile = {};
  audioFile = {};

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto MSU1::main() -> void {
  double left  = 0.0;
  double right = 0.0;

  if(io.audioPlay) {
    if(audioFile) {
      if(audioFile->end()) {
        if(!io.audioRepeat) {
          io.audioPlay = false;
          audioFile->seek(io.audioPlayOffset = 8);
        } else {
          audioFile->seek(io.audioPlayOffset = io.audioLoopOffset);
        }
      } else {
        io.audioPlayOffset += 4;
        left  = (double)(int16)audioFile->readl(2) / 32768.0 * (double)io.audioVolume / 255.0;
        right = (double)(int16)audioFile->readl(2) / 32768.0 * (double)io.audioVolume / 255.0;
        if(dsp.mute()) left = 0, right = 0;
      }
    } else {
      io.audioPlay = false;
    }
  }

  stream->sample(left, right);
  Thread::step(1);
  Thread::synchronize(cpu);
}

auto MSU1::power() -> void {
  Thread::create(44100, {&MSU1::main, this});
  cpu.coprocessors.append(this);

  io.dataSeekOffset = 0;
  io.dataReadOffset = 0;

  io.audioPlayOffset = 0;
  io.audioLoopOffset = 0;

  io.audioTrack = 0;
  io.audioVolume = 0;

  io.audioResumeTrack = ~0;  //no resume
  io.audioResumeOffset = 0;

  io.audioError = false;
  io.audioPlay = false;
  io.audioRepeat = false;
  io.audioBusy = false;
  io.dataBusy = false;

  dataOpen();
  audioOpen();
}

auto MSU1::dataOpen() -> void {
  dataFile.reset();
  string name = {"msu1/data.rom"};
  if(dataFile = platform->open(cartridge.node, name, File::Read)) {
    dataFile->seek(io.dataReadOffset);
  }
}

auto MSU1::audioOpen() -> void {
  audioFile.reset();
  string name = {"msu1/track-", io.audioTrack, ".pcm"};
  if(audioFile = platform->open(cartridge.node, name, File::Read)) {
    if(audioFile->size() >= 8) {
      uint32 header = audioFile->readm(4);
      if(header == 0x4d535531) {  //"MSU1"
        io.audioLoopOffset = 8 + audioFile->readl(4) * 4;
        if(io.audioLoopOffset > audioFile->size()) io.audioLoopOffset = 8;
        io.audioError = false;
        audioFile->seek(io.audioPlayOffset);
        return;
      }
    }
    audioFile.reset();
  }
  io.audioError = true;
}

auto MSU1::readIO(uint24 address, uint8 data) -> uint8 {
  cpu.synchronize(*this);

  switch(0x2000 | address & 7) {
  case 0x2000:
    data.bit(0,2) = Revision;
    data.bit(3)   = io.audioError;
    data.bit(4)   = io.audioPlay;
    data.bit(5)   = io.audioRepeat;
    data.bit(6)   = io.audioBusy;
    data.bit(7)   = io.dataBusy;
    return data;
  case 0x2001:
    if(io.dataBusy) return 0x00;
    if(!dataFile) return 0x00;
    if(dataFile->end()) return 0x00;
    io.dataReadOffset++;
    return dataFile->read();
  case 0x2002: return 'S';
  case 0x2003: return '-';
  case 0x2004: return 'M';
  case 0x2005: return 'S';
  case 0x2006: return 'U';
  case 0x2007: return '1';
  }

  return data;  //unreachable
}

auto MSU1::writeIO(uint24 address, uint8 data) -> void {
  cpu.synchronize(*this);

  switch(0x2000 | address & 7) {
  case 0x2000: io.dataSeekOffset.byte(0) = data; break;
  case 0x2001: io.dataSeekOffset.byte(1) = data; break;
  case 0x2002: io.dataSeekOffset.byte(2) = data; break;
  case 0x2003: io.dataSeekOffset.byte(3) = data;
    io.dataReadOffset = io.dataSeekOffset;
    if(dataFile) dataFile->seek(io.dataReadOffset);
    break;
  case 0x2004: io.audioTrack.byte(0) = data; break;
  case 0x2005: io.audioTrack.byte(1) = data;
    io.audioPlay = false;
    io.audioRepeat = false;
    io.audioPlayOffset = 8;
    if(io.audioTrack == io.audioResumeTrack) {
      io.audioPlayOffset = io.audioResumeOffset;
      io.audioResumeTrack = ~0;  //erase resume track
      io.audioResumeOffset = 0;
    }
    audioOpen();
    break;
  case 0x2006:
    io.audioVolume = data;
    break;
  case 0x2007:
    if(io.audioBusy) break;
    if(io.audioError) break;
    io.audioPlay = data.bit(0);
    io.audioRepeat = data.bit(1);
    boolean audioResume = data.bit(2);
    if(!io.audioPlay && audioResume) {
      io.audioResumeTrack = io.audioTrack;
      io.audioResumeOffset = io.audioPlayOffset;
    }
    break;
  }
}
