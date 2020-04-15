#pragma once

#include <nall/file.hpp>
#include <nall/maybe.hpp>
#include <nall/string.hpp>

namespace nall::Decode {

struct CUE {
  struct Index {
    uint sector;
    uint number;
  };

  struct Track {
    string type;
    uint number;
    vector<Index> indices;
  };

  struct File {
    string name;
    string type;
    vector<Track> tracks;
  };

  struct Seek {
    string name;
    string type;
    uint sector;
  };

  auto load(const string& cuesheet) -> bool;
  auto seek(uint trackNumber) const -> maybe<Seek>;

  vector<File> files;
  vector<string> lines;

private:
  auto loadFile(uint& offset) -> File;
  auto loadTrack(uint& offset) -> Track;
  auto loadIndex(uint& offset) -> Index;
};

inline auto CUE::load(const string& cuesheet) -> bool {
  lines = string(cuesheet).replace("\r", "").split("\n");

  uint offset = 0;
  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) {
      auto file = loadFile(offset);
      files.append(file);
      continue;
    }
    offset++;
  }

  return true;
}

inline auto CUE::loadFile(uint& offset) -> File {
  File file;

  lines[offset].itrimLeft("FILE ", 1L).strip();
  file.type = lines[offset].split(" ").last().strip().downcase();
  lines[offset].itrimRight(file.type, 1L).strip();
  file.name = lines[offset].trim("\"", "\"", 1L);
  offset++;

  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) break;
    if(lines[offset].ibeginsWith("TRACK ")) {
      auto track = loadTrack(offset);
      file.tracks.append(track);
      continue;
    }
    offset++;
  }

  return file;
}

inline auto CUE::loadTrack(uint& offset) -> Track {
  Track track;

  lines[offset].itrimLeft("TRACK ", 1L).strip();
  track.type = lines[offset].split(" ").last().strip().downcase();
  lines[offset].itrimRight(track.type, 1L).strip();
  track.number = lines[offset].natural();
  offset++;

  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) break;
    if(lines[offset].ibeginsWith("TRACK ")) break;
    if(lines[offset].ibeginsWith("INDEX ")) {
      auto index = loadIndex(offset);
      track.indices.append(index);
      continue;
    }
    offset++;
  }

  return track;
}

inline auto CUE::loadIndex(uint& offset) -> Index {
  Index index;

  lines[offset].itrimLeft("INDEX ", 1L).strip();
  string sector = lines[offset].split(" ").last().strip();
  lines[offset].itrimRight(sector, 1L).strip();
  index.number = lines[offset].natural();
  offset++;

  uint m = sector.split(":")(0).natural();
  uint s = sector.split(":")(1).natural();
  uint f = sector.split(":")(2).natural();
  index.sector = m * 60 * 75 + s * 75 + f;

  return index;
}

inline auto CUE::seek(uint trackNumber) const -> maybe<Seek> {
  for(auto& file : files) {
    for(auto& track : file.tracks) {
      if(track.number == trackNumber && track.indices) {
        Seek seek;
        seek.name = file.name;
        seek.type = track.type;
        seek.sector = track.indices.first().sector;
        return seek;
      }
    }
  }
  return {};
}

}
