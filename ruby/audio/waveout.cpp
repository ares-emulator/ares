#include <mmsystem.h>

auto CALLBACK waveOutCallback(HWAVEOUT handle, UINT message, DWORD_PTR userData, DWORD_PTR, DWORD_PTR) -> void;

struct AudioWaveOut : AudioDriver {
  AudioWaveOut& self = *this;
  AudioWaveOut(Audio& super) : AudioDriver(super) {}
  ~AudioWaveOut() { terminate(); }

  auto create() -> bool override {
    super.setChannels(2);
    super.setFrequency(44100);
    super.setLatency(512);
    return initialize();
  }

  auto driver() -> string override { return "waveOut"; }
  auto ready() -> bool override { return true; }

  auto hasDevices() -> std::vector<string> override {
    std::vector<string> devices{"Default"};
    for(u32 index : range(waveOutGetNumDevs())) {
      WAVEOUTCAPS caps{};
      if(waveOutGetDevCaps(index, &caps, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR) {
        devices.push_back((const char*)utf8_t(caps.szPname));
      }
    }
    return devices;
  }

  auto hasBlocking() -> bool override { return true; }
  auto hasDynamic() -> bool override { return true; }
  auto hasFrequencies() -> std::vector<u32> override { return {44100}; }
  auto hasLatencies() -> std::vector<u32> override { return {512, 384, 320, 256, 192, 160, 128, 96, 80, 64, 48, 40, 32}; }

  auto setBlocking(bool blocking) -> bool override { return true; }
  auto setDynamic(bool dynamic) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }

  auto clear() -> void override {
    for(auto& header : headers) {
      memory::fill(header.lpData, frameCount * 4);
    }
  }

  auto level() -> f64 override {
    return (f64)((blockQueue * frameCount) + frameIndex) / (blockCount * frameCount);
  }

  auto output(const f64 samples[]) -> void override {
    u16 lsample = sclamp<16>(samples[0] * 32767.0);
    u16 rsample = sclamp<16>(samples[1] * 32767.0);

    auto block = (u32*)headers[blockIndex].lpData;
    block[frameIndex] = lsample << 0 | rsample << 16;

    if(++frameIndex >= frameCount) {
      frameIndex = 0;
      if(self.dynamic) {
        while(waveOutWrite(handle, &headers[blockIndex], sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
        InterlockedIncrement(&blockQueue);
      } else while(true) {
        auto result = waveOutWrite(handle, &headers[blockIndex], sizeof(WAVEHDR));
        if(!self.blocking || result != WAVERR_STILLPLAYING) break;
        InterlockedIncrement(&blockQueue);
      }
      if(++blockIndex >= blockCount) {
        blockIndex = 0;
      }
    }
  }

private:
  auto initialize() -> bool {
    terminate();

    u32 deviceIndex = 0;
    {
      auto names = hasDevices();
      auto it = std::find(names.begin(), names.end(), self.device);
      if(it != names.end()) deviceIndex = (u32)std::distance(names.begin(), it);
    }

    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = 44100;
    format.nBlockAlign = 4;
    format.wBitsPerSample = 16;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;  //not sizeof(WAVEFORMAT); size of extra information after WAVEFORMATEX
    //-1 = default; 0+ = specific device; subtract -1 as hasDevices() includes "Default" entry
    waveOutOpen(&handle, (s32)deviceIndex - 1, &format, (DWORD_PTR)waveOutCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);

    frameCount = self.latency;
    blockCount = 32;
    frameIndex = 0;
    blockIndex = 0;
    blockQueue = 0;

    headers.resize(blockCount);
    for(auto& header : headers) {
      memory::fill(&header, sizeof(WAVEHDR));
      header.lpData = (LPSTR)LocalAlloc(LMEM_FIXED, frameCount * 4);
      header.dwBufferLength = frameCount * 4;
      waveOutPrepareHeader(handle, &header, sizeof(WAVEHDR));
    }

    waveOutSetVolume(handle, 0xffff'ffff);  //100% volume (65535 left, 65535 right)
    waveOutRestart(handle);
    return true;
  }

  auto terminate() -> void {
    if(!handle) return;
    waveOutPause(handle);
    waveOutReset(handle);
    for(auto& header : headers) {
      waveOutUnprepareHeader(handle, &header, sizeof(WAVEHDR));
      LocalFree(header.lpData);
    }
    waveOutClose(handle);
    handle = nullptr;
    headers.clear();
  }

  HWAVEOUT handle = nullptr;
  std::vector<WAVEHDR> headers;
  u32 frameCount = 0;
  u32 blockCount = 0;
  u32 frameIndex = 0;
  u32 blockIndex = 0;

public:
  LONG blockQueue = 0;
};

auto CALLBACK waveOutCallback(HWAVEOUT handle, UINT message, DWORD_PTR userData, DWORD_PTR, DWORD_PTR) -> void {
  auto instance = (AudioWaveOut*)userData;
  if(instance->blockQueue > 0) InterlockedDecrement(&instance->blockQueue);
}
