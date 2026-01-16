#pragma once

#include <xaudio2.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <audioclient.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

struct AudioXAudio2 : AudioDriver, public IXAudio2VoiceCallback {
  enum : u32 { Buffers = 32 };

  AudioXAudio2& self = *this;
  AudioXAudio2(Audio& super) : AudioDriver(super) { construct(); }
  ~AudioXAudio2() { destruct(); }

  auto create() -> bool override;
  auto hasDevices() -> std::vector<string> override;
  auto clear() -> void override;
  auto level() -> f64 override;
  auto output(const f64 samples[]) -> void override;
  
  auto driver() -> string override { return "XAudio 2.9"; }
  auto ready() -> bool override { return self.isReady; }
  auto hasBlocking() -> bool override { return true; }
  auto hasDynamic() -> bool override { return true; }
  auto hasFrequencies() -> std::vector<u32> override { return {44100, 48000, 96000}; }
  auto hasLatencies() -> std::vector<u32> override { return {10, 20, 40, 60, 80, 100}; }
  auto setDevice(string device) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { return true; }
  auto setFrequency(u32 frequency) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }

private:
  struct Device {
    string id;
    u32 channels = 0;
    u32 frequency = 0;
    Format format = Format::none;
    string name;
  };

  auto construct() -> bool;
  auto destruct() -> void;
  auto initialize() -> bool;
  auto terminate() -> void;
  auto queryDeviceDetails(IMMDevice* pIMMDevice, Device& device) -> bool;
  auto write(const u32* audioData, u32 bytes) -> void;

  std::vector<Device> devices;

  bool isReady = false;
  queue<u32> buffers[Buffers];
  u32 period = 0;          //amount (in 32-bit frames) of samples per buffer
  u32 index = 0;           //current buffer for writing samples to
  volatile long queue = 0;  //how many buffers are queued and ready for playback

  IXAudio2* xa2Interface = nullptr;
  IXAudio2MasteringVoice* masterVoice = nullptr;
  IXAudio2SourceVoice* sourceVoice = nullptr;

  //inherited from IXAudio2VoiceCallback
  STDMETHODIMP_(void) OnBufferStart(void* pBufferContext) noexcept override {}
  STDMETHODIMP_(void) OnLoopEnd(void* pBufferContext) noexcept override {}
  STDMETHODIMP_(void) OnStreamEnd() noexcept override {}
  STDMETHODIMP_(void) OnVoiceError(void* pBufferContext, HRESULT Error) noexcept override {}
  STDMETHODIMP_(void) OnVoiceProcessingPassEnd() noexcept override {}
  STDMETHODIMP_(void) OnVoiceProcessingPassStart(UINT32 BytesRequired) noexcept override {}
  STDMETHODIMP_(void) OnBufferEnd(void* pBufferContext) noexcept override { InterlockedDecrement(&self.queue); }
};
