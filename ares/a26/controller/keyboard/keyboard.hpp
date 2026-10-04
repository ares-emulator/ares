struct Keyboard : Controller {
  Node::Input::Button key[12];

  Keyboard(Node::Port);

  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto readAnalog(n1 index) -> AnalogConnection override;

private:
  enum class ColumnState : u32 { Pullup, Ground, Vcc };

  auto readColumn(u32 column) -> ColumnState;

  n4 rows = 0x0f;
};
