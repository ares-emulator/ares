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
    u8 flags = 0;
  };

  struct File {
    auto sectorCount() const -> u32;
    auto scan(const string& pathname, const string& archiveFolder, const Decode::ZIP* archive) -> bool;
    auto skipBytes() const -> u64 { return tracks.empty() ? 0 : (u64)skipSectors * tracks.front().sectorSize(); }

    string name;
    string archiveFolder;
    string type;
    u64 dataOffset = 0;
    u32 skipSectors = 0;
    std::vector<Track> tracks;
  };

  auto load(const string& location, const Decode::ZIP* archive, const Decode::ZIP::File* compressedFile) -> bool;
  auto sectorCount() const -> u32;

  std::vector<File> files;

private:
  auto loadFile(std::vector<string>& lines, u32& offset) -> File;
  auto loadTrack(std::vector<string>& lines, u32& offset) -> Track;
  auto loadIndex(std::vector<string>& lines, u32& offset) -> Index;
  auto toLBA(const string& msf) -> s32;

  bool failed = false;
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

  for(auto& line : lines) line.replace("\t", " ");

  failed = false;
  files.clear();

  u32 offset = 0;
  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) {
      auto file = loadFile(lines, offset);
      if(failed) return false;
      if(file.tracks.empty()) continue;
      files.push_back(file);
      continue;
    }
    offset++;
  }

  if(failed) return false;
  if(files.empty()) return false;
  if(files.front().tracks.empty()) return false;
  if(files.front().tracks.front().indices.empty()) return false;

  u32 previousTrack = 0;
  for(auto& file : files) {
    for(auto& track : file.tracks) {
      if(previousTrack && track.number != previousTrack + 1) return false;
      previousTrack = track.number;
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
  u64 number = lines[offset].natural();
  offset++;

  auto fail = [&]() -> Track { failed = true; return {}; };
  if(number == 0 || number > 99) return fail();
  track.number = number;
  if(!track.sectorSize()) return fail();

  bool hasPostgap = false;
  s32 lastReal = -1;
  while(offset < lines.size()) {
    lines[offset].strip();
    if(lines[offset].ibeginsWith("FILE ")) break;
    if(lines[offset].ibeginsWith("TRACK ")) break;
    if(lines[offset].ibeginsWith("INDEX ")) {
      if(hasPostgap) return fail();
      auto index = loadIndex(lines, offset);
      if(failed) return {};
      lastReal = index.number;
      track.indices.push_back(index);
      continue;
    }
    if(lines[offset].ibeginsWith("PREGAP ")) {
      if(track.pregap || lastReal >= 0) return fail();
      auto lba = toLBA(lines[offset++].itrimLeft("PREGAP ", 1L));
      if(lba < 0) return fail();
      track.pregap = lba;
      Index index; index.number = 0; index.lba = -1;
      track.indices.push_back(index); // placeholder
      continue;
    }
    if(lines[offset].ibeginsWith("POSTGAP ")) {
      if(track.postgap || lastReal < 0) return fail();
      auto lba = toLBA(lines[offset++].itrimLeft("POSTGAP ", 1L));
      if(lba < 0) return fail();
      track.postgap = lba;
      hasPostgap = true;
      Index index; index.number = lastReal + 1; index.lba = -1;
      track.indices.push_back(index); // placeholder
      continue;
    }
    if(lines[offset].ibeginsWith("FLAGS ")) {
      auto flags = nall::split_and_strip(lines[offset++], " ");
      for(u32 n = 1; n < flags.size(); n++) {
        string flag = flags[n];
        flag.downcase();
        if(flag == "pre") track.flags |= 0b0001;
        if(flag == "dcp") track.flags |= 0b0010;
        if(flag == "4ch") track.flags |= 0b1000;
      }
      continue;
    }
    offset++;
  }

  std::stable_sort(track.indices.begin(), track.indices.end(),[](const Index& a, const Index& b) {
    return a.number < b.number;
  });

  bool hasIndex1 = false;
  s32 previous = -1;
  for(auto& index : track.indices) {
    if(index.lba < 0) continue;
    if(index.number == 0 && track.pregap) return fail();
    if(previous < 0 ? index.number > 1 : index.number != previous + 1) return fail();
    previous = index.number;
    if(index.number == 1) hasIndex1 = true;
  }
  if(!hasIndex1) return fail();

  return track;
}

inline auto CUE::loadIndex(std::vector<string>& lines, u32& offset) -> Index {
  Index index;

  lines[offset].itrimLeft("INDEX ", 1L);
  auto parts = nall::split_and_strip(lines[offset], " ");
  string sector = parts.empty() ? string{} : parts.back();
  lines[offset].itrimRight(sector, 1L).strip();
  u64 number = lines[offset].natural();
  index.lba = toLBA(sector);
  offset++;

  if(number > 99 || index.lba < 0) { failed = true; return {}; }
  index.number = number;
  return index;
}

inline auto CUE::toLBA(const string& msf) -> s32 {
  auto parts = nall::split(msf, ":");
  if(parts.size() != 3) return -1;
  u32 value[3];
  for(u32 n : range(3)) {
    if(parts[n].size() == 0) return -1;
    for(u32 c : range(parts[n].size())) {
      if(parts[n].data()[c] < '0' || parts[n].data()[c] > '9') return -1;
    }
    if(parts[n].size() > 3) return -1;
    value[n] = parts[n].natural();
  }
  if(value[0] >= 100 || value[1] >= 60 || value[2] >= 75) return -1;
  return value[0] * 60 * 75 + value[1] * 75 + value[2];
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
    size = zipFileEntry ? zipFileEntry->size : file::size(location);
  } else if(type == "wave") {
    //##TODO## Do we bother to support wav files in our zip bundles?
    Decode::WAV wav;
    if(!wav.open(location)) return false;
    if(wav.channels != 2) return false;
    if(wav.frequency != 44100) return false;
    if(wav.bitrate != 16) return false;
    size = wav.size();
    dataOffset = wav.headerSize;
  } else {
    return false;
  }

  auto firstInFileLBA = [&](const auto& track) -> s32 {
    s32 first = -1;
    for(auto& index : track.indices) {
      if(index.lba < 0) continue;
      if(first < 0 || index.lba < first) first = index.lba;
    }
    return first;
  };

  auto lastSectorInFile = [&](const auto& track) -> s32 {
    auto bytesPerSector = track.sectorSize();
    if(!bytesPerSector) return -1;
    return (s32)(size / bytesPerSector) - 1;
  };

  auto nextTrackStartLBA = [&](u32 t) -> s32 {
    auto thisStart = firstInFileLBA(tracks[t]);
    if(thisStart < 0) return -1;

    s32 next = -1;
    for(u32 n : range(tracks.size())) {
      auto start = firstInFileLBA(tracks[n]);
      if(start < 0) continue;
      if(start <= thisStart) continue;
      if(next < 0 || start < next) next = start;
    }
    return next;
  };

  auto nextIndexStartLBA = [&](const auto& track, s32 lba) -> s32 {
    s32 next = -1;
    for(auto& candidate : track.indices) {
      if(candidate.lba < 0) continue;
      if(candidate.lba <= lba) continue;
      if(next < 0 || candidate.lba < next) next = candidate.lba;
    }
    return next;
  };

  for(u32 t : range(tracks.size())) {
    auto& track = tracks[t];

    auto start = firstInFileLBA(track);
    if(start < 0) continue;

    auto nextTrack = nextTrackStartLBA(t);
    auto trackEnd  = nextTrack >= 0 ? nextTrack - 1 : lastSectorInFile(track);
    if(trackEnd < 0) continue;

    for(auto& index : track.indices) {
      if(index.lba < 0) continue;

      auto nextIndex = nextIndexStartLBA(track, index.lba);
      index.end = nextIndex >= 0 ? nextIndex - 1 : trackEnd;

      if(index.end < index.lba) index.end = index.lba - 1;
    }
  }

  skipSectors = 0;
  if(!tracks.empty()) {
    auto first = firstInFileLBA(tracks.front());
    if(first > 0) {
      if((u64)first * tracks.front().sectorSize() > size) return false;
      skipSectors = first;
      for(auto& t : tracks) {
        for(auto& index : t.indices) {
          if(index.lba < 0) continue;
          index.lba -= first;
          if(index.end >= 0) index.end -= first;
          if(index.end < index.lba) index.end = index.lba - 1;
        }
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
