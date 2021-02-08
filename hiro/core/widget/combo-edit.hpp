#if defined(Hiro_ComboEdit)
struct mComboEdit : mWidget {
  Declare(ComboEdit)
  using mObject::remove;

  auto append(sComboEditItem item) -> type&;
  auto backgroundColor() const -> Color;
  auto doActivate() const -> void;
  auto doChange() const -> void;
  auto editable() const -> bool;
  auto foregroundColor() const -> Color;
  auto item(u32 position) const -> ComboEditItem;
  auto itemCount() const -> u32;
  auto items() const -> vector<ComboEditItem>;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto remove(sComboEditItem item) -> type&;
  auto reset() -> type& override;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setEditable(bool editable = true) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    Color backgroundColor;
    bool editable = true;
    Color foregroundColor;
    vector<sComboEditItem> items;
    function<void ()> onActivate;
    function<void ()> onChange;
    string text;
  } state;

  auto destruct() -> void override;
};
#endif
