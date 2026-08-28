struct Controller {
  Node::Peripheral node;

  struct AnalogConnection {
    enum class Type : u32 { Disconnected, Ground, Vcc };

    static auto disconnected() -> AnalogConnection { return {}; }
    static auto ground(u32 resistance = 0) -> AnalogConnection { return {Type::Ground, resistance}; }
    static auto vcc(u32 resistance = 0) -> AnalogConnection { return {Type::Vcc, resistance}; }

    Type type = Type::Disconnected;
    u32 resistance = 0;
  };

  virtual ~Controller() = default;

  virtual auto poll() -> void {}
  virtual auto read() -> n8 { return 0xff; }
  virtual auto write(n8 data) -> void {}
  virtual auto readAnalogA() -> AnalogConnection { return AnalogConnection::disconnected(); }
  virtual auto readAnalogB() -> AnalogConnection { return AnalogConnection::disconnected(); }
  virtual auto serialize(serializer&) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
#include "paddles/paddles.hpp"
#include "driving/driving.hpp"
#include "keyboard/keyboard.hpp"
