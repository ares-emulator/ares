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

  //create a virtual game pak directory from a game pak or game ROM
  virtual auto pak(string location) -> shared_pointer<vfs::directory>;

  //create a vector from a game pak
  virtual auto rom(string location) -> vector<u8> = 0;

  //generate a manifest for a game pak or game ROM
  virtual auto manifest(string location) -> string = 0;

  //generate a filesystem-safe name for a game pak or game ROM
  auto name(string location) const -> string;

  //append a file onto a vector
  auto append(vector<u8>& data, string location) -> bool;

  Markup::Node database;
  string pathname;
};
