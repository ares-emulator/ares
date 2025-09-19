#if defined(Hiro_Group)
struct mGroup : mObject {
  Declare(Group)
  using mObject::remove;

  auto append(sObject object) -> type&;
  auto object(u32 offset) const -> Object;
  auto objectCount() const -> u32;
  auto objects() const -> std::vector<Object>;
  auto remove(sObject object) -> type&;

//private:
  struct State {
    std::vector<wObject> objects;
  } state;
};
#endif
