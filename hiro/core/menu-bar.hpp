#if defined(Hiro_MenuBar)
struct mMenuBar : mObject {
  Declare(MenuBar)

  auto append(sMenu menu) -> type&;
  auto menu(u32 position) const -> Menu;
  auto menuCount() const -> u32;
  auto menus() const -> vector<Menu>;
  auto remove() -> type& override;
  auto remove(sMenu menu) -> type&;
  auto reset() -> type& override;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;

//private:
  struct State {
    vector<sMenu> menus;
  } state;

  auto destruct() -> void override;
};
#endif
