#if defined(Hiro_TabFrame)
struct mTabFrameItem : mObject {
  Declare(TabFrameItem)

  auto append(sSizable sizable) -> type&;
  auto closable() const -> bool;
  auto icon() const -> multiFactorImage;
  auto movable() const -> bool;
  auto remove() -> type& override;
  auto remove(sSizable sizable) -> type&;
  auto reset() -> type& override;
  auto selected() const -> bool;
  auto setClosable(bool closable = true) -> type&;
  auto setEnabled(bool enabled = true) -> type& override;
  auto setFont(const Font& font = {}) -> type& override;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setMovable(bool movable = true) -> type&;
  auto setParent(mObject* object = nullptr, s32 offset = -1) -> type& override;
  auto setSelected() -> type&;
  auto setText(const string& text = "") -> type&;
  auto setVisible(bool visible = true) -> type& override;
  auto sizable() const -> Sizable;
  auto text() const -> string;

//private:
  struct State {
    bool closable = false;
    multiFactorImage icon;
    bool movable = false;
    bool selected = false;
    sSizable sizable;
    string text;
  } state;

  auto destruct() -> void override;
};
#endif
