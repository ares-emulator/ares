namespace nall::vfs {

struct directory : node {
  auto count() const -> u32 {
    return _nodes.size();
  }

  auto find(shared_pointer<node> item) const -> bool {
    return (bool)_nodes.find(item);
  }

  template<typename T = node>
  auto find(const string& name) -> shared_pointer<T> {
    for(auto& node : _nodes) {
      if(node->name() == name) {
        if(auto cast = node.cast<T>()) return cast;
      }
    }
    return {};
  }

  auto append(const string& name, shared_pointer<node> item) -> bool {
    if(_nodes.find(item)) return false;
    item->setName(name);
    return _nodes.append(item), true;
  }

  auto append(const string& name, array_view<u8> view) -> bool {
    if(find(name)) return false;
    auto item = memory::open(view);
    item->setName(name);
    return _nodes.append(item), true;
  }

  auto append(shared_pointer<node> item) -> bool {
    if(_nodes.find(item)) return false;
    return _nodes.append(item), true;
  }

  auto remove(shared_pointer<node> item) -> bool {
    if(!_nodes.find(item)) return false;
    return _nodes.removeByValue(item), true;
  }

  auto files() const -> vector<shared_pointer<file>> {
    vector<shared_pointer<file>> files;
    for(auto& node : _nodes) {
      if(!node->isFile()) continue;
      files.append(node);
    }
    return files;
  }

  auto directories() const -> vector<shared_pointer<directory>> {
    vector<shared_pointer<directory>> directories;
    for(auto& node : _nodes) {
      if(!node->isDirectory()) continue;
      directories.append(node);
    }
    return directories;
  }

  auto begin() { return _nodes.begin(); }
  auto end() { return _nodes.end(); }

  auto begin() const { return _nodes.begin(); }
  auto end() const { return _nodes.end(); }

protected:
  vector<shared_pointer<node>> _nodes;
};

inline auto node::isFile() const -> bool {
  return dynamic_cast<const file*>(this);
}

inline auto node::isDirectory() const -> bool {
  return dynamic_cast<const directory*>(this);
}

}
