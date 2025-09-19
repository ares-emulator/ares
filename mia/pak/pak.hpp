struct Pak {
  static auto create(string name) -> shared_pointer<Pak>;

  virtual ~Pak() = default;
  virtual auto type() -> string { return pak->attribute("type"); }
  virtual auto name() -> string { return pak->attribute("name"); }
  virtual auto saveName() -> string { return name(); }
  virtual auto extensions() -> std::vector<string> { return {}; }
  virtual auto load(string location = {}) -> LoadResult { return successful; }
  virtual auto loadMultiple(std::vector<string> location = {}) -> bool { return true; }
  virtual auto save(string location = {}) -> bool { return true; }

  auto name(string location) const -> string;
  auto read(string location) -> std::vector<u8>;
  auto read(string location, std::vector<string> match) -> std::vector<u8>;
  auto append(std::vector<u8>& data, string location) -> bool;
  auto load(string name, string extension, string location = {}) -> bool;
  auto save(string name, string extension, string location = {}) -> bool;
  auto load(Markup::Node node, string extension, string location = {}) -> bool;
  auto save(Markup::Node node, string extension, string location = {}) -> bool;
  auto saveLocation(string location, string name, string extension) -> string;

  string location;
  string manifest;
  shared_pointer<vfs::directory> pak;
};
