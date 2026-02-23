#if defined(Hiro_Widget)
struct mWidget : mSizable {
  Declare(Widget)

  auto doDrop(std::vector<string> names) const -> void;
  auto doMouseEnter() const -> void;
  auto doMouseLeave() const -> void;
  auto doMouseMove(Position position) const -> void;
  auto doMousePress(Mouse::Button button) const -> void;
  auto doMouseRelease(Mouse::Button button) const -> void;
  auto droppable() const -> bool;
  auto focusable() const -> bool;
  auto mouseCursor() const -> MouseCursor;
  auto onDrop(const std::function<void (std::vector<string>)>& callback = {}) -> type&;
  auto onMouseEnter(const std::function<void ()>& callback = {}) -> type&;
  auto onMouseLeave(const std::function<void ()>& callback = {}) -> type&;
  auto onMouseMove(const std::function<void (Position position)>& callback = {}) -> type&;
  auto onMousePress(const std::function<void (Mouse::Button)>& callback = {}) -> type&;
  auto onMouseRelease(const std::function<void (Mouse::Button)>& callback = {}) -> type&;
  auto remove() -> type& override;
  auto setDroppable(bool droppable = true) -> type&;
  auto setFocusable(bool focusable = true) -> type&;
  auto setMouseCursor(const MouseCursor& mouseCursor = {}) -> type&;
  auto setToolTip(const string& toolTip = "") -> type&;
  auto toolTip() const -> string;

//private:
  struct State {
    bool droppable = false;
    bool focusable = false;
    MouseCursor mouseCursor;
    std::function<void (std::vector<string>)> onDrop;
    std::function<void ()> onMouseEnter;
    std::function<void ()> onMouseLeave;
    std::function<void (Position)> onMouseMove;
    std::function<void (Mouse::Button)> onMousePress;
    std::function<void (Mouse::Button)> onMouseRelease;
    string toolTip;
  } state;
};
#endif
