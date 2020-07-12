#pragma once

#include <nall/file.hpp>
#include <nall/maybe.hpp>
#include <nall/string.hpp>
#include <nall/decode/wav.hpp>

namespace nall::Decode {

struct CUE {
  struct Index {
    auto sectorCount() const -> uint;

    int number;  //01-99
    int lba = -1;
    int end = -1;
  };

  struct Track {
    auto sectorCount() const -> uint;
    auto sectorSize() const -> uint;

    int number;  //01-99
    string type;
    vector<Index> indices;
    maybe<int> pregap;
    maybe<int> postgap;
  };

  struct File {
    auto sectorCount() const -> uint;
    auto scan(const string& pathname) -> bool;

    string name;
    string type;
    vector<Track> tracks;
  };

  auto load(const string& location) -> bool;
  auto sectorCount() const -> uint;

  vector<File> files;

private:
  auto loadFile(vector<string>& lines, uint& offset) -> File;
  auto loadTrack(vector<string>& lines, uint& offset) -> Track;
  auto loadIndex(vector<string>& lines, uint& offset) -> Index;
  auto toLBA(const string& msf) -> uint;
};

inline auto CUE::load(const string& location) -> bool {
  auto lines = string::read(location).replace("\r", "").split("\n");

  uint offset = 0;
  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) {
      auto file = loadFile(lines, offset);
      if(!file.tracks) continue;
      files.append(file);
      continue;
    }
    offset++;
  }

  if(!files) return false;
  if(!files.first().tracks) return false;
  if(!files.first().tracks.first().indices) return false;

  for(auto& file : files) {
    maybe<Index&> previous;
    for(auto& track : file.tracks) {
      for(auto& index : track.indices) {
        if(previous) previous->end = index.lba - 1;
        previous = index;
      }
    }
  }

  for(auto& file : files) {
    if(!file.scan(Location::path(location))) return false;
  }

  return true;
}

inline auto CUE::loadFile(vector<string>& lines, uint& offset) -> File {
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
      auto track = loadTrack(lines, offset);
      if(!track.indices) continue;
      file.tracks.append(track);
      continue;
    }
    offset++;
  }

  return file;
}

inline auto CUE::loadTrack(vector<string>& lines, uint& offset) -> Track {
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
      auto index = loadIndex(lines, offset);
      track.indices.append(index);
      continue;
    }
    if(lines[offset].ibeginsWith("PREGAP ")) {
      track.pregap = toLBA(lines[offset++].itrimLeft("PREGAP ", 1L));
      continue;
    }
    if(lines[offset].ibeginsWith("POSTGAP ")) {
      track.postgap = toLBA(lines[offset++].itrimLeft("POSTGAP ", 1L));
      continue;
    }
    offset++;
  }

  if(track.number > 99) return {};
  return track;
}

inline auto CUE::loadIndex(vector<string>& lines, uint& offset) -> Index {
  Index index;

  lines[offset].itrimLeft("INDEX ", 1L);
  string sector = lines[offset].split(" ").last().strip();
  lines[offset].itrimRight(sector, 1L).strip();
  index.number = lines[offset].natural();
  index.lba = toLBA(sector);
  offset++;

  if(index.number > 99) return {};
  return index;
}

inline auto CUE::toLBA(const string& msf) -> uint {
  uint m = msf.split(":")(0).natural();
  uint s = msf.split(":")(1).natural();
  uint f = msf.split(":")(2).natural();
  return m * 60 * 75 + s * 75 + f;
}

inline auto CUE::sectorCount() const -> uint {
  uint count = 0;
  for(auto& file : files) count += file.sectorCount();
  return count;
}

inline auto CUE::File::scan(const string& pathname) -> bool {
  string location = {Location::path(pathname), name};
  if(!file::exists(location)) return false;

  uint64_t size = 0;

  if(type == "binary") {
    size = file::size(location);
  } else if(type == "wave") {
    Decode::WAV wav;
    if(!wav.open(location)) return false;
    if(wav.channels != 2) return false;
    if(wav.frequency != 44100) return false;
    if(wav.bitrate != 16) return false;
    size = wav.size();
  } else {
    return false;
  }

  for(auto& track : tracks) {
    for(auto& index : track.indices) {
      if(index.end >= 0) {
        size -= track.sectorSize() * index.sectorCount();
      } else {
        index.end = index.lba + size / track.sectorSize() - 1;
      }
    }
  }

  return true;
}

inline auto CUE::File::sectorCount() const -> uint {
  uint count = 0;
  for(auto& track : tracks) count += track.sectorCount();
  return count;
}

inline auto CUE::Track::sectorCount() const -> uint {
  uint count = 0;
  for(auto& index : indices) count += index.sectorCount();
  return count;
}

inline auto CUE::Track::sectorSize() const -> uint {
  if(type == "mode1/2048") return 2048;
  if(type == "mode1/2352") return 2352;
  if(type == "mode2/2352") return 2352;
  if(type == "audio"     ) return 2352;
  return 0;
}

inline auto CUE::Index::sectorCount() const -> uint {
  if(end < 0) return 0;
  return end - lba + 1;
}

}
