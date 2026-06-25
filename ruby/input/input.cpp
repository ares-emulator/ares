#include <ruby/input/sdl.cpp>

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

auto Input::poll() -> std::vector<std::shared_ptr<nall::HID::Device>> {
  return instance->poll();
}

auto Input::rumble(u64 id, u16 strong, u16 weak) -> bool {
  return instance->rumble(id, strong, weak);
}

//

auto Input::onChange(const std::function<void (std::shared_ptr<HID::Device>, u32, u32, s16, s16)>& onChange) -> void {
  change = onChange;
}

auto Input::doChange(std::shared_ptr<HID::Device> device, u32 group, u32 input, s16 oldValue, s16 newValue) -> void {
  if(change) change(device, group, input, oldValue, newValue);
}

//

auto Input::create(string driver) -> bool {
  self.instance.reset();

  self.instance = std::make_unique<InputSDL>(*this);

  if(!self.instance) self.instance = std::make_unique<InputDriver>(*this);

  return self.instance->create();
}

}
