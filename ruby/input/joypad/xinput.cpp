#pragma once

//documented functionality
#define oXInputGetState  "XInputGetState"
#define oXInputSetState  "XInputSetState"
typedef DWORD WINAPI (*pXInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI (*pXInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);

//undocumented functionality
#define oXInputGetStateEx             (LPCSTR)100
#define oXInputWaitForGuideButton     (LPCSTR)101
#define oXInputCancelGuideButtonWait  (LPCSTR)102
#define oXInputPowerOffController     (LPCSTR)103
typedef DWORD WINAPI (*pXInputGetStateEx)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI (*pXInputWaitForGuideButton)(DWORD dwUserIndex, DWORD dwFlag, void* pUnknown);
typedef DWORD WINAPI (*pXInputCancelGuideButtonWait)(DWORD dwUserIndex);
typedef DWORD WINAPI (*pXInputPowerOffController)(DWORD dwUserIndex);

#define XINPUT_GAMEPAD_GUIDE  0x0400

struct InputJoypadXInput {
  Input& input;
  InputJoypadXInput(Input& input) : input(input) {}

  HMODULE libxinput = nullptr;
  pXInputGetStateEx XInputGetStateEx = nullptr;
  pXInputSetState XInputSetState = nullptr;

  struct Joypad {
    shared_pointer<HID::Joypad> hid{new HID::Joypad};
    u32 id = 0;
  };
  vector<Joypad> joypads;

  auto assign(shared_pointer<HID::Joypad> hid, u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(vector<shared_pointer<HID::Device>>& devices) -> void {
    for(auto& jp : joypads) {
      XINPUT_STATE state;
      if(XInputGetStateEx(jp.id, &state) != ERROR_SUCCESS) continue;

      //flip vertical axes so that -32768 = up, +32767 = down
      u16 axisLY = 32768 + state.Gamepad.sThumbLY;
      u16 axisRY = 32768 + state.Gamepad.sThumbRY;
      assign(jp.hid, HID::Joypad::GroupID::Axis, 0, (s16)state.Gamepad.sThumbLX);
      assign(jp.hid, HID::Joypad::GroupID::Axis, 1, (s16)(~axisLY - 32768));
      assign(jp.hid, HID::Joypad::GroupID::Axis, 2, (s16)state.Gamepad.sThumbRX);
      assign(jp.hid, HID::Joypad::GroupID::Axis, 3, (s16)(~axisRY - 32768));

      s16 hatX = 0;
      s16 hatY = 0;
      if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP   ) hatY -= 32767;
      if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ) hatY += 32767;
      if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ) hatX -= 32767;
      if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) hatX += 32767;

      assign(jp.hid, HID::Joypad::GroupID::Hat, 0, hatX);
      assign(jp.hid, HID::Joypad::GroupID::Hat, 1, hatY);

      //scale trigger ranges for (not-pressed to pressed) from (0 to 255) to (0 to +32767)
      s16 triggerL = state.Gamepad.bLeftTrigger;
      s16 triggerR = state.Gamepad.bRightTrigger;
      triggerL = triggerL << 7 | triggerL >> 1;
      triggerR = triggerR << 7 | triggerR >> 1;

      assign(jp.hid, HID::Joypad::GroupID::Trigger, 0, triggerL);
      assign(jp.hid, HID::Joypad::GroupID::Trigger, 1, triggerR);

      assign(jp.hid, HID::Joypad::GroupID::Button,  0, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_A));
      assign(jp.hid, HID::Joypad::GroupID::Button,  1, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_B));
      assign(jp.hid, HID::Joypad::GroupID::Button,  2, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_X));
      assign(jp.hid, HID::Joypad::GroupID::Button,  3, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y));
      assign(jp.hid, HID::Joypad::GroupID::Button,  4, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK));
      assign(jp.hid, HID::Joypad::GroupID::Button,  5, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_START));
      assign(jp.hid, HID::Joypad::GroupID::Button,  6, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER));
      assign(jp.hid, HID::Joypad::GroupID::Button,  7, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER));
      assign(jp.hid, HID::Joypad::GroupID::Button,  8, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB));
      assign(jp.hid, HID::Joypad::GroupID::Button,  9, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB));
      assign(jp.hid, HID::Joypad::GroupID::Button, 10, (bool)(state.Gamepad.wButtons & XINPUT_GAMEPAD_GUIDE));

      devices.append(jp.hid);
    }
  }

  auto rumble(u64 id, bool enable) -> bool {
    for(auto& jp : joypads) {
      if(jp.hid->id() != id) continue;

      XINPUT_VIBRATION vibration;
      memset(&vibration, 0, sizeof(XINPUT_VIBRATION));
      vibration.wLeftMotorSpeed  = enable ? 65535 : 0;  //low-frequency motor  (0 = off, 65535 = max)
      vibration.wRightMotorSpeed = enable ? 65535 : 0;  //high-frequency motor (0 = off, 65535 = max)
      XInputSetState(jp.id, &vibration);
      return true;
    }

    return false;
  }

  auto initialize() -> bool {
    if(!libxinput) libxinput = LoadLibraryA("xinput1_3.dll");
    if(!libxinput) libxinput = LoadLibraryA("xinput1_4.dll");
    if(!libxinput) return false;

    //XInputGetStateEx is an undocumented function; but is required to get the state of the guide button
    //if for some reason it is not available, fall back on XInputGetState, which takes the same parameters
    XInputGetStateEx = (pXInputGetStateEx)GetProcAddress(libxinput, oXInputGetStateEx);
    XInputSetState = (pXInputSetState)GetProcAddress(libxinput, oXInputSetState);
    if(!XInputGetStateEx) XInputGetStateEx = (pXInputGetStateEx)GetProcAddress(libxinput, oXInputGetState);
    if(!XInputGetStateEx || !XInputSetState) return terminate(), false;

    //XInput supports a maximum of four controllers
    //add all four to devices list now. If they are not connected, they will not show up in poll() results
    for(u32 id : range(4)) {
      Joypad jp;
      jp.id = id;
      jp.hid->setVendorID(0x045e);
      jp.hid->setProductID(0x028e);
      jp.hid->setPathID(id);
      jp.hid->setRumble(true);

      jp.hid->axes().append("LeftThumbX");
      jp.hid->axes().append("LeftThumbY");
      jp.hid->axes().append("RightThumbX");
      jp.hid->axes().append("RightThumbY");

      jp.hid->hats().append("HatX");
      jp.hid->hats().append("HatY");

      jp.hid->triggers().append("LeftTrigger");
      jp.hid->triggers().append("RightTrigger");

      jp.hid->buttons().append("A");
      jp.hid->buttons().append("B");
      jp.hid->buttons().append("X");
      jp.hid->buttons().append("Y");
      jp.hid->buttons().append("Back");
      jp.hid->buttons().append("Start");
      jp.hid->buttons().append("LeftShoulder");
      jp.hid->buttons().append("RightShoulder");
      jp.hid->buttons().append("LeftThumb");
      jp.hid->buttons().append("RightThumb");
      jp.hid->buttons().append("Guide");

      joypads.append(jp);
    }

    return true;
  }

  auto terminate() -> void {
    if(!libxinput) return;

    FreeLibrary(libxinput);
    libxinput = nullptr;
  }
};
