#pragma once

#include <nall/file.hpp>
#include <nall/maybe.hpp>
#include <nall/string.hpp>
#include <nall/decode/wav.hpp>
#include <nall/decode/zip.hpp>
#include <vector>

namespace nall::Decode {

struct CUE {
  struct Index {
    auto sectorCount() const -> u32;

    u8 number = 0xff; //00-99
    s32 lba = -1;
    s32 end = -1;
  };

  struct Track {
    auto sectorCount() const -> u32;
    auto sectorSize() const -> u32;

    u8 number = 0xff; //01-99
    string type;
    std::vector<Index> indices;
    maybe<s32> pregap;
    maybe<s32> postgap;
  };

  struct File {
    auto sectorCount() const -> u32;
    auto scan(const string& pathname, const string& archiveFolder, const Decode::ZIP* archive) -> bool;

    string name;
    string archiveFolder;
    string type;
    std::vector<Track> tracks;
  };

  auto load(const string& location, const Decode::ZIP* archive, const Decode::ZIP::File* compressedFile) -> bool;
  auto sectorCount() const -> u32;

  std::vector<File> files;

private:
  auto loadFile(std::vector<string>& lines, u32& offset) -> File;
  auto loadTrack(std::vector<string>& lines, u32& offset) -> Track;
  auto loadIndex(std::vector<string>& lines, u32& offset) -> Index;
  auto toLBA(const string& msf) -> u32;
};

inline auto CUE::load(const string& location, const Decode::ZIP* archive, const Decode::ZIP::File* compressedFile) -> bool {
  std::vector<string> lines;
  string archiveFolder;
  if (compressedFile != nullptr) {
    auto fileNameSeparatorPos = compressedFile->name.findPrevious(compressedFile->name.size()-1, "/");
    if (fileNameSeparatorPos.data() != nullptr) {
      archiveFolder = compressedFile->name.slice(0, fileNameSeparatorPos.get() + 1);
    }
    auto rawDataBuffer = archive->extract(*compressedFile);
    // Currently (2025-08-14) there is no way to construct a nall::string from a fixed-length buffer using
    // nall::string_view, as the variadic constructor overrides "string_view(const char* data, u32 size)",
    // meaning we can't create a string_view from a fixed-length input. We use a std::span here as a
    // workaround.
    auto rawDataBufferAsSpan = std::span<const u8>(rawDataBuffer.data(), rawDataBuffer.size());
    auto splitLines = nall::split(string(rawDataBufferAsSpan).replace("\r", ""), "\n");
    lines.clear();
    lines.reserve(splitLines.size());
    for (u32 i = 0; i < splitLines.size(); i++) lines.push_back(splitLines[i]);
  } else {
    auto splitLines = nall::split(string::read(location).replace("\r", ""), "\n");
    lines.clear();
    lines.reserve(splitLines.size());
    for (u32 i = 0; i < splitLines.size(); i++) lines.push_back(splitLines[i]);
  }

  u32 offset = 0;
  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) {
      auto file = loadFile(lines, offset);
      if(file.tracks.empty()) continue;
      files.push_back(file);
      continue;
    }
    offset++;
  }

  if(files.empty()) return false;
  if(files.front().tracks.empty()) return false;
  if(files.front().tracks.front().indices.empty()) return false;

  // calculate index ends for all but the last index
  for(auto& file : files) {
    maybe<Index&> previous;
    for(auto& track : file.tracks) {
      for(auto& index : track.indices) {
        if(index.lba < 0) continue; // ignore gaps (not in file)
        if(previous) previous->end = index.lba - 1;
        previous = index;
      }
    }
  }

  for(auto& file : files) {
    if(!file.scan(Location::path(location), archiveFolder, archive)) return false;
  }

  return true;
}

inline auto CUE::loadFile(std::vector<string>& lines, u32& offset) -> File {
  File file;

  lines[offset].itrimLeft("FILE ", 1L).strip();
  auto parts = nall::split_and_strip(lines[offset], " ");
  file.type = parts.empty() ? string{} : parts.back().downcase();
  lines[offset].itrimRight(file.type, 1L).strip();
  file.name = lines[offset].trim("\"", "\"", 1L);
  offset++;

  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) break;
    if(lines[offset].ibeginsWith("TRACK ")) {
      auto track = loadTrack(lines, offset);
      if(track.indices.empty()) continue;
      file.tracks.push_back(track);
      continue;
    }
    offset++;
  }

  return file;
}

inline auto CUE::loadTrack(std::vector<string>& lines, u32& offset) -> Track {
  Track track;

  lines[offset].itrimLeft("TRACK ", 1L).strip();
  auto parts = nall::split_and_strip(lines[offset], " ");
  track.type = parts.empty() ? string{} : parts.back().downcase();
  lines[offset].itrimRight(track.type, 1L).strip();
  track.number = lines[offset].natural();
  offset++;

  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) break;
    if(lines[offset].ibeginsWith("TRACK ")) break;
    if(lines[offset].ibeginsWith("INDEX ")) {
      auto index = loadIndex(lines, offset);
      if(index.number == 0 && track.number == 1)
        index.lba = 0; // ignore track 1 index 0 (assume 1st pregap always starts at origin)
      track.indices.push_back(index);
      continue;
    }
    if(lines[offset].ibeginsWith("PREGAP ")) {
      track.pregap = toLBA(lines[offset++].itrimLeft("PREGAP ", 1L));
      Index index; index.number = 0; index.lba = -1;
      track.indices.push_back(index); // placeholder
      continue;
    }
    if(lines[offset].ibeginsWith("POSTGAP ")) {
      track.postgap = toLBA(lines[offset++].itrimLeft("POSTGAP ", 1L));
      Index index; index.number = track.indices.back().number + 1; index.lba = -1;
      track.indices.push_back(index); // placeholder
      continue;
    }
    offset++;
  }

  if(track.number == 0 || track.number > 99) return {};
  return track;
}

inline auto CUE::loadIndex(std::vector<string>& lines, u32& offset) -> Index {
  Index index;

  lines[offset].itrimLeft("INDEX ", 1L);
  auto parts = nall::split_and_strip(lines[offset], " ");
  string sector = parts.empty() ? string{} : parts.back();
  lines[offset].itrimRight(sector, 1L).strip();
  index.number = lines[offset].natural();
  index.lba = toLBA(sector);
  offset++;

  if(index.number > 99) return {};
  return index;
}

inline auto CUE::toLBA(const string& msf) -> u32 {
  auto parts = nall::split(msf, ":");
  u32 m = parts.size() > 0 ? parts[0].natural() : 0;
  u32 s = parts.size() > 1 ? parts[1].natural() : 0;
  u32 f = parts.size() > 2 ? parts[2].natural() : 0;
  return m * 60 * 75 + s * 75 + f;
}

inline auto CUE::sectorCount() const -> u32 {
  u32 count = 0;
  for(auto& file : files) count += file.sectorCount();
  return count;
}

inline auto CUE::File::scan(const string& pathname, const string& archiveFolderPath, const Decode::ZIP* archive) -> bool {
  string location = {Location::path(pathname), name};

  maybe<ZIP::File> zipFileEntry;
  if(archive != nullptr) {
    string archiveFilePath = archiveFolderPath;
    archiveFilePath.append(name);
    zipFileEntry = archive->findFile(archiveFilePath);
    archiveFolder = archiveFolderPath;
    if(!zipFileEntry) return false;
  } else {
    if(!file::exists(location)) return false;
  }

  u64 size = 0;

  if(type == "binary") {
    if(zipFileEntry) {
      size = zipFileEntry->size;
    } else {
      size = file::size(location);
    }
  } else if(type == "wave") {
    //##TODO## Do we bother to support wav files in our zip bundles?
    Decode::WAV wav;
    if(!wav.open(location)) return false;
    if(wav.channels != 2) return false;
    if(wav.frequency != 44100) return false;
    if(wav.bitrate != 16) return false;
    size = wav.size();
  } else {
    return false;
  }

  // calculate last index end for the file
  for(auto& track : tracks) {
    for(auto& index : track.indices) {
      if(index.lba < 0) continue; // ignore gaps (not in file)
      if(index.end >= 0) {
        size -= track.sectorSize() * index.sectorCount();
      } else {
        index.end = index.lba + size / track.sectorSize() - 1;
      }
    }
  }

  return true;
}

inline auto CUE::File::sectorCount() const -> u32 {
  u32 count = 0;
  for(auto& track : tracks) count += track.sectorCount();
  return count;
}

inline auto CUE::Track::sectorCount() const -> u32 {
  u32 count = 0;
  for(auto& index : indices) count += index.sectorCount();
  return count;
}

inline auto CUE::Track::sectorSize() const -> u32 {
  if(type == "mode1/2048") return 2048;
  if(type == "mode1/2352") return 2352;
  if(type == "mode2/2352") return 2352;
  if(type == "audio"     ) return 2352;
  return 0;
}

inline auto CUE::Index::sectorCount() const -> u32 {
  if(end < 0) return 0;
  return end - lba + 1;
}

}
