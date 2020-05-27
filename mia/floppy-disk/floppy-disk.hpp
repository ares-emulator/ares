struct FloppyDisk : Media {
  auto type() -> string override { return "Floppy Disk"; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;

  auto append(vector<uint8_t>& data, string filename) -> bool;
  auto manifest(vector<uint8_t>& data, string location) -> string;

  virtual auto export(string location) -> vector<uint8_t> = 0;
  virtual auto heuristics(vector<uint8_t>& data, string location) -> string = 0;
};

#include "famicom-disk.hpp"
#include "nintendo-64dd.hpp"
