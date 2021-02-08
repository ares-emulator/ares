#if defined(Hiro_IconView)
struct mIconView : mWidget {
  Declare(IconView)
  using mObject::remove;

  auto append(sIconViewItem item) -> type&;
  auto backgroundColor() const -> Color;
  auto batchable() const -> bool;
  auto batched() const -> vector<IconViewItem>;
  auto doActivate() const -> void;
  auto doChange() const -> void;
  auto doContext() const -> void;
  auto flow() const -> Orientation;
  auto foregroundColor() const -> Color;
  auto item(u32 position) const -> IconViewItem;
  auto itemCount() const -> u32;
  auto items() const -> vector<IconViewItem>;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto onContext(const function<void ()>& callback = {}) -> type&;
  auto orientation() const -> Orientation;
  auto remove(sIconViewItem item) -> type&;
  auto reset() -> type& override;
  auto selected() const -> IconViewItem;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setBatchable(bool batchable = true) -> type&;
  auto setFlow(Orientation flow = Orientation::Vertical) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setOrientation(Orientation orientation = Orientation::Horizontal) -> type&;
  auto setParent(mObject* object = nullptr, s32 offset = -1) -> type& override;
  auto setSelected(const vector<s32>& selections) -> type&;

//private:
  struct State {
    Color backgroundColor;
    bool batchable = false;
    Color foregroundColor;
    Orientation flow = Orientation::Vertical;
    vector<sIconViewItem> items;
    function<void ()> onActivate;
    function<void ()> onChange;
    function<void ()> onContext;
    Orientation orientation = Orientation::Horizontal;
  } state;

  auto destruct() -> void override;
};
#endif
