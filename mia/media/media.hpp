struct Media {
  virtual ~Media() = default;

  //delayed constructor so that virtual functions can be called
  virtual auto construct() -> void;

  //read a game pak or game ROM into a vector
  virtual auto read(string location) -> vector<u8>;

  //the type of media for this system ("Cartridge", "Compact Disc", etc)
  virtual auto type() -> string = 0;

  //the name of the media type ("BS Memory", "Super Famicom", etc)
  virtual auto name() -> string = 0;

  //the list of extensions used for this type of media
  virtual auto extensions() -> vector<string> = 0;

  //load a game pak or game ROM
  virtual auto load(string location) -> shared_pointer<vfs::directory> { return {}; }

  //save a game pak or game ROM
  virtual auto save(string location, shared_pointer<vfs::directory> pak) -> bool { return false; }

  //generate a manifest for a game pak or game ROM
  virtual auto manifest(string location) -> string;

  //get save file location on disk
  auto saveLocation(string location, string name, string extension) -> string;

  //load a file into a game pak if it exists; otherwise simply create empty save file
  auto load(string location, shared_pointer<vfs::directory> pak, string name, string extension, u64 size) -> bool;
  auto load(string location, shared_pointer<vfs::directory> pak, Markup::Node node, string extension) -> bool;

  //save a file from a game pak
  auto save(string location, shared_pointer<vfs::directory> pak, string name, string extension) -> bool;
  auto save(string location, shared_pointer<vfs::directory> pak, Markup::Node node, string extension) -> bool;

  //generate a filesystem-safe name for a game pak or game ROM
  auto name(string location) const -> string;

  //append a file onto a vector
  auto append(vector<u8>& data, string location) -> bool;

  Markup::Node database;
  string pathname;
};
