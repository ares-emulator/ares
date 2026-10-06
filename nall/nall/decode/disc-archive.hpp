#pragma once

#include <nall/decode/archive.hpp>
#include <nall/decode/cue.hpp>
#include <nall/decode/zip-archive.hpp>
#include <memory>
#include <span>
#include <vector>

namespace nall::Decode {

// Resolves an optical-disc descriptor stored inside an archive container.
// Initially ZIP is the only backend; additional archive formats plug into the
// same factory without changing CUE parsing, MIA, or vfs::cdrom.
struct DiscArchive {
  auto open(const string& filename) -> bool {
    close();
    location = filename;

    if(filename.iendsWith(".zip")) archive = std::make_unique<ZIPArchive>();
    else return false;

    if(!archive->open(filename)) {
      close();
      return false;
    }

    maybe<Archive::File> candidate;
    for(auto& entry : archive->files()) {
      if(!entry.name.iendsWith(".cue")) continue;
      if(candidate) {
        // Multiple descriptors are ambiguous until a selection policy exists.
        close();
        return false;
      }
      candidate = entry;
    }

    if(!candidate) {
      close();
      return false;
    }

    descriptor = candidate.get();
    return true;
  }

  auto close() -> void {
    if(archive) archive->close();
    archive.reset();
    descriptor = {};
    location = {};
  }

  auto readDataSector(u32 sectorID) -> std::vector<u8> {
    if(!archive || !descriptor.name) return {};

    Decode::CUE cuesheet;
    if(!cuesheet.load(location, archive.get(), &descriptor)) return {};

    for(auto& cueFile : cuesheet.files) {
      if(cueFile.type != "binary") continue;

      auto filePathInArchive = cueFile.archiveFolder;
      filePathInArchive.append(cueFile.name);
      auto fileEntry = archive->findFile(filePathInArchive);
      if(!fileEntry) continue;

      std::vector<u8> rawDataBuffer;
      std::span<const u8> rawDataView;
      if(archive->isDataUncompressed(*fileEntry)) {
        rawDataView = archive->dataViewIfUncompressed(*fileEntry);
      } else {
        rawDataBuffer = archive->extract(*fileEntry);
        rawDataView = {rawDataBuffer.data(), rawDataBuffer.size()};
      }
      if(rawDataView.empty()) continue;

      u64 offset = 0;
      for(auto& track : cueFile.tracks) {
        for(auto& index : track.indices) {
          u32 sectorSize = 0;
          if(track.type == "mode1/2048") sectorSize = 2048;
          if(track.type == "mode1/2352") sectorSize = 2352;
          if(track.type == "mode2/2352") sectorSize = 2352;

          if(sectorSize && index.number == 1) {
            auto readPos = offset + (u64)sectorSize * sectorID + (sectorSize == 2352 ? 16 : 0);
            if(readPos + 2048 > rawDataView.size()) return {};

            std::vector<u8> sector(2048);
            memory::copy(sector.data(), sector.size(), rawDataView.data() + readPos, sector.size());
            return sector;
          }

          offset += (u64)track.sectorSize() * index.sectorCount();
        }
      }
    }

    return {};
  }

  string location;
  std::unique_ptr<Archive> archive;
  Archive::File descriptor;
};

}
