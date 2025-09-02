#pragma once

struct InputJoypadUdev {
  Input& input;
  InputJoypadUdev(Input& input) : input(input) {}

  udev* context = nullptr;
  udev_monitor* monitor = nullptr;
  udev_enumerate* enumerator = nullptr;
  udev_list_entry* devices = nullptr;
  udev_list_entry* item = nullptr;

  struct JoypadInput {
    s32 code = 0;
    u32 id = 0;
    s16 value = 0;
    input_absinfo info;

    JoypadInput() {}
    JoypadInput(s32 code) : code(code) {}
    JoypadInput(s32 code, u32 id) : code(code), id(id) {}
    bool operator< (const JoypadInput& source) const { return code <  source.code; }
    bool operator==(const JoypadInput& source) const { return code == source.code; }
  };

  struct Joypad {
    shared_pointer<HID::Joypad> hid{new HID::Joypad};

    s32 fd = -1;
    dev_t device = 0;
    string deviceName;
    string deviceNode;

    u8 evbit[(EV_MAX + 7) / 8] = {0};
    u8 keybit[(KEY_MAX + 7) / 8] = {0};
    u8 absbit[(ABS_MAX + 7) / 8] = {0};
    u8 ffbit[(FF_MAX + 7) / 8] = {0};
    u32 effects = 0;

    string name;
    string manufacturer;
    string product;
    string serial;
    string vendorID;
    string productID;

    set<JoypadInput> axes;
    set<JoypadInput> hats;
    set<JoypadInput> buttons;
    bool rumble = false;
    s32 effectID = -1;
  };
  vector<Joypad> joypads;

  auto assign(shared_pointer<HID::Joypad> hid, u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<shared_pointer<HID::Device>>& devices) -> void {
    while(hotplugDevicesAvailable()) hotplugDevice();

    for(auto& jp : joypads) {
      input_event events[32];
      s32 length = 0;
      while((length = read(jp.fd, events, sizeof(events))) > 0) {
        length /= sizeof(input_event);
        for(u32 i : range(length)) {
          s32 code = events[i].code;
          s32 type = events[i].type;
          s32 value = events[i].value;

          if(type == EV_ABS) {
            if(auto input = jp.axes.find({code})) {
              s32 range = input().info.maximum - input().info.minimum;
              value = (value - input().info.minimum) * 65535ll / range - 32767;
              assign(jp.hid, HID::Joypad::GroupID::Axis, input().id, sclamp<16>(value));
            } else if(auto input = jp.hats.find({code})) {
              s32 range = input().info.maximum - input().info.minimum;
              value = (value - input().info.minimum) * 65535ll / range - 32767;
              assign(jp.hid, HID::Joypad::GroupID::Hat, input().id, sclamp<16>(value));
            }
          } else if(type == EV_KEY) {
            if(code >= BTN_MISC) {
              if(auto input = jp.buttons.find({code})) {
                assign(jp.hid, HID::Joypad::GroupID::Button, input().id, (bool)value);
              }
            }
          }
        }
      }

      devices.append(jp.hid);
    }
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool {
    for(auto& jp : joypads) {
      if(jp.hid->id() != id) continue;
      if(!jp.hid->rumble()) continue;

      if(weak == 0 && strong == 0) {
        if(jp.effectID == -1) return true;  //already stopped?

        ioctl(jp.fd, EVIOCRMFF, jp.effectID);
        jp.effectID = -1;
      } else {
        if(jp.effectID != -1) return true;  //already started?

        ff_effect effect;
        memory::fill(&effect, sizeof(ff_effect));
        effect.type = FF_RUMBLE;
        effect.id = -1;
        effect.u.rumble.strong_magnitude = strong;
        effect.u.rumble.weak_magnitude   = weak;
        ioctl(jp.fd, EVIOCSFF, &effect);
        jp.effectID = effect.id;

        input_event play;
        memory::fill(&play, sizeof(input_event));
        play.type = EV_FF;
        play.code = jp.effectID;
        play.value = weak > 0 || strong > 0;
        (void)write(jp.fd, &play, sizeof(input_event));
      }

      return true;
    }

    return false;
  }

  auto initialize() -> bool {
    context = udev_new();
    if(context == nullptr) return false;

    monitor = udev_monitor_new_from_netlink(context, "udev");
    if(monitor) {
      udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", nullptr);
      udev_monitor_enable_receiving(monitor);
    }

    enumerator = udev_enumerate_new(context);
    if(enumerator) {
      udev_enumerate_add_match_property(enumerator, "ID_INPUT_JOYSTICK", "1");
      udev_enumerate_scan_devices(enumerator);
      devices = udev_enumerate_get_list_entry(enumerator);
      for(udev_list_entry* item = devices; item != nullptr; item = udev_list_entry_get_next(item)) {
        string name = udev_list_entry_get_name(item);
        udev_device* device = udev_device_new_from_syspath(context, name);
        string deviceNode = udev_device_get_devnode(device);
        if(deviceNode) createJoypad(device, deviceNode);
        udev_device_unref(device);
      }
    }

    return true;
  }

  auto terminate() -> void {
    if(enumerator) { udev_enumerate_unref(enumerator); enumerator = nullptr; }
  }

private:
  auto hotplugDevicesAvailable() -> bool {
    pollfd fd = {0};
    fd.fd = udev_monitor_get_fd(monitor);
    fd.events = POLLIN;
    return (::poll(&fd, 1, 0) == 1) && (fd.revents & POLLIN);
  }

  auto hotplugDevice() -> void {
    udev_device* device = udev_monitor_receive_device(monitor);
    if(device == nullptr) return;

    string value = udev_device_get_property_value(device, "ID_INPUT_JOYSTICK");
    string action = udev_device_get_action(device);
    string deviceNode = udev_device_get_devnode(device);
    if(value == "1") {
      if(action == "add") {
        createJoypad(device, deviceNode);
      }
      if(action == "remove") {
        removeJoypad(device, deviceNode);
      }
    }
  }

  auto createJoypad(udev_device* device, const string& deviceNode) -> void {
    Joypad jp;
    jp.deviceNode = deviceNode;

    struct stat st;
    if(stat(deviceNode, &st) < 0) return;
    jp.device = st.st_rdev;

    jp.fd = open(deviceNode, O_RDWR | O_NONBLOCK);
    if(jp.fd < 0) return;

    u8 evbit[(EV_MAX + 7) / 8] = {0};
    u8 keybit[(KEY_MAX + 7) / 8] = {0};
    u8 absbit[(ABS_MAX + 7) / 8] = {0};

    ioctl(jp.fd, EVIOCGBIT(0, sizeof(jp.evbit)), jp.evbit);
    ioctl(jp.fd, EVIOCGBIT(EV_KEY, sizeof(jp.keybit)), jp.keybit);
    ioctl(jp.fd, EVIOCGBIT(EV_ABS, sizeof(jp.absbit)), jp.absbit);
    ioctl(jp.fd, EVIOCGBIT(EV_FF, sizeof(jp.ffbit)), jp.ffbit);
    ioctl(jp.fd, EVIOCGEFFECTS, &jp.effects);

    #define testBit(buffer, bit) (buffer[(bit) >> 3] & 1 << ((bit) & 7))

    if(testBit(jp.evbit, EV_KEY)) {
      if(udev_device* parent = udev_device_get_parent_with_subsystem_devtype(device, "input", nullptr)) {
        jp.name = udev_device_get_sysattr_value(parent, "name");
        jp.vendorID = udev_device_get_sysattr_value(parent, "id/vendor");
        jp.productID = udev_device_get_sysattr_value(parent, "id/product");
        if(udev_device* root = udev_device_get_parent_with_subsystem_devtype(parent, "usb", "usb_device")) {
          if(jp.vendorID == udev_device_get_sysattr_value(root, "idVendor")
          && jp.productID == udev_device_get_sysattr_value(root, "idProduct")
          ) {
            jp.deviceName = udev_device_get_devpath(root);
            jp.manufacturer = udev_device_get_sysattr_value(root, "manufacturer");
            jp.product = udev_device_get_sysattr_value(root, "product");
            jp.serial = udev_device_get_sysattr_value(root, "serial");
          }
        }
      }

      u32 axes = 0;
      u32 hats = 0;
      u32 buttons = 0;
      for(s32 i = 0; i < ABS_MISC; i++) {
        if(testBit(jp.absbit, i)) {
          if(i >= ABS_HAT0X && i <= ABS_HAT3Y) {
            if(auto hat = jp.hats.insert({i, hats++})) {
              ioctl(jp.fd, EVIOCGABS(i), &hat().info);
            }
          } else {
            if(auto axis = jp.axes.insert({i, axes++})) {
              ioctl(jp.fd, EVIOCGABS(i), &axis().info);
            }
          }
        }
      }
      for(s32 i = BTN_JOYSTICK; i < KEY_MAX; i++) {
        if(testBit(jp.keybit, i)) {
          jp.buttons.insert({i, buttons++});
        }
      }
      for(s32 i = BTN_MISC; i < BTN_JOYSTICK; i++) {
        if(testBit(jp.keybit, i)) {
          jp.buttons.insert({i, buttons++});
        }
      }
      jp.rumble = jp.effects >= 2 && testBit(jp.ffbit, FF_RUMBLE);

      createJoypadHID(jp);
      joypads.append(jp);
    }

    #undef testBit
  }

  auto createJoypadHID(Joypad& jp) -> void {
    jp.hid->setVendorID(jp.vendorID.hex());
    jp.hid->setProductID(jp.productID.hex());
    jp.hid->setPathID(Hash::CRC32(jp.deviceName).value());

    for(u32 n : range(jp.axes.size())) jp.hid->axes().append(n);
    for(u32 n : range(jp.hats.size())) jp.hid->hats().append(n);
    for(u32 n : range(jp.buttons.size())) jp.hid->buttons().append(n);
    jp.hid->setRumble(jp.rumble);
  }

  auto removeJoypad(udev_device* device, const string& deviceNode) -> void {
    for(u32 n : range(joypads.size())) {
      if(joypads[n].deviceNode == deviceNode) {
        close(joypads[n].fd);
        joypads.remove(n);
        return;
      }
    }
  }
};
