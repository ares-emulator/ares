#if defined(Hiro_Menu)
struct mMenu : mAction {
  Declare(Menu)
  using mObject::remove;

  auto action(u32 position) const -> Action;
  auto actionCount() const -> u32;
  auto actions() const -> vector<Action>;
  auto append(sAction action) -> type&;
  auto icon() const -> multiFactorImage;
  auto remove(sAction action) -> type&;
  auto reset() -> type& override;
  auto setIcon(const multiFactorImage& icon = {}, bool force = false) -> type&;
  auto setIconForFile(const string& filename) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    vector<sAction> actions;
    multiFactorImage icon;
    string text;
  } state;

  auto destruct() -> void override;
};
#endif
