struct Media {
  virtual ~Media() {}
  virtual auto construct() -> void;
  virtual auto read(string filename, string suffix = "") -> vector<uint8_t>;
  virtual auto type() -> string = 0;
  virtual auto name() -> string = 0;
  virtual auto extensions() -> vector<string> = 0;
  virtual auto manifest(string location) -> string = 0;
  virtual auto import(string filename) -> string = 0;

  auto location(string location, string suffix) const -> string;
  auto name(string location) const -> string;

  Markup::Node database;
  string pathname;
};
