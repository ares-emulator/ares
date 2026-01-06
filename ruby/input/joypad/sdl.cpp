#pragma once
#include ".deps/ares-deps-windows-x64/include/SDL3/SDL_joystick.h"

struct InputJoypadSDL {
  Input& input;
  InputJoypadSDL(Input& input) : input(input) {}

  struct Joypad {
    std::shared_ptr<HID::Joypad> hid = std::make_shared<HID::Joypad>();
    std::vector<bool> axisPolled;

    u32 id = 0;
    SDL_Joystick* handle = nullptr;
  };
  std::vector<Joypad> joypads;

  auto assign(Joypad& joypad, u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = joypad.hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    if(groupID == HID::Joypad::GroupID::Axis && (inputID < joypad.axisPolled.size()) && !joypad.axisPolled[inputID]) {
      //suppress the first axis polling event, because the value can change dramatically.
      //SDL seems to return 0 for all axes, until the first movement, where it jumps to the real value.
      //this triggers onChange handlers to instantly bind inputs erroneously if not suppressed.
      joypad.axisPolled[inputID] = true;
    } else {
      input.doChange(joypad.hid, groupID, inputID, group.input(inputID).value(), value);
    }
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<std::shared_ptr<HID::Device>>& devices) -> void {
    SDL_UpdateJoysticks();
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_EVENT_JOYSTICK_ADDED || event.type == SDL_EVENT_JOYSTICK_REMOVED) {
        enumerate();
      }
    }

    for(auto& jp : joypads) {
      for(u32 n : range(jp.hid->axes().size())) {
        assign(jp, HID::Joypad::GroupID::Axis, n, (s16)SDL_GetJoystickAxis(jp.handle, n));
      }

      for(s32 n = 0; n < (s32)jp.hid->hats().size() - 1; n += 2) {
        u8 state = SDL_GetJoystickHat(jp.handle, n >> 1);
        assign(jp, HID::Joypad::GroupID::Hat, n + 0, state & SDL_HAT_LEFT ? -32767 : state & SDL_HAT_RIGHT ? +32767 : 0);
        assign(jp, HID::Joypad::GroupID::Hat, n + 1, state & SDL_HAT_UP   ? -32767 : state & SDL_HAT_DOWN  ? +32767 : 0);
      }

      for(u32 n : range(jp.hid->buttons().size())) {
        assign(jp, HID::Joypad::GroupID::Button, n, (bool)SDL_GetJoystickButton(jp.handle, n));
      }

      devices.push_back(jp.hid);
    }
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool {
    for(auto& jp : joypads) {
      if(jp.hid->id() != id) continue;

      SDL_RumbleJoystick(jp.handle, strong, weak, 0);
      return true;
    }

    return false;
  }

  auto initialize() -> bool {
    terminate();
    SDL_Init(SDL_INIT_EVENTS);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    //SDL_JoystickEventState(1);
    enumerate();
    return true;
  }

  auto terminate() -> void {
    for(auto& jp : joypads) {
      SDL_CloseJoystick(jp.handle);
    }
    joypads.clear();
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  }

private:
  auto crc32(const string& s) -> u32 {
    return Hash::CRC32({(const u8*)s.data(), s.size()}).value();
  }

  static auto guidString(SDL_Joystick* js) -> string {
    SDL_GUID guid = SDL_GetJoystickGUID(js);
    char buffer[64]{};
    SDL_GUIDToString(guid, buffer, sizeof(buffer));
    return buffer;
  }

  auto enumerate() -> void {
    joypads.clear();
    int num_joysticks;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);

    // Track the number of devices per model to assign unique path IDs
    std::unordered_map<u32, u32> deviceSlotIndex;

    for(int i = 0; i < num_joysticks; i++) {
      SDL_JoystickID id = joysticks[i];
      Joypad jp;
      jp.id = id;
      jp.handle = SDL_OpenJoystick(jp.id);
      if(!jp.handle) {
        const char *err = SDL_GetError();
        print("Error opening SDL joystick id ", id, ": ", err);
        continue;
      }

      s32 axes = SDL_GetNumJoystickAxes(jp.handle);
      s32 hats = SDL_GetNumJoystickHats(jp.handle) * 2;
      s32 buttons = SDL_GetNumJoystickButtons(jp.handle);
      if(axes < 0 || hats < 0 || buttons < 0) {
        const char *err = SDL_GetError();
        print("Error retrieving SDL joystick information for device ", jp.handle, " at index ", id, ": ", err);
        continue;
      }

      u16 vid = SDL_GetJoystickVendor(jp.handle);
      u16 pid = SDL_GetJoystickProduct(jp.handle);
      if(vid == 0) vid = HID::Joypad::GenericVendorID;
      if(pid == 0) pid = HID::Joypad::GenericProductID;

      string path = "";
      if(const char* serial = SDL_GetJoystickSerial(jp.handle); serial && *serial) {
        path = string{"SER:", serial, "|VID:", vid, "|PID:", pid};
      } else if(const char* path = SDL_GetJoystickPath(jp.handle); path && *path) {
        path = string{"PATH:", path, "|VID:", vid, "|PID:", pid};
      } else {
        string modelKey = {"GUID:", guidString(jp.handle), "|VID:", vid, "|PID:", pid};
        u32 modelID = crc32(modelKey);
        u32 slot = deviceSlotIndex[modelID]++;
        path = string{modelKey, "|SLOT:", slot};
      }

      u32 pathID = crc32(path);

      string name = SDL_GetJoystickName(jp.handle);
      if(!name) name = "Joypad";
      int playerIndex = SDL_GetJoystickPlayerIndex(jp.handle) ;
      int index = playerIndex >= 0 ? playerIndex + 1 : SDL_GetJoystickID(jp.handle);;
      jp.hid->setName({name, " ", index});
      jp.hid->setVendorID(vid);
      jp.hid->setProductID(pid);
      jp.hid->setPathID(pathID);
      for(u32 n : range(axes)) jp.hid->axes().append(n);
      for(u32 n : range(hats)) jp.hid->hats().append(n);
      for(u32 n : range(buttons)) jp.hid->buttons().append(n);
      jp.hid->setRumble(true);

      joypads.push_back(jp);
    }

    SDL_free(joysticks);
    SDL_UpdateJoysticks();
  }
};
