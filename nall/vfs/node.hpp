namespace nall::vfs {

struct node {
  virtual ~node() = default;

  auto isFile() const -> bool;
  auto isDirectory() const -> bool;

  auto name() const -> string { return _name; }
  auto setName(const string& name) -> void { _name = name; }

protected:
  string _name;
};

}
