#if defined(INPUT_CARBON)
  #include <ruby/input/carbon.cpp>
#endif

#if defined(INPUT_QUARTZ)
  #include <ruby/input/quartz.cpp>
#endif

#if defined(INPUT_SDL)
  #include <ruby/input/sdl.cpp>
#endif

#if defined(INPUT_UDEV)
  #include <ruby/input/udev.cpp>
#endif

#if defined(INPUT_UHID)
  #include <ruby/input/uhid.cpp>
#endif

#if defined(INPUT_WINDOWS)
  #include <ruby/input/windows.cpp>
#endif

#if defined(INPUT_XLIB)
  #include <ruby/input/xlib.cpp>
#endif

namespace ruby {

auto Input::setContext(uintptr context) -> bool {
  if(instance->context == context) return true;
  if(!instance->hasContext()) return false;
  if(!instance->setContext(instance->context = context)) return false;
  return true;
}

//

auto Input::acquired() -> bool {
  return instance->acquired();
}

auto Input::acquire() -> bool {
  return instance->acquire();
}

auto Input::release() -> bool {
  return instance->release();
}

auto Input::poll() -> std::vector<shared_pointer<nall::HID::Device>> {
  return instance->poll();
}

auto Input::rumble(u64 id, u16 strong, u16 weak) -> bool {
  return instance->rumble(id, strong, weak);
}

//

auto Input::onChange(const std::function<void (shared_pointer<HID::Device>, u32, u32, s16, s16)>& onChange) -> void {
  change = onChange;
}

auto Input::doChange(shared_pointer<HID::Device> device, u32 group, u32 input, s16 oldValue, s16 newValue) -> void {
  if(change) change(device, group, input, oldValue, newValue);
}

//

auto Input::create(string driver) -> bool {
  self.instance.reset();
  if(!driver) driver = optimalDriver();

  #if defined(INPUT_WINDOWS)
  if(driver == "Windows") self.instance = new InputWindows(*this);
  #endif

  #if defined(INPUT_QUARTZ)
  if(driver == "Quartz") self.instance = new InputQuartz(*this);
  #endif

  #if defined(INPUT_CARBON)
  if(driver == "Carbon") self.instance = new InputCarbon(*this);
  #endif

  #if defined(INPUT_UDEV)
  if(driver == "udev") self.instance = new InputUdev(*this);
  #endif

  #if defined(INPUT_UHID)
  if(driver == "uhid") self.instance = new InputUHID(*this);
  #endif

  #if defined(INPUT_SDL)
  if(driver == "SDL") self.instance = new InputSDL(*this);
  #endif

  #if defined(INPUT_XLIB)
  if(driver == "Xlib") self.instance = new InputXlib(*this);
  #endif

  if(!self.instance) self.instance = new InputDriver(*this);

  return self.instance->create();
}

auto Input::hasDrivers() -> std::vector<string> {
  return {

  #if defined(INPUT_WINDOWS)
  "Windows",
  #endif

  #if defined(INPUT_QUARTZ)
  "Quartz",
  #endif

  #if defined(INPUT_CARBON)
  "Carbon",
  #endif

  #if defined(INPUT_UDEV)
  "udev",
  #endif

  #if defined(INPUT_UHID)
  "uhid",
  #endif

  #if defined(INPUT_SDL)
  "SDL",
  #endif

  #if defined(INPUT_XLIB)
  "Xlib",
  #endif

  "None"};
}

auto Input::optimalDriver() -> string {
  #if defined(INPUT_SDL)
  return "SDL";
  #elif defined(INPUT_WINDOWS)
  return "Windows";
  #elif defined(INPUT_QUARTZ)
  return "Quartz";
  #elif defined(INPUT_CARBON)
  return "Carbon";
  #elif defined(INPUT_UDEV)
  return "udev";
  #elif defined(INPUT_UHID)
  return "uhid";
  #elif defined(INPUT_XLIB)
  return "Xlib";
  #else
  return "None";
  #endif
}

auto Input::safestDriver() -> string {
  #if defined(INPUT_WINDOWS)
  return "Windows";
  #elif defined(INPUT_QUARTZ)
  return "Quartz";
  #elif defined(INPUT_CARBON)
  return "Carbon";
  #elif defined(INPUT_UDEV)
  return "udev";
  #elif defined(INPUT_UHID)
  return "uhid";
  #elif defined(INPUT_SDL)
  return "SDL";
  #elif defined(INPUT_XLIB)
  return "Xlib";
  #else
  return "none";
  #endif
}

}
