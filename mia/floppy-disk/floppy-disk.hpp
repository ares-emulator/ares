struct FloppyDisk : Media {
  auto type() -> string override { return "Floppy Disk"; }
  auto construct() -> void override;
  auto manifest(vector<u8>& data, string location) -> string;
  virtual auto heuristics(vector<u8>& data, string location) -> string = 0;
};

#include "famicom-disk.hpp"
#include "nintendo-64dd.hpp"
