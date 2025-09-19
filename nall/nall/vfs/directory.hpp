#include <span>

namespace nall::vfs {

struct directory : node {
  auto count() const -> u32 {
    return _nodes.size();
  }

  auto find(shared_pointer<node> item) const -> bool {
    return std::ranges::find(_nodes, item) != _nodes.end();
  }

  auto find(const string& name) const -> bool {
    for(auto& node : _nodes) {
      if(node->name() == name) return true;
    }
    return false;
  }

  template<typename T = file>
  auto read(const string& name) -> shared_pointer<T> {
    for(auto& node : _nodes) {
      if(node->name() == name) {
        if(auto fp = node.cast<T>()) {
          if(!fp->readable()) return {};
          fp->seek(0);
          return fp;
        }
      }
    }
    return {};
  }

  template<typename T = file>
  auto write(const string& name) -> shared_pointer<T> {
    for(auto& node : _nodes) {
      if(node->name() == name) {
        if(auto fp = node.cast<T>()) {
          if(!fp->writable()) return {};
          fp->seek(0);
          return fp;
        }
      }
    }
    return {};
  }

  auto append(const string& name, u64 size) -> bool {
    if(find(name)) return false;
    auto item = memory::create(size);
    item->setName(name);
    _nodes.push_back(item);
    return true;
  }

  auto append(const string& name, shared_pointer<node> item) -> bool {
    if(!item) return false;
    if(find(item)) return false;
    item->setName(name);
    _nodes.push_back(item);
    return true;
  }

  auto append(const string& name, std::span<const u8> view) -> bool {
    if(find(name)) return false;
    auto item = memory::open(view);
    item->setName(name);
    _nodes.push_back(item);
    return true;
  }

  auto append(shared_pointer<node> item) -> bool {
    if(find(item)) return false;
    _nodes.push_back(item);
    return true;
  }

  auto remove(shared_pointer<node> item) -> bool {
    auto erased = std::erase(_nodes, item);
    return erased > 0;
  }

  auto files() const -> std::vector<shared_pointer<file>> {
    std::vector<shared_pointer<file>> files;
    for(auto& node : _nodes) {
      if(!node->isFile()) continue;
      files.push_back(node);
    }
    return files;
  }

  auto directories() const -> std::vector<shared_pointer<directory>> {
    std::vector<shared_pointer<directory>> directories;
    for(auto& node : _nodes) {
      if(!node->isDirectory()) continue;
      directories.push_back(node);
    }
    return directories;
  }

  auto begin() { return _nodes.begin(); }
  auto end() { return _nodes.end(); }

  auto begin() const { return _nodes.begin(); }
  auto end() const { return _nodes.end(); }

protected:
  std::vector<shared_pointer<node>> _nodes;
};

inline auto node::isFile() const -> bool {
  return dynamic_cast<const file*>(this);
}

inline auto node::isDirectory() const -> bool {
  return dynamic_cast<const directory*>(this);
}

}
