struct FloppyDisk : Media {
  auto type() -> string override { return "Floppy Disk"; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;

  auto append(vector<u8>& data, string filename) -> bool;
  auto manifest(vector<u8>& data, string location) -> string;

  virtual auto export(string location) -> vector<u8> = 0;
  virtual auto heuristics(vector<u8>& data, string location) -> string = 0;
};

#include "famicom-disk.hpp"
#include "nintendo-64dd.hpp"
