#if defined(Hiro_TableView)
struct mTableViewCell : mObject {
  Declare(TableViewCell)

  auto alignment(bool recursive = false) const -> Alignment;
  auto backgroundColor(bool recursive = false) const -> Color;
  auto checkable() const -> bool;
  auto checked() const -> bool;
  auto font(bool recursive = false) const -> Font;
  auto foregroundColor(bool recursive = false) const -> Color;
  auto icon() const -> multiFactorImage;
  auto setAlignment(Alignment alignment = {}) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setCheckable(bool checkable = true) -> type&;
  auto setChecked(bool checked = true) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setForegroundColor(SystemColor color) -> type&;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    Alignment alignment;
    Color backgroundColor;
    bool checkable = false;
    bool checked = false;
    Color foregroundColor;
    SystemColor foregroundSystemColor;
    multiFactorImage icon;
    string text;
  } state;
};
#endif
