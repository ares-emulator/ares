#if defined(Hiro_Group)

auto mGroup::allocate() -> pObject* {
  return new pGroup(*this);
}

//

auto mGroup::append(sObject object) -> type& {
  if(auto group = instance.lock()) {
    state.objects.push_back(object);
    object->setGroup(std::static_pointer_cast<mGroup>(group));
  }
  return *this;
}

auto mGroup::object(u32 position) const -> Object {
  if(position < objectCount()) {
    if(auto object = state.objects[position].lock()) {
      return object;
    }
  }
  return {};
}

auto mGroup::objectCount() const -> u32 {
  return state.objects.size();
}

auto mGroup::objects() const -> std::vector<Object> {
  std::vector<Object> objects;
  for(auto& weak : state.objects) {
    if(auto object = weak.lock()) objects.push_back(object);
  }
  return objects;
}

auto mGroup::remove(sObject object) -> type& {
  object->setGroup();
  for(auto offset : range(state.objects.size())) {
    if(auto shared = state.objects[offset].lock()) {
      if(object == shared) {
        state.objects.erase(state.objects.begin() + offset);
        break;
      }
    }
  }
  return *this;
}

#endif
