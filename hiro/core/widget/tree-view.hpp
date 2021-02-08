#if defined(Hiro_TreeView)
struct mTreeView : mWidget {
  Declare(TreeView)
  using mObject::remove;

  auto activation() const -> Mouse::Click;
  auto append(sTreeViewItem item) -> type&;
  auto backgroundColor() const -> Color;
  auto collapse(bool recursive = true) -> type&;
  auto doActivate() const -> void;
  auto doChange() const -> void;
  auto doContext() const -> void;
  auto doToggle(sTreeViewItem item) const -> void;
  auto expand(bool recursive = true) -> type&;
  auto foregroundColor() const -> Color;
  auto item(const string& path) const -> TreeViewItem;
  auto itemCount() const -> u32;
  auto items() const -> vector<TreeViewItem>;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto onContext(const function<void ()>& callback = {}) -> type&;
  auto onToggle(const function<void (sTreeViewItem)>& callback = {}) -> type&;
  auto remove(sTreeViewItem item) -> type&;
  auto reset() -> type& override;
  auto selectNone() -> type&;
  auto selected() const -> TreeViewItem;
  auto setActivation(Mouse::Click activation = Mouse::Click::Double) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;

//private:
  struct State {
    Mouse::Click activation = Mouse::Click::Double;
    Color backgroundColor;
    Color foregroundColor;
    vector<sTreeViewItem> items;
    function<void ()> onActivate;
    function<void ()> onChange;
    function<void ()> onContext;
    function<void (sTreeViewItem)> onToggle;
    string selectedPath;
  } state;

  auto destruct() -> void override;
};
#endif
