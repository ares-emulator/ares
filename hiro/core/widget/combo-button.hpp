#if defined(Hiro_ComboButton)
struct mComboButton : mWidget {
  Declare(ComboButton)
  using mObject::remove;

  auto append(sComboButtonItem item) -> type&;
  auto doChange() const -> void;
  auto item(u32 position) const -> ComboButtonItem;
  auto itemCount() const -> u32;
  auto items() const -> vector<ComboButtonItem>;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto remove(sComboButtonItem item) -> type&;
  auto reset() -> type& override;
  auto selected() const -> ComboButtonItem;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;

//private:
  struct State {
    vector<sComboButtonItem> items;
    function<void ()> onChange;
  } state;

  auto destruct() -> void override;
};
#endif
