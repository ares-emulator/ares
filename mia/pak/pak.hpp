struct Pak {
  static auto create(string name) -> shared_pointer<Pak>;

  virtual ~Pak() = default;
  virtual auto type() -> string { return pak->attribute("type"); }
  virtual auto name() -> string { return pak->attribute("name"); }
  virtual auto extensions() -> vector<string> { return {}; }
  virtual auto load(string location = {}) -> bool { return true; }
  virtual auto save(string location = {}) -> bool { return true; }

  auto name(string location) const -> string;
  auto read(string location, vector<string> match = {"*"}) -> vector<u8>;
  auto append(vector<u8>& data, string location) -> bool;
  auto load(string name, string extension, string location = {}) -> bool;
  auto save(string name, string extension, string location = {}) -> bool;
  auto load(Markup::Node node, string extension, string location = {}) -> bool;
  auto save(Markup::Node node, string extension, string location = {}) -> bool;
  auto saveLocation(string location, string name, string extension) -> string;

  string location;
  string manifest;
  shared_pointer<vfs::directory> pak;
};
