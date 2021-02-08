#if defined(Hiro_HexEdit)
struct mHexEdit : mWidget {
  Declare(HexEdit)

  auto address() const -> u32;
  auto backgroundColor() const -> Color;
  auto columns() const -> u32;
  auto doRead(u32 offset) const -> u8;
  auto doWrite(u32 offset, u8 data) const -> void;
  auto foregroundColor() const -> Color;
  auto length() const -> u32;
  auto onRead(const function<u8 (u32)>& callback = {}) -> type&;
  auto onWrite(const function<void (u32, u8)>& callback = {}) -> type&;
  auto rows() const -> u32;
  auto setAddress(u32 address = 0) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setColumns(u32 columns = 16) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setLength(u32 length) -> type&;
  auto setRows(u32 rows = 16) -> type&;
  auto update() -> type&;

//private:
  struct State {
    u32 address = 0;
    Color backgroundColor;
    u32 columns = 16;
    Color foregroundColor;
    u32 length = 0;
    function<u8 (u32)> onRead;
    function<void (u32, u8)> onWrite;
    u32 rows = 16;
  } state;
};
#endif
