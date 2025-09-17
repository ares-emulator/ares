#if defined(Hiro_ListView)

struct ListView;
struct ListViewItem;

struct mListView;
struct mListViewItem;

using sListView = std::shared_ptr<mListView>;
using sListViewItem = std::shared_ptr<mListViewItem>;

struct mListView : mTableView {
  using type = mListView;
  using mTableView::append;
  using mTableView::remove;

  mListView();
  auto batched() const -> std::vector<ListViewItem>;
  auto doActivate() const -> void;
  auto doChange() const -> void;
  auto doContext() const -> void;
  auto doToggle(ListViewItem) const -> void;
  auto item(u32 position) const -> ListViewItem;
  auto items() const -> std::vector<ListViewItem>;
  auto onActivate(const std::function<void ()>& callback) -> type&;
  auto onChange(const std::function<void ()>& callback) -> type&;
  auto onContext(const std::function<void ()>& callback) -> type&;
  auto onToggle(const std::function<void (ListViewItem)>& callback) -> type&;
  auto reset() -> type& override;
  auto resizeColumn() -> type&;
  auto selected() const -> ListViewItem;
  auto setVisible(bool visible = true) -> type& override;

//private:
  struct State {
    std::function<void ()> onActivate;
    std::function<void ()> onChange;
    std::function<void ()> onContext;
    std::function<void (ListViewItem)> onToggle;
  } state;
};

struct mListViewItem : mTableViewItem {
  using type = mListViewItem;
  using mTableViewItem::append;
  using mTableViewItem::remove;

  mListViewItem();
  auto checkable() const -> bool;
  auto checked() const -> bool;
  auto icon() const -> multiFactorImage;
  auto reset() -> type&;
  auto setCheckable(bool checkable) -> type&;
  auto setChecked(bool checked) -> type&;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setText(const string& text) -> type&;
  auto text() const -> string;
};

#endif
