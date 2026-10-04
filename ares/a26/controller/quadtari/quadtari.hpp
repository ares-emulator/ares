struct QuadTari : Controller {
  struct Slot {
    Slot(QuadTari& owner) : owner(owner) {}

    auto load(Node::Peripheral, string name) -> void;
    auto unload() -> void;
    auto allocate(string name) -> Node::Peripheral;
    auto save() -> void;
    auto power(bool reset) -> void;
    auto serialize(serializer&) -> void;

    QuadTari& owner;
    Node::Port port;
    std::unique_ptr<Controller> device;
  };

  QuadTari(Node::Port);
  ~QuadTari() override;

  auto save() -> void override;
  auto power(bool reset) -> void override;
  auto poll() -> void override;
  auto frame() -> void override;
  auto clock() -> void override;
  auto vblank(n1 dumped) -> void override;
  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto controlWrite(n8 data) -> void override;
  auto readAnalog(n1 index) -> AnalogConnection override;
  auto serialize(serializer&) -> void override;

  static constexpr u16 SettlingClocks = 228;

private:
  auto active() -> Controller*;
  auto synchronizeActive() -> void;

public:
  Slot first;
  Slot second;
  n1 dumped = 0;
  n1 selected = 0;
  n1 pending = 0;
  u16 settling = 0;
  n4 output = 0x0f;
  n4 control = 0x0f;
};
