#include "xaudio2.hpp"

  auto AudioXAudio2::create() -> bool {
    auto devices = hasDevices();
    if(!devices.empty()) super.setDevice(devices.front());
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(40);
    return initialize();
  }

  auto AudioXAudio2::hasDevices() -> std::vector<string> {
    std::vector<string> devices;
    for(auto& device : self.devices) devices.push_back(device.name);
    return devices;
  }
  
  auto AudioXAudio2::clear() -> void {
    if(self.sourceVoice) {
      self.sourceVoice->Stop(0);
      self.sourceVoice->FlushSourceBuffers();  //calls OnBufferEnd for all currently submitted buffers
    }

    self.index = 0;
    self.queue = 0;
    for(u32 n : range(Buffers)) self.buffers[n].fill();

    if(self.sourceVoice) self.sourceVoice->Start(0);
  }
  
  auto AudioXAudio2::level() -> f64 {
    XAUDIO2_VOICE_STATE state{};
    self.sourceVoice->GetState(&state);
    u32 level = state.BuffersQueued * self.period + buffers[self.index].size() - state.SamplesPlayed % self.period;
    u32 limit = Buffers * self.period;
    return (f64)level / limit;
  }

  auto AudioXAudio2::output(const f64 samples[]) -> void {
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
  
  auto AudioXAudio2::construct() -> bool {
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

  auto AudioXAudio2::destruct() -> void {
    terminate();

    if(self.xa2Interface) {
      self.xa2Interface->Release();
      self.xa2Interface = nullptr;
    }
    CoUninitialize();
  }

  auto AudioXAudio2::initialize() -> bool {
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

  auto AudioXAudio2::terminate() -> void {
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

  auto AudioXAudio2::queryDeviceDetails(IMMDevice* pIMMDevice, Device& device) -> bool {
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

  auto AudioXAudio2::write(const u32* audioData, u32 bytes) -> void {
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = bytes;
    buffer.pAudioData = (const BYTE*)audioData;
    buffer.pContext = nullptr;
    InterlockedIncrement(&self.queue);
    self.sourceVoice->SubmitSourceBuffer(&buffer);
  }
