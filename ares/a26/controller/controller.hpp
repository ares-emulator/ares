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
  virtual auto readAnalog(n1 index) -> AnalogConnection { return AnalogConnection::disconnected(); }
  virtual auto serialize(serializer&) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
#include "booster-grip/booster-grip.hpp"
#include "sega-genesis/sega-genesis.hpp"
#include "joy2b-plus/joy2b-plus.hpp"
#include "paddles/paddles.hpp"
#include "driving/driving.hpp"
#include "keyboard/keyboard.hpp"
#include "relative-pointing/relative-pointing.hpp"
#include "trak-ball/trak-ball.hpp"
#include "atari-mouse/atari-mouse.hpp"
#include "amiga-mouse/amiga-mouse.hpp"
#include "xg1-light-gun/xg1-light-gun.hpp"
