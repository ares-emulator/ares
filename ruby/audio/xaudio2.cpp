#include <xaudio2.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audioclient.h>
#include <mmreg.h>

struct AudioXAudio2 : AudioDriver, public IXAudio2VoiceCallback {
  enum : u32 { Buffers = 32 };

  AudioXAudio2& self = *this;
  AudioXAudio2(Audio& super) : AudioDriver(super) { construct(); }
  ~AudioXAudio2() { destruct(); }

  auto create() -> bool override {
    auto devices = hasDevices();
    if(!devices.empty()) super.setDevice(devices.front());
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(40);
    return initialize();
  }

  auto hasDevices() -> std::vector<string> override {
    std::vector<string> devices;
    for(auto& device : self.devices) devices.push_back(device.name);
    return devices;
  }
  
  auto clear() -> void override {
    if(self.sourceVoice) {
      self.sourceVoice->Stop(0);
      self.sourceVoice->FlushSourceBuffers();  //calls OnBufferEnd for all currently submitted buffers
    }

    self.index = 0;
    self.queue = 0;
    for(u32 n : range(Buffers)) self.buffers[n].fill();

    if(self.sourceVoice) self.sourceVoice->Start(0);
  }
  
  auto level() -> f64 override {
    XAUDIO2_VOICE_STATE state{};
    self.sourceVoice->GetState(&state);
    u32 level = state.BuffersQueued * self.period + buffers[self.index].size() - state.SamplesPlayed % self.period;
    u32 limit = Buffers * self.period;
    return (f64)level / limit;
  }

  auto output(const f64 samples[]) -> void override {
    u32 frame = 0;
    frame |= (u16)sclamp<16>(samples[0] * 32767.0) <<  0;
    frame |= (u16)sclamp<16>(samples[1] * 32767.0) << 16;

    auto& buffer = self.buffers[self.index];
    buffer.write(frame);
    if(!buffer.full()) return;

    buffer.flush();
    if(self.queue == Buffers - 1) {
      if(self.blocking) {
        //wait until there is at least one other free buffer for the next sample
        while(self.queue == Buffers - 1);
      } else {
        //there is no free buffer for the next block, so ignore the current contents
        return;
      }
    }

    write(buffer.data(), buffer.capacity<u8>());
    self.index = (self.index + 1) % Buffers;
  }
  
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

  auto construct() -> bool {
    bool result = false;
    if(XAudio2Create(&self.xa2Interface, 0 , XAUDIO2_DEFAULT_PROCESSOR) != S_OK) return result;
 
    IMMDeviceEnumerator* pEnumerator = nullptr;
    if(SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pEnumerator)))) {
      IMMDeviceCollection* pCollection = nullptr;
      if(SUCCEEDED(pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection))) {
        u32 deviceCount = 0;
        if(SUCCEEDED(pCollection->GetCount(&deviceCount))) {
          for(u32 deviceIndex : range(deviceCount)) {
            IMMDevice* pIMMDevice = nullptr;
            if(SUCCEEDED(pCollection->Item(deviceIndex, &pIMMDevice))) {
              IPropertyStore* pProps = nullptr;
              if(SUCCEEDED(pIMMDevice->OpenPropertyStore(STGM_READ, &pProps))) {
                PROPVARIANT varName = {};
                PropVariantInit(&varName);
                if(SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
                  Device device = {};
                  LPWSTR pwstrId = nullptr;
                  if(SUCCEEDED(pIMMDevice->GetId(&pwstrId))) {
                    device.id = (const char*)utf8_t(pwstrId);
                    device.name = (const char*)utf8_t(varName.pwszVal);
                    if(self.queryDeviceDetails(pIMMDevice, device)) {
                      devices.push_back(device);
                      result = true;
                    }
                    CoTaskMemFree(pwstrId);
                  }
                }
                PropVariantClear(&varName);
              }
              pProps->Release();
            }
            pIMMDevice->Release();
          }
        }
      }
      pCollection->Release();
    }
    pEnumerator->Release();
    return result;
  }

  auto destruct() -> void {
    terminate();

    if(self.xa2Interface) {
      self.xa2Interface->Release();
      self.xa2Interface = nullptr;
    }
    CoUninitialize();
  }

  auto initialize() -> bool {
    terminate();
    if(!self.xa2Interface) return false;

    self.period = self.frequency * self.latency / Buffers / 1000.0 + 0.5;
    for(u32 n : range(Buffers)) buffers[n].resize(self.period);
    self.index = 0;
    self.queue = 0;

    auto names = hasDevices();
    u32 idx = (u32)index_of(names, self.device).value_or(0);
    if(FAILED(self.xa2Interface->CreateMasteringVoice(&self.masterVoice, self.channels, self.frequency, 0, (LPCWSTR)utf16_t(devices[idx].id.data()), nullptr))) return terminate(), false;

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = self.channels;
    waveFormat.nSamplesPerSec = self.frequency;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    if(FAILED(self.xa2Interface->CreateSourceVoice(&self.sourceVoice, (WAVEFORMATEX*)&waveFormat, XAUDIO2_VOICE_NOSRC, XAUDIO2_DEFAULT_FREQ_RATIO, this, nullptr, nullptr))) return terminate(), false;

    clear();
    return self.isReady = true;
  }

  auto terminate() -> void {
    self.isReady = false;

    if(self.sourceVoice) {
      self.sourceVoice->Stop(0);
      self.sourceVoice->DestroyVoice();
      self.sourceVoice = nullptr;
    }

    if(self.masterVoice) {
      self.masterVoice->DestroyVoice();
      self.masterVoice = nullptr;
    }
  }

  auto queryDeviceDetails(IMMDevice* pIMMDevice, Device& device) -> bool {
    IAudioClient* pAudioClient = nullptr;
    if(pIMMDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&pAudioClient)) != S_OK) return false;
    WAVEFORMATEX* pwfx = nullptr;
    if(pAudioClient->GetMixFormat(&pwfx) !=S_OK) return pAudioClient->Release(), false;

    if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pwfx->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) {
      PWAVEFORMATEXTENSIBLE pExt = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
      if (IsEqualGUID(pExt->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
        if(pExt->Format.wBitsPerSample == 32) device.format = Format::float32;
      } else if(IsEqualGUID(pExt->SubFormat, KSDATAFORMAT_SUBTYPE_PCM)) {
        if(pExt->Format.wBitsPerSample == 16) device.format = Format::int16;
        if(pExt->Format.wBitsPerSample == 32) device.format = Format::int32;
      }
        device.channels = pExt->Format.nChannels;
        device.frequency = pExt->Format.nSamplesPerSec;
    } else {
      if(pwfx->wBitsPerSample == 16) device.format = Format::int16;
      if(pwfx->wBitsPerSample == 32) device.format = Format::int32;
      device.channels = pwfx->nChannels;
      device.frequency = pwfx->nSamplesPerSec;
    }     
    CoTaskMemFree(pwfx);
    pAudioClient->Release();
    return true;
  }

  auto write(const u32* audioData, u32 bytes) -> void {
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = bytes;
    buffer.pAudioData = (const BYTE*)audioData;
    buffer.pContext = nullptr;
    InterlockedIncrement(&self.queue);
    self.sourceVoice->SubmitSourceBuffer(&buffer);
  }

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
