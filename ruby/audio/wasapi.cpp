#include <avrt.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <devicetopology.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#if defined(_MSC_VER)
  #define CLSID_MMDeviceEnumerator __uuidof(MMDeviceEnumerator)
  #define IID_IMMDeviceEnumerator  __uuidof(IMMDeviceEnumerator)
  #define IID_IAudioClient         __uuidof(IAudioClient)
  #define IID_IAudioRenderClient   __uuidof(IAudioRenderClient)
#endif

struct AudioWASAPI : AudioDriver {
  AudioWASAPI& self = *this;
  AudioWASAPI(Audio& super) : AudioDriver(super) { construct(); }
  ~AudioWASAPI() { destruct(); }

  auto create() -> bool override {
    super.setExclusive(false);
    if(hasDevices()) super.setDevice(hasDevices().first());
    super.setBlocking(false);
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(40);
    return initialize();
  }

  auto driver() -> string override { return "WASAPI"; }
  auto ready() -> bool override { return self.isReady; }

  auto hasExclusive() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }

  auto hasDevices() -> vector<string> override {
    vector<string> devices;
    for(auto& device : self.devices) devices.append(device.name);
    return devices;
  }

  auto hasChannels() -> vector<u32> override {
    return {self.channels};
  }

  auto hasFrequencies() -> vector<u32> override {
    return {self.frequency};
  }

  auto hasLatencies() -> vector<u32> override {
    return {0, 20, 40, 60, 80, 100};
  }

  auto setExclusive(bool exclusive) -> bool override { return initialize(); }
  auto setDevice(string device) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { return true; }
  auto setFrequency(u32 frequency) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }

  auto clear() -> void override {
    if(!ready()) return;

    self.queue.read = 0;
    self.queue.write = 0;
    self.queue.count = 0;
    self.audioClient->Stop();
    self.audioClient->Reset();
    self.audioClient->Start();
  }

  auto output(const f64 samples[]) -> void override {
    for(u32 n : range(self.channels)) {
      self.queue.samples[self.queue.write][n] = samples[n];
    }
    self.queue.write++;
    self.queue.count++;

    if(self.queue.count >= self.bufferSize) {
      if(WaitForSingleObject(self.eventHandle, self.blocking ? INFINITE : 0) == WAIT_OBJECT_0) {
        write();
      }
    }
  }

private:
  struct Device {
    string id;
    string name;
  };
  vector<Device> devices;

  auto construct() -> bool {
    if(CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&self.enumerator) != S_OK) return false;

    IMMDevice* defaultDeviceContext = nullptr;
    if(self.enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDeviceContext) != S_OK) return false;

    Device defaultDevice;
    LPWSTR defaultDeviceString = nullptr;
    defaultDeviceContext->GetId(&defaultDeviceString);
    defaultDevice.id = (const char*)utf8_t(defaultDeviceString);
    CoTaskMemFree(defaultDeviceString);

    IMMDeviceCollection* deviceCollection = nullptr;
    if(self.enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection) != S_OK) return false;

    u32 deviceCount = 0;
    if(deviceCollection->GetCount(&deviceCount) != S_OK) return false;

    for(u32 deviceIndex : range(deviceCount)) {
      IMMDevice* deviceContext = nullptr;
      if(deviceCollection->Item(deviceIndex, &deviceContext) != S_OK) continue;

      Device device;
      LPWSTR deviceString = nullptr;
      deviceContext->GetId(&deviceString);
      device.id = (const char*)utf8_t(deviceString);
      CoTaskMemFree(deviceString);

      IPropertyStore* propertyStore = nullptr;
      deviceContext->OpenPropertyStore(STGM_READ, &propertyStore);
      PROPVARIANT propVariant;
      propertyStore->GetValue(PKEY_Device_FriendlyName, &propVariant);
      device.name = (const char*)utf8_t(propVariant.pwszVal);
      propertyStore->Release();

      if(device.id == defaultDevice.id) {
        self.devices.prepend(device);
      } else {
        self.devices.append(device);
      }
    }

    deviceCollection->Release();
    return true;
  }

  auto destruct() -> void {
    terminate();

    if(self.enumerator) {
      self.enumerator->Release();
      self.enumerator = nullptr;
    }
  }

  auto initialize() -> bool {
    terminate();

    string deviceID;
    if(auto index = self.devices.find([&](auto& device) { return device.name == self.device; })) {
      deviceID = self.devices[*index].id;
    } else {
      return false;
    }

    utf16_t deviceString(deviceID);
    if(self.enumerator->GetDevice(deviceString, &self.audioDevice) != S_OK) return false;

    if(self.audioDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&self.audioClient) != S_OK) return false;

    WAVEFORMATEXTENSIBLE waveFormat{};
    if(self.exclusive) {
      IPropertyStore* propertyStore = nullptr;
      if(self.audioDevice->OpenPropertyStore(STGM_READ, &propertyStore) != S_OK) return false;
      PROPVARIANT propVariant;
      if(propertyStore->GetValue(PKEY_AudioEngine_DeviceFormat, &propVariant) != S_OK) return false;
      waveFormat = *(WAVEFORMATEXTENSIBLE*)propVariant.blob.pBlobData;
      propertyStore->Release();
      if(self.audioClient->GetDevicePeriod(nullptr, &self.devicePeriod) != S_OK) return false;
      auto latency = max(self.devicePeriod, (REFERENCE_TIME)self.latency * 10'000);  //1ms to 100ns units
      auto result = self.audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, latency, latency, &waveFormat.Format, nullptr);
      if(result == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
        if(self.audioClient->GetBufferSize(&self.bufferSize) != S_OK) return false;
        self.audioClient->Release();
        latency = (REFERENCE_TIME)(10'000 * 1'000 * self.bufferSize / waveFormat.Format.nSamplesPerSec);
        if(self.audioDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, (void**)&self.audioClient) != S_OK) return false;
        result = self.audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, latency, latency, &waveFormat.Format, nullptr);
      }
      if(result != S_OK) return false;
      DWORD taskIndex = 0;
      self.taskHandle = AvSetMmThreadCharacteristics(L"Pro Audio", &taskIndex);
    } else {
      WAVEFORMATEX* waveFormatEx = nullptr;
      if(self.audioClient->GetMixFormat(&waveFormatEx) != S_OK) return false;
      waveFormat = *(WAVEFORMATEXTENSIBLE*)waveFormatEx;
      CoTaskMemFree(waveFormatEx);
      if(self.audioClient->GetDevicePeriod(&self.devicePeriod, nullptr)) return false;
      auto latency = max(self.devicePeriod, (REFERENCE_TIME)self.latency * 10'000);  //1ms to 100ns units
      if(self.audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, latency, 0, &waveFormat.Format, nullptr) != S_OK) return false;
    }

    self.eventHandle = CreateEvent(nullptr, false, false, nullptr);
    if(self.audioClient->SetEventHandle(self.eventHandle) != S_OK) return false;
    if(self.audioClient->GetService(IID_IAudioRenderClient, (void**)&self.renderClient) != S_OK) return false;
    if(self.audioClient->GetBufferSize(&self.bufferSize) != S_OK) return false;

    self.channels = waveFormat.Format.nChannels;
    self.frequency = waveFormat.Format.nSamplesPerSec;
    self.mode = waveFormat.SubFormat.Data1;
    self.precision = waveFormat.Format.wBitsPerSample;

    clear();
    return self.isReady = true;
  }

  auto terminate() -> void {
    self.isReady = false;
    if(self.audioClient) self.audioClient->Stop();
    if(self.renderClient) self.renderClient->Release(), self.renderClient = nullptr;
    if(self.audioClient) self.audioClient->Release(), self.audioClient = nullptr;
    if(self.audioDevice) self.audioDevice->Release(), self.audioDevice = nullptr;
    if(self.eventHandle) CloseHandle(self.eventHandle), self.eventHandle = nullptr;
    if(self.taskHandle) AvRevertMmThreadCharacteristics(self.taskHandle), self.taskHandle = nullptr;
  }

  auto write() -> void {
    u32 available = self.bufferSize;
    if(!self.exclusive) {
      u32 padding = 0;
      self.audioClient->GetCurrentPadding(&padding);
      available = self.bufferSize - padding;
    }
    u32 length = min(available, self.queue.count);

    u8* buffer = nullptr;
    if(self.renderClient->GetBuffer(length, &buffer) == S_OK) {
      u32 bufferFlags = 0;
      for(u32 _ : range(length)) {
        f64 samples[8] = {};
        if(self.queue.count) {
          for(u32 n : range(self.channels)) {
            samples[n] = self.queue.samples[self.queue.read][n];
          }
          self.queue.read++;
          self.queue.count--;
        }

        if(self.mode == 1 && self.precision == 16) {
          auto output = (u16*)buffer;
          for(u32 n : range(self.channels)) *output++ = (u16)sclamp<16>(samples[n] * (32768.0 - 1.0));
          buffer = (u8*)output;
        } else if(self.mode == 1 && self.precision == 32) {
          auto output = (u32*)buffer;
          for(u32 n : range(self.channels)) *output++ = (u32)sclamp<32>(samples[n] * (65536.0 * 32768.0 - 1.0));
          buffer = (u8*)output;
        } else if(self.mode == 3 && self.precision == 32) {
          auto output = (f32*)buffer;
          for(u32 n : range(self.channels)) *output++ = f32(max(-1.0, min(+1.0, samples[n])));
          buffer = (u8*)output;
        } else {
          //output silence for unsupported sample formats
          bufferFlags = AUDCLNT_BUFFERFLAGS_SILENT;
          break;
        }
      }
      self.renderClient->ReleaseBuffer(length, bufferFlags);
    }
  }

  bool isReady = false;

  u32 mode = 0;
  u32 precision = 0;

  struct Queue {
    f64 samples[65536][8];
    u16 read;
    u16 write;
    u16 count;
  } queue;

  IMMDeviceEnumerator* enumerator = nullptr;
  IMMDevice* audioDevice = nullptr;
  IAudioClient* audioClient = nullptr;
  IAudioRenderClient* renderClient = nullptr;
  HANDLE eventHandle = nullptr;
  HANDLE taskHandle = nullptr;
  REFERENCE_TIME devicePeriod = 0;
  u32 bufferSize = 0;  //in frames
};
