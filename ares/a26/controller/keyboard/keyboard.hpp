struct Keyboard : Controller {
  Node::Input::Button key[12];

  Keyboard(Node::Port);

  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto readAnalogA() -> AnalogConnection override;
  auto readAnalogB() -> AnalogConnection override;

private:
  enum class ColumnState : u32 { Pullup, Ground, Vcc };

  auto readColumn(u32 column) -> ColumnState;
  auto readAnalog(u32 column) -> AnalogConnection;

  n4 rows = 0x0f;
};
