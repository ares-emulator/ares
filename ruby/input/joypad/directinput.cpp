#pragma once

auto CALLBACK DirectInput_EnumJoypadsCallback(const DIDEVICEINSTANCE* instance, void* p) -> BOOL;
auto CALLBACK DirectInput_EnumJoypadAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, void* p) -> BOOL;
auto CALLBACK DirectInput_EnumJoypadEffectsCallback(const DIDEVICEOBJECTINSTANCE* instance, void* p) -> BOOL;

struct InputJoypadDirectInput {
  Input& input;
  InputJoypadDirectInput(Input& input) : input(input) {}

  struct Joypad {
    std::shared_ptr<HID::Joypad> hid = std::make_shared<HID::Joypad>();

    LPDIRECTINPUTDEVICE8 device = nullptr;
    LPDIRECTINPUTEFFECT effect = nullptr;

    u32 pathID = 0;
    u16 vendorID = 0;
    u16 productID = 0;
    bool isXInputDevice = false;
  };
  std::vector<Joypad> joypads;

  uintptr handle = 0;
  LPDIRECTINPUT8 context = nullptr;
  LPDIRECTINPUTDEVICE8 device = nullptr;
  bool xinputAvailable = false;
  u32 effects = 0;

  auto assign(std::shared_ptr<HID::Joypad> hid, u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<std::shared_ptr<HID::Device>>& devices) -> void {
    for(auto& jp : joypads) {
      if(FAILED(jp.device->Poll())) jp.device->Acquire();

      DIJOYSTATE2 state;
      if(FAILED(jp.device->GetDeviceState(sizeof(DIJOYSTATE2), &state))) continue;

      for(u32 n : range(4)) {
        assign(jp.hid, HID::Joypad::GroupID::Axis, 0, state.lX);
        assign(jp.hid, HID::Joypad::GroupID::Axis, 1, state.lY);
        assign(jp.hid, HID::Joypad::GroupID::Axis, 2, state.lZ);
        assign(jp.hid, HID::Joypad::GroupID::Axis, 3, state.lRx);
        assign(jp.hid, HID::Joypad::GroupID::Axis, 4, state.lRy);
        assign(jp.hid, HID::Joypad::GroupID::Axis, 5, state.lRz);

        u32 pov = state.rgdwPOV[n];
        s16 xaxis = 0;
        s16 yaxis = 0;

        if(pov < 36000) {
          if(pov >= 31500 || pov <=  4500) yaxis = -32767;
          if(pov >=  4500 && pov <= 13500) xaxis = +32767;
          if(pov >= 13500 && pov <= 22500) yaxis = +32767;
          if(pov >= 22500 && pov <= 31500) xaxis = -32767;
        }

        assign(jp.hid, HID::Joypad::GroupID::Hat, n * 2 + 0, xaxis);
        assign(jp.hid, HID::Joypad::GroupID::Hat, n * 2 + 1, yaxis);
      }

      for(u32 n : range(128)) {
        assign(jp.hid, HID::Joypad::GroupID::Button, n, (bool)state.rgbButtons[n]);
      }

      devices.push_back(jp.hid);
    }
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool {
    for(auto& jp : joypads) {
      if(jp.hid->id() != id) continue;
      if(jp.effect == nullptr) continue;

      auto enable = weak > 0 || strong > 0;
      if(enable) jp.effect->Start(1, 0);
      else jp.effect->Stop();
      return true;
    }

    return false;
  }

  auto initialize(uintptr handle, LPDIRECTINPUT8 context, bool xinputAvailable) -> bool {
    if(!handle) return false;
    this->handle = handle;
    this->context = context;
    this->xinputAvailable = xinputAvailable;
    context->EnumDevices(DI8DEVCLASS_GAMECTRL, DirectInput_EnumJoypadsCallback, (void*)this, DIEDFL_ATTACHEDONLY);
    return true;
  }

  auto terminate() -> void {
    for(auto& jp : joypads) {
      jp.device->Unacquire();
      if(jp.effect) jp.effect->Release();
      jp.device->Release();
    }
    joypads.clear();
    context = nullptr;
  }

  auto initJoypad(const DIDEVICEINSTANCE* instance) -> bool {
    Joypad jp;
    jp.vendorID = instance->guidProduct.Data1 >> 0;
    jp.productID = instance->guidProduct.Data1 >> 16;
    jp.isXInputDevice = false;
    if(auto device = rawinput.find(jp.vendorID, jp.productID)) {
      jp.isXInputDevice = device().isXInputDevice;
    }

    //Microsoft has intentionally imposed artificial restrictions on XInput devices when used with DirectInput
    //a) the two triggers are merged into a single axis, making uniquely distinguishing them impossible
    //b) rumble support is not exposed
    //thus, it's always preferred to let the XInput driver handle these joypads
    //but if the driver is not available (XInput 1.3 does not ship with stock Windows XP), fall back on DirectInput
    if(jp.isXInputDevice && xinputAvailable) return DIENUM_CONTINUE;

    if(FAILED(context->CreateDevice(instance->guidInstance, &device, 0))) return DIENUM_CONTINUE;
    jp.device = device;
    device->SetDataFormat(&c_dfDIJoystick2);
    device->SetCooperativeLevel((HWND)handle, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);

    effects = 0;
    device->EnumObjects(DirectInput_EnumJoypadAxesCallback, (void*)this, DIDFT_ABSAXIS);
    device->EnumObjects(DirectInput_EnumJoypadEffectsCallback, (void*)this, DIDFT_FFACTUATOR);
    jp.hid->setRumble(effects > 0);

    DIPROPGUIDANDPATH property;
    memset(&property, 0, sizeof(DIPROPGUIDANDPATH));
    property.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    property.diph.dwObj = 0;
    property.diph.dwHow = DIPH_DEVICE;
    device->GetProperty(DIPROP_GUIDANDPATH, &property.diph);
    string devicePath = (const char*)utf8_t(property.wszPath);
    jp.pathID = Hash::CRC32(devicePath).value();
    jp.hid->setVendorID(jp.vendorID);
    jp.hid->setProductID(jp.productID);
    jp.hid->setPathID(jp.pathID);

    if(jp.hid->rumble()) {
      //disable auto-centering spring for rumble support
      DIPROPDWORD property;
      memset(&property, 0, sizeof(DIPROPDWORD));
      property.diph.dwSize = sizeof(DIPROPDWORD);
      property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
      property.diph.dwObj = 0;
      property.diph.dwHow = DIPH_DEVICE;
      property.dwData = false;
      device->SetProperty(DIPROP_AUTOCENTER, &property.diph);

      DWORD dwAxes[2] = {(DWORD)DIJOFS_X, (DWORD)DIJOFS_Y};
      LONG lDirection[2] = {0, 0};
      DICONSTANTFORCE force;
      force.lMagnitude = DI_FFNOMINALMAX;  //full force
      DIEFFECT effect;
      memset(&effect, 0, sizeof(DIEFFECT));
      effect.dwSize = sizeof(DIEFFECT);
      effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
      effect.dwDuration = INFINITE;
      effect.dwSamplePeriod = 0;
      effect.dwGain = DI_FFNOMINALMAX;
      effect.dwTriggerButton = DIEB_NOTRIGGER;
      effect.dwTriggerRepeatInterval = 0;
      effect.cAxes = 2;
      effect.rgdwAxes = dwAxes;
      effect.rglDirection = lDirection;
      effect.lpEnvelope = 0;
      effect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
      effect.lpvTypeSpecificParams = &force;
      effect.dwStartDelay = 0;
      device->CreateEffect(GUID_ConstantForce, &effect, &jp.effect, NULL);
    }

    for(auto n : range(6)) jp.hid->axes().append(n);
    for(auto n : range(8)) jp.hid->hats().append(n);
    for(auto n : range(128)) jp.hid->buttons().append(n);
    joypads.push_back(jp);

    return DIENUM_CONTINUE;
  }

  auto initAxis(const DIDEVICEOBJECTINSTANCE* instance) -> bool {
    DIPROPRANGE range;
    memset(&range, 0, sizeof(DIPROPRANGE));
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwHow = DIPH_BYID;
    range.diph.dwObj = instance->dwType;
    range.lMin = -32768;
    range.lMax = +32767;
    device->SetProperty(DIPROP_RANGE, &range.diph);
    return DIENUM_CONTINUE;
  }

  auto initEffect(const DIDEVICEOBJECTINSTANCE* instance) -> bool {
    effects++;
    return DIENUM_CONTINUE;
  }
};

auto CALLBACK DirectInput_EnumJoypadsCallback(const DIDEVICEINSTANCE* instance, void* p) -> BOOL {
  return ((InputJoypadDirectInput*)p)->initJoypad(instance);
}

auto CALLBACK DirectInput_EnumJoypadAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, void* p) -> BOOL {
  return ((InputJoypadDirectInput*)p)->initAxis(instance);
}

auto CALLBACK DirectInput_EnumJoypadEffectsCallback(const DIDEVICEOBJECTINSTANCE* instance, void* p) -> BOOL {
  return ((InputJoypadDirectInput*)p)->initEffect(instance);
}
