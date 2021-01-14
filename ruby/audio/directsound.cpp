#include <dsound.h>

struct AudioDirectSound : AudioDriver {
  AudioDirectSound& self = *this;
  AudioDirectSound(Audio& super) : AudioDriver(super) {}
  ~AudioDirectSound() { terminate(); }

  auto create() -> bool override {
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(40);
    return initialize();
  }

  auto driver() -> string override { return "DirectSound 7.0"; }
  auto ready() -> bool override { return _ready; }

  auto hasBlocking() -> bool override { return true; }

  auto hasFrequencies() -> vector<u32> override {
    return {44100, 48000, 96000};
  }

  auto hasLatencies() -> vector<u32> override {
    return {40, 60, 80, 100};
  }

  auto setBlocking(bool blocking) -> bool override { return true; }
  auto setFrequency(u32 frequency) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }

  auto clear() -> void override {
    if(!ready()) return;

    _ringRead = 0;
    _ringWrite = _rings - 1;
    _ringDistance = _rings - 1;

    if(_buffer) memory::fill<u32>(_buffer, _period * _rings);
    _offset = 0;

    if(!_secondary) return;
    _secondary->Stop();
    _secondary->SetCurrentPosition(0);

    void* output;
    DWORD size;
    _secondary->Lock(0, _period * _rings * 4, &output, &size, 0, 0, 0);
    memory::fill<u8>(output, size);
    _secondary->Unlock(output, size, 0, 0);

    _secondary->Play(0, 0, DSBPLAY_LOOPING);
  }

  auto output(const f64 samples[]) -> void override {
    if(!ready()) return;

    _buffer[_offset]  = (u16)sclamp<16>(samples[0] * 32767.0) <<  0;
    _buffer[_offset] |= (u16)sclamp<16>(samples[1] * 32767.0) << 16;
    if(++_offset < _period) return;
    _offset = 0;

    if(self.blocking) {
      //wait until playback buffer has an empty ring to write new audio data to
      while(_ringDistance >= _rings - 1) {
        DWORD position;
        _secondary->GetCurrentPosition(&position, 0);
        u32 ringActive = position / (_period * 4);
        if(ringActive == _ringRead) continue;

        //subtract number of played rings from ring distance counter
        _ringDistance -= (_rings + ringActive - _ringRead) % _rings;
        _ringRead = ringActive;

        if(_ringDistance < 2) {
          //buffer underflow; set max distance to recover quickly
          _ringDistance = _rings - 1;
          _ringWrite = (_rings + _ringRead - 1) % _rings;
          break;
        }
      }
    }

    _ringWrite = (_ringWrite + 1) % _rings;
    _ringDistance = (_ringDistance + 1) % _rings;

    void* output;
    DWORD size;
    if(_secondary->Lock(_ringWrite * _period * 4, _period * 4, &output, &size, 0, 0, 0) == DS_OK) {
      memory::copy<u32>(output, _buffer, _period);
      _secondary->Unlock(output, size, 0, 0);
    }
  }

private:
  auto initialize() -> bool {
    terminate();

    _rings = 8;
    _period = self.frequency * self.latency / _rings / 1000.0 + 0.5;
    _buffer = new u32[_period * _rings];
    _offset = 0;

    if(DirectSoundCreate(0, &_interface, 0) != DS_OK) return terminate(), false;
    _interface->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

    DSBUFFERDESC primaryDescription = {};
    primaryDescription.dwSize = sizeof(DSBUFFERDESC);
    primaryDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    primaryDescription.dwBufferBytes = 0;
    primaryDescription.lpwfxFormat = 0;
    _interface->CreateSoundBuffer(&primaryDescription, &_primary, 0);

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = self.channels;
    waveFormat.nSamplesPerSec = self.frequency;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    _primary->SetFormat(&waveFormat);

    DSBUFFERDESC secondaryDescription = {};
    secondaryDescription.dwSize = sizeof(DSBUFFERDESC);
    secondaryDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS | DSBCAPS_LOCSOFTWARE;
    secondaryDescription.dwBufferBytes = _period * _rings * 4;
    secondaryDescription.guid3DAlgorithm = GUID_NULL;
    secondaryDescription.lpwfxFormat = &waveFormat;
    _interface->CreateSoundBuffer(&secondaryDescription, &_secondary, 0);
    _secondary->SetFrequency(self.frequency);
    _secondary->SetCurrentPosition(0);

    _ready = true;
    clear();
    return true;
  }

  auto terminate() -> void {
    _ready = false;
    if(_buffer) { delete[] _buffer; _buffer = nullptr; }
    if(_secondary) { _secondary->Stop(); _secondary->Release(); _secondary = nullptr; }
    if(_primary) { _primary->Stop(); _primary->Release(); _primary = nullptr; }
    if(_interface) { _interface->Release(); _interface = nullptr; }
  }

  bool _ready = false;

  LPDIRECTSOUND _interface = nullptr;
  LPDIRECTSOUNDBUFFER _primary = nullptr;
  LPDIRECTSOUNDBUFFER _secondary = nullptr;

  u32* _buffer = nullptr;
  u32 _offset = 0;

  u32 _period = 0;
  u32 _rings = 0;
  u32 _ringRead = 0;
  u32 _ringWrite = 0;
  s32 _ringDistance = 0;
};
