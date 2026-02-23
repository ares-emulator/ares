namespace ares::Core {
  struct Object;
  struct System;
  struct Peripheral;
  struct Port;
  struct Tape;
  namespace Component {
    struct Component;
    struct RealTimeClock;
  }
  namespace Video {
    struct Video;
    struct Sprite;
    struct Screen;
  }
  namespace Audio {
    struct Audio;
    struct Stream;
  }
  namespace Input {
    struct Input;
    struct Button;
    struct Axis;
    struct Trigger;
    struct Rumble;
  }
  namespace Setting {
    struct Setting;
    struct Boolean;
    struct Natural;
    struct Integer;
    struct Real;
    struct String;
  }
  namespace Debugger {
    struct Debugger;
    struct Memory;
    struct Graphics;
    struct Properties;
    namespace Tracer {
      struct Tracer;
      struct Notification;
      struct Instruction;
    }
  }
}

namespace ares::Node {
  using Object           = std::shared_ptr<Core::Object>;
  using System           = std::shared_ptr<Core::System>;
  using Peripheral       = std::shared_ptr<Core::Peripheral>;
  using Port             = std::shared_ptr<Core::Port>;
  using Tape             = std::shared_ptr<Core::Tape>;
  namespace Component {
    using Component      = std::shared_ptr<Core::Component::Component>;
    using RealTimeClock  = std::shared_ptr<Core::Component::RealTimeClock>;
  }
  namespace Video {
    using Video          = std::shared_ptr<Core::Video::Video>;
    using Sprite         = std::shared_ptr<Core::Video::Sprite>;
    using Screen         = std::shared_ptr<Core::Video::Screen>;
  }
  namespace Audio {
    using Audio          = std::shared_ptr<Core::Audio::Audio>;
    using Stream         = std::shared_ptr<Core::Audio::Stream>;
  }
  namespace Input {
    using Input          = std::shared_ptr<Core::Input::Input>;
    using Button         = std::shared_ptr<Core::Input::Button>;
    using Axis           = std::shared_ptr<Core::Input::Axis>;
    using Trigger        = std::shared_ptr<Core::Input::Trigger>;
    using Rumble         = std::shared_ptr<Core::Input::Rumble>;
  }
  namespace Setting {
    using Setting        = std::shared_ptr<Core::Setting::Setting>;
    using Boolean        = std::shared_ptr<Core::Setting::Boolean>;
    using Natural        = std::shared_ptr<Core::Setting::Natural>;
    using Integer        = std::shared_ptr<Core::Setting::Integer>;
    using Real           = std::shared_ptr<Core::Setting::Real>;
    using String         = std::shared_ptr<Core::Setting::String>;
  }
  namespace Debugger {
    using Debugger       = std::shared_ptr<Core::Debugger::Debugger>;
    using Memory         = std::shared_ptr<Core::Debugger::Memory>;
    using Graphics       = std::shared_ptr<Core::Debugger::Graphics>;
    using Properties     = std::shared_ptr<Core::Debugger::Properties>;
    namespace Tracer {
      using Tracer       = std::shared_ptr<Core::Debugger::Tracer::Tracer>;
      using Notification = std::shared_ptr<Core::Debugger::Tracer::Notification>;
      using Instruction  = std::shared_ptr<Core::Debugger::Tracer::Instruction>;
    }
  }
}

namespace ares::Core {
  // <ares/platform.hpp> forward declarations
  static auto PlatformAttach(Node::Object) -> void;
  static auto PlatformDetach(Node::Object) -> void;
  static auto PlatformLog(Node::Debugger::Tracer::Tracer tracer, string_view) -> void;

  #include <ares/node/attribute.hpp>
  #include <ares/node/class.hpp>
  #include <ares/node/object.hpp>
  #include <ares/node/system.hpp>
  #include <ares/node/peripheral.hpp>
  #include <ares/node/port.hpp>
  #include <ares/node/tape.hpp>
  namespace Component {
    #include <ares/node/component/component.hpp>
    #include <ares/node/component/real-time-clock.hpp>
  }
  namespace Video {
    #include <ares/node/video/video.hpp>
    #include <ares/node/video/sprite.hpp>
    #include <ares/node/video/screen.hpp>
  }
  namespace Audio {
    #include <ares/node/audio/audio.hpp>
    #include <ares/node/audio/stream.hpp>
  }
  namespace Input {
    #include <ares/node/input/input.hpp>
    #include <ares/node/input/button.hpp>
    #include <ares/node/input/axis.hpp>
    #include <ares/node/input/trigger.hpp>
    #include <ares/node/input/rumble.hpp>
  }
  namespace Setting {
    #include <ares/node/setting/setting.hpp>
    #include <ares/node/setting/boolean.hpp>
    #include <ares/node/setting/natural.hpp>
    #include <ares/node/setting/integer.hpp>
    #include <ares/node/setting/real.hpp>
    #include <ares/node/setting/string.hpp>
  }
  namespace Debugger {
    #include <ares/node/debugger/debugger.hpp>
    #include <ares/node/debugger/memory.hpp>
    #include <ares/node/debugger/graphics.hpp>
    #include <ares/node/debugger/properties.hpp>
    namespace Tracer {
      #include <ares/node/debugger/tracer/tracer.hpp>
      #include <ares/node/debugger/tracer/notification.hpp>
      #include <ares/node/debugger/tracer/instruction.hpp>
    }
  }
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
    auto node = Core::Class::create(document["node"].string());
    node->unserialize(document["node"]);
    return node;
  }

  static inline auto parent(Object child) -> Object {
    if(!child) return {};
    auto wp = child->parent();
    if(wp.expired()) return {};
    return wp.lock();
  }

  template<typename T>
  static inline auto find(Object from, string name) -> Object {
    if(!from) return {};
    if(auto object = from->find<T>(name)) return object;
    return {};
  }

  template<typename T>
  static inline auto enumerate(Object from) -> std::vector<T> {
    std::vector<T> objects;
    if(from) from->enumerate<T>(objects);
    return objects;
  }
}
