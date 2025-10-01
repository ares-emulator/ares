#pragma once

#include <usbhid.h>
#include <dev/usb/usbhid.h>
#include <dev/usb/usb_ioctl.h>

struct InputJoypadUHID {
  Input& input;
  InputJoypadUHID(Input& input) : input(input) {}

  struct Joypad {
    std::shared_ptr<HID::Joypad> hid = std::make_shared<HID::Joypad>();
    s32 fd = -1;
    report_desc_t report;
    s32 reportID = -1;
    s32 reportSize = -1;
    u8* buffer = nullptr;
    bool writable = false;
  };
  std::vector<Joypad> joypads;

  auto assign(Joypad& joypad, u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = joypad.hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(joypad.hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<std::shared_ptr<HID::Device>>& devices) -> void {
    //hotplug support
    u64 thisTimestamp = chrono::millisecond();
    if(thisTimestamp - lastTimestamp >= 2000) {
      lastTimestamp = thisTimestamp;
      auto devices = directory::files("/dev/", "uhid*");
      if(enumeratedDevices != nall::merge(devices, ";")) initialize();
    }

    for(auto& joypad : joypads) {
      bool isXbox = joypad.hid->vendorID() == 0x045e && joypad.hid->productID() == 0x028e;
      s16 hat[2] = {0, 0};
      u32 axis = 0;
      u32 button = 0;
      while(read(joypad.fd, joypad.buffer, joypad.reportSize) == joypad.reportSize) {
        auto parse = hid_start_parse(joypad.report, 1 << hid_input, joypad.reportID);
        hid_item_t item;
        while(hid_get_item(parse, &item) > 0) {
          if(item.kind != hid_input) continue;
          if(HID_PAGE(item.usage) == HUP_GENERIC_DESKTOP) {
            auto usage = HID_USAGE(item.usage);
            auto value = hid_get_data(joypad.buffer, &item);
            if(value == -32768) value = -32767;

            //Xbox 360 hat support
            if(usage == 0x90 && value == 1) { hat[1] = -32767; continue; }  //HatUp
            if(usage == 0x91 && value == 1) { hat[1] = +32767; continue; }  //HatDown
            if(usage == 0x93 && value == 1) { hat[0] = -32767; continue; }  //HatLeft
            if(usage == 0x92 && value == 1) { hat[0] = +32767; continue; }  //HatRight

            if(isXbox) {
              if(usage == HUG_X ) assign(joypad, HID::Joypad::GroupID::Axis, 0, +value);  //LeftThumbX
              if(usage == HUG_Y ) assign(joypad, HID::Joypad::GroupID::Axis, 1, -value);  //LeftThumbY
              if(usage == HUG_RX) assign(joypad, HID::Joypad::GroupID::Axis, 2, +value);  //RightThumbX
              if(usage == HUG_RY) assign(joypad, HID::Joypad::GroupID::Axis, 3, -value);  //RightTHumbY
              if(usage == HUG_Z ) assign(joypad, HID::Joypad::GroupID::Trigger, 0, value << 7 | value >> 1);
              if(usage == HUG_RZ) assign(joypad, HID::Joypad::GroupID::Trigger, 1, value << 7 | value >> 1);
            } else if(usage == HUG_X  || usage == HUG_Y  || usage == HUG_Z
                   || usage == HUG_RX || usage == HUG_RY || usage == HUG_RZ
            ) {
              if(axis < joypad.hid->axes().size()) {
                assign(joypad, HID::Joypad::GroupID::Axis, axis++, value);
              }
            } else if(usage == HUG_HAT_SWITCH) {
              //todo
            }
          }
          if(HID_PAGE(item.usage) == HUP_BUTTON) {
            auto value = hid_get_data(joypad.buffer, &item);
            if(button < joypad.hid->buttons().size()) {
              assign(joypad, HID::Joypad::GroupID::Button, button++, value);
            }
          }
        }
        if(isXbox) {
          assign(joypad, HID::Joypad::GroupID::Hat, 0, hat[0]);
          assign(joypad, HID::Joypad::GroupID::Hat, 1, hat[1]);
        }
        hid_end_parse(parse);
      }
      devices.push_back(joypad.hid);
    }
  }

  auto initialize() -> bool {
    terminate();

    u32 pathID = 0;
    auto devices = directory::files("/dev/", "uhid*");
    enumeratedDevices = nall::merge(devices, ";");
    for(auto device : devices) {
      Joypad joypad;
      string deviceName = {"/dev/", device};
      if(inode::writable(deviceName)) {
        //needed to use hid_set_report and write on fd
        joypad.fd = open(deviceName, O_RDWR | O_NONBLOCK);
        joypad.writable = true;
      } else if(inode::readable(deviceName)) {
        joypad.fd = open(deviceName, O_RDONLY | O_NONBLOCK);
        joypad.writable = false;
      } else {
        continue;
      }

      //device files use global state: if two processes open the same device file, the driver can crash the process.
      //this can easily happen by opening two instances of a program utilizing this driver, for instance.
      //to prevent this, request an exclusive access lock on the file, released via terminate() or on error below.
      if(flock(joypad.fd, LOCK_EX) != 0) {  //0 = success; -1 = error
        close(joypad.fd);
        continue;
      }

      joypad.report = hid_get_report_desc(joypad.fd);
      joypad.reportID = hid_get_report_id(joypad.fd);
      joypad.reportSize = hid_report_size(joypad.report, hid_input, joypad.reportID);
      joypad.buffer = new u8[joypad.reportSize];
      auto parse = hid_start_parse(joypad.report, 1 << hid_input, joypad.reportID);
      hid_item_t item;
      bool isJoypad = false;
      u32 axes = 0;
      u32 hats = 0;
      u32 buttons = 0;
      bool xbox360[4] = {0, 0, 0, 0};
      while(hid_get_item(parse, &item)) {
        if(item.kind == hid_collection && HID_PAGE(item.usage) == HUP_GENERIC_DESKTOP) {
          if(HID_USAGE(item.usage) == HUG_JOYSTICK) isJoypad = true;
          if(HID_USAGE(item.usage) == HUG_GAME_PAD) isJoypad = true;
          if(HID_USAGE(item.usage) == HUG_POINTER ) isJoypad = true;  //PS3 sixaxis
        }
        if(item.kind == hid_input && HID_PAGE(item.usage) == HUP_GENERIC_DESKTOP) {
          auto usage = HID_USAGE(item.usage);
          if(usage == HUG_X  || usage == HUG_Y  || usage == HUG_Z ) axes++;
          if(usage == HUG_RX || usage == HUG_RY || usage == HUG_RZ) axes++;
          if(usage == HUG_HAT_SWITCH) hats++;
          if(usage >= 0x90 && usage <= 0x93) xbox360[usage - 0x90] = 1;
        }
        if(item.kind == hid_input && HID_PAGE(item.usage) == HUP_BUTTON) {
          buttons++;
        }
      }
      hid_end_parse(parse);

      if(!isJoypad) {
        flock(joypad.fd, LOCK_UN);
        close(joypad.fd);
        continue;
      }

      if(xbox360[0] && xbox360[1] && xbox360[2] && xbox360[3]) {
        joypad.hid->setVendorID(0x045e);
        joypad.hid->setProductID(0x028e);
        joypad.hid->setPathID(pathID++);
        joypad.hid->axes().append("LeftThumbX");
        joypad.hid->axes().append("LeftThumbY");
        joypad.hid->axes().append("RightThumbX");
        joypad.hid->axes().append("RightThumbY");
        joypad.hid->hats().append("HatX");
        joypad.hid->hats().append("HatY");
        joypad.hid->triggers().append("LeftTrigger");
        joypad.hid->triggers().append("RightTrigger");
        joypad.hid->buttons().append("Start");
        joypad.hid->buttons().append("Back");
        joypad.hid->buttons().append("LeftThumb");
        joypad.hid->buttons().append("RightThumb");
        joypad.hid->buttons().append("LeftShoulder");
        joypad.hid->buttons().append("RightShoulder");
        joypad.hid->buttons().append("Guide");
        joypad.hid->buttons().append("A");
        joypad.hid->buttons().append("B");
        joypad.hid->buttons().append("X");
        joypad.hid->buttons().append("Y");
      } else {
        joypad.hid->setVendorID(HID::Joypad::GenericVendorID);
        joypad.hid->setProductID(HID::Joypad::GenericProductID);
        joypad.hid->setPathID(pathID++);
        for(u32 n : range(axes)) joypad.hid->axes().append(n);
        for(u32 n : range(axes)) joypad.hid->hats().append(n);
        for(u32 n : range(axes)) joypad.hid->buttons().append(n);
      }

      joypads.push_back(joypad);
    }

    lastTimestamp = chrono::millisecond();
    return true;
  }

  auto terminate() -> void {
    for(auto& joypad : joypads) {
      delete[] joypad.buffer;
      flock(joypad.fd, LOCK_UN);
      close(joypad.fd);
    }
    joypads.clear();
  }

private:
  u64 lastTimestamp = 0;
  string enumeratedDevices;
};
