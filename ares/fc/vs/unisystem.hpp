struct VsUniSystem {
  struct Controls {
    Node::Object node;
    Node::Input::Button service;
    Node::Input::Button coins[2];

    auto load(Node::Object parent) -> bool;
    auto unload() -> void;
    auto data(u32 stream) -> n1;
    auto latch() -> std::array<n8, 2>;
    auto serialize(serializer&) -> void;

  private:
    struct Input {
      virtual ~Input() = default;
      virtual auto data(u32 stream) -> n1 = 0;
      virtual auto latch() -> std::array<n8, 2> = 0;
      virtual auto serialize(serializer&) -> void {}
    };

    struct StandardInput : Input {
      enum class Wiring : u32 {
        Standard,
        Swapped,
        SwapAB,
      } wiring;

      struct Player {
        Node::Input::Button up;
        Node::Input::Button down;
        Node::Input::Button left;
        Node::Input::Button right;
        Node::Input::Button a;
        Node::Input::Button b;
        Node::Input::Button start;
      } players[2];
      Node::Input::Button live[2];

      StandardInput(Node::Object parent, Wiring wiring);

      auto data(u32 stream) -> n1 override;
      auto latch() -> std::array<n8, 2> override;

    private:
      auto loadPlayer(Node::Object parent, Player& player, u32 id) -> void;
      auto pollPlayer(u32 player) -> n8;
    };

    struct ZapperInput : Input {
      Node::Object node;
      Node::Input::Axis x;
      Node::Input::Axis y;
      Node::Input::Button trigger;
      LightGun lightGun;

      ZapperInput(Node::Object parent);
      ~ZapperInput();

      auto data(u32 stream) -> n1 override;
      auto latch() -> std::array<n8, 2> override;
      auto serialize(serializer&) -> void override;
    };

    std::unique_ptr<Input> input;
  } controls;

  struct DIPSwitches {
    struct Option {
      string name;
      u8 value = 0;
    };

    struct Setting {
      Node::Setting::String node;
      u8 mask = 0;
      std::vector<Option> options;
    };

    Node::Object node;
    std::vector<Setting> settings;
    n8 value = 0;

    auto load(Node::Object parent) -> bool;
    auto unload() -> void;
    auto update(u32 index, string value) -> void;
    auto serialize(serializer&) -> void;

  private:
    struct Definition {
      string name;
      u8 mask = 0;
      u8 defaultValue = 0;
      std::vector<Option> options;
    };

    auto create(Node::Object parent, std::vector<Definition> definitions) -> void;
  } dipSwitches;

  struct IO {
    struct Pulse {
      bool held = false;
      u8 frames = 0;
    };

    auto power() -> void;
    auto frame() -> void;
    auto read(n16 address, n8 data) -> n8;
    auto write(n8 data) -> void;
    auto serialize(serializer&) -> void;

  private:
    auto readStream(u32 stream) -> n1;
    auto pollPulse(Node::Input::Button input, Pulse& pulse) -> void;

    n8 streams[2] = {};
    u8 positions[2] = {};
    n1 strobe = 0;
    Pulse servicePulse;
    Pulse coinPulses[2];
  } io;

  Node::Object parent;
  Node::Object node;

  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;
  auto frame() -> void;
  auto serialize(serializer&) -> void;
};

extern VsUniSystem vsUniSystem;
