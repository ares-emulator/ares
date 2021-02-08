#if defined(Hiro_PopupMenu)
struct mPopupMenu : mObject {
  Declare(PopupMenu)
  using mObject::remove;

  auto action(u32 position) const -> Action;
  auto actionCount() const -> u32;
  auto actions() const -> vector<Action>;
  auto append(sAction action) -> type&;
  auto remove(sAction action) -> type&;
  auto reset() -> type& override;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setVisible(bool visible = true) -> type& override;

//private:
  struct State {
    vector<sAction> actions;
  } state;

  auto destruct() -> void override;
};
#endif
