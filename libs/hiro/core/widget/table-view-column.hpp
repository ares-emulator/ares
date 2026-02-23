#if defined(Hiro_TableView)
struct mTableViewColumn : mObject {
  Declare(TableViewColumn)

  auto active() const -> bool;
  auto alignment() const -> Alignment;
  auto backgroundColor() const -> Color;
  auto editable() const -> bool;
  auto expandable() const -> bool;
  auto foregroundColor() const -> Color;
  auto horizontalAlignment() const -> f32;
  auto icon() const -> multiFactorImage;
  auto remove() -> type& override;
  auto resizable() const -> bool;
  auto setActive() -> type&;
  auto setAlignment(Alignment alignment = {}) -> type&;
  auto setBackgroundColor(Color color = {}) -> type&;
  auto setEditable(bool editable = true) -> type&;
  auto setExpandable(bool expandable = true) -> type&;
  auto setForegroundColor(Color color = {}) -> type&;
  auto setHorizontalAlignment(f32 alignment = 0.0) -> type&;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setResizable(bool resizable = true) -> type&;
  auto setSorting(Sort sorting = Sort::None) -> type&;
  auto setText(const string& text = "") -> type&;
  auto setVerticalAlignment(f32 alignment = 0.5) -> type&;
  auto setVisible(bool visible = true) -> type& override;
  auto setWidth(f32 width = 0) -> type&;
  auto sort(Sort sorting) -> type&;
  auto sorting() const -> Sort;
  auto text() const -> string;
  auto verticalAlignment() const -> f32;
  auto width() const -> f32;

//private:
  struct State {
    bool active = false;
    Alignment alignment;
    Color backgroundColor;
    bool editable = false;
    bool expandable = false;
    Color foregroundColor;
    f32 horizontalAlignment = 0.0;
    multiFactorImage icon;
    bool resizable = true;
    Sort sorting = Sort::None;
    string text;
    f32 verticalAlignment = 0.5;
    bool visible = true;
    f32 width = 0;
  } state;
};
#endif
