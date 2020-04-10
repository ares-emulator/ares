namespace ares::Core {
  struct Object;
    struct System;
    struct Component;
    struct RealTimeClock;
    struct Peripheral;
    struct Port;
    struct Input;
      struct Button;
      struct Axis;
      struct Trigger;
      struct Rumble;
    struct Settings;
    struct Setting;
      struct Boolean;
      struct Natural;
      struct Integer;
      struct Real;
      struct String;
  //struct Video;
      struct Sprite;
      struct Screen;
  //struct Audio;
      struct Stream;
  //struct Debug;
      struct Memory;
      struct Graphics;
      struct Properties;
      struct Tracer;
        struct Notification;
        struct Instruction;
}

namespace ares::Node {
  using Object             = shared_pointer<Core::Object>;
    using System           = shared_pointer<Core::System>;
    using Component        = shared_pointer<Core::Component>;
    using RealTimeClock    = shared_pointer<Core::RealTimeClock>;
    using Peripheral       = shared_pointer<Core::Peripheral>;
    using Port             = shared_pointer<Core::Port>;
    using Input            = shared_pointer<Core::Input>;
      using Button         = shared_pointer<Core::Button>;
      using Axis           = shared_pointer<Core::Axis>;
      using Trigger        = shared_pointer<Core::Trigger>;
      using Rumble         = shared_pointer<Core::Rumble>;
    using Settings         = shared_pointer<Core::Settings>;
    using Setting          = shared_pointer<Core::Setting>;
      using Boolean        = shared_pointer<Core::Boolean>;
      using Natural        = shared_pointer<Core::Natural>;
      using Integer        = shared_pointer<Core::Integer>;
      using Real           = shared_pointer<Core::Real>;
      using String         = shared_pointer<Core::String>;
  //using Video;
      using Sprite         = shared_pointer<Core::Sprite>;
      using Screen         = shared_pointer<Core::Screen>;
  //using Audio;
      using Stream         = shared_pointer<Core::Stream>;
  //using Debug;
      using Memory         = shared_pointer<Core::Memory>;
      using Graphics       = shared_pointer<Core::Graphics>;
      using Properties     = shared_pointer<Core::Properties>;
      using Tracer         = shared_pointer<Core::Tracer>;
        using Notification = shared_pointer<Core::Notification>;
        using Instruction  = shared_pointer<Core::Instruction>;
}

namespace ares::Core {
  // <emulator/platform.hpp> forward declarations (dependent on <emulator/node.hpp>)
  static auto PlatformAttach(Node::Object) -> void;
  static auto PlatformDetach(Node::Object) -> void;
  static auto PlatformLog(string_view) -> void;

  #include <ares/node/attribute.hpp>
  #include <ares/node/class.hpp>
  #include <ares/node/object.hpp>
  #include <ares/node/system.hpp>
  #include <ares/node/component.hpp>
  #include <ares/node/real-time-clock.hpp>
  #include <ares/node/peripheral.hpp>
  #include <ares/node/port.hpp>
  #include <ares/node/input.hpp>
  #include <ares/node/setting.hpp>
  #include <ares/node/video/sprite.hpp>
  #include <ares/node/video/screen.hpp>
  #include <ares/node/audio/stream.hpp>
  #include <ares/node/debug/memory.hpp>
  #include <ares/node/debug/graphics.hpp>
  #include <ares/node/debug/properties.hpp>
  #include <ares/node/debug/tracer.hpp>
  #include <ares/node/debug/notification.hpp>
  #include <ares/node/debug/instruction.hpp>
}

namespace ares::Node {
  static inline auto create(string identifier) -> Object {
    return Core::Class::create(identifier);
  }

  static inline auto serialize(Object node) -> string {
    if(!node) return {};
    string result;
    node->serialize(result, {});
    return result;
  }

  static inline auto unserialize(string markup) -> Object {
    auto document = BML::unserialize(markup);
    if(!document) return {};
    auto node = Core::Class::create(document["node"].text());
    node->unserialize(document["node"]);
    return node;
  }

  static inline auto parent(Object child) -> Object {
    if(!child || !child->parent()) return {};
    if(auto parent = child->parent().acquire()) return parent;
    return {};
  }

  template<typename T>
  static inline auto find(Object from, string name) -> Object {
    if(!from) return {};
    if(auto object = from->find<T>(name)) return object;
    return {};
  }

  template<typename T>
  static inline auto enumerate(Object from) -> vector<T> {
    vector<T> objects;
    if(from) from->enumerate<T>(objects);
    return objects;
  }
}
