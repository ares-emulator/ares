#pragma once

#include <nall/decode/archive.hpp>
#include <nall/decode/cue.hpp>
#include <nall/decode/sevenzip-archive.hpp>
#include <nall/decode/zip-archive.hpp>
#include <limits>
#include <memory>
#include <span>
#include <vector>

namespace nall::Decode {

// Decorates one archive containing a single ISO with an in-memory CUE. The
// image stays in the archive namespace; no member path is ever extracted to
// the host filesystem.
struct GeneratedCueArchive : Archive {
  GeneratedCueArchive(std::unique_ptr<Archive> source, const File& image)
  : source(std::move(source)) {
    descriptor.name = {Location::path(image.name), ".ares-generated.cue"};
    for(u32 suffix = 0; this->source->findFile(descriptor.name); suffix++)
      descriptor.name = {Location::path(image.name), ".ares-generated-", suffix, ".cue"};

    cue = {"FILE \"", Location::file(image.name), "\" BINARY\n",
           "  TRACK 01 MODE1/2048\n",
           "    INDEX 01 00:00:00\n"};
    descriptor.size = cue.size();
  }

  auto generatedDescriptor() const -> File { return descriptor; }

  auto findFile(const string& filename) const -> const maybe<File> override {
    auto normalized = normalizeMemberName(filename);
    if(!normalized) {
      errorMessage = {"Unsafe archive member reference: ", filename};
      return nothing;
    }
    if(descriptor.name == *normalized || descriptor.name.iequals(*normalized)) return descriptor;
    auto result = source->findFile(*normalized);
    if(!result) errorMessage = source->error();
    return result;
  }

  auto files() const -> std::vector<File> override {
    auto result = source->files();
    result.push_back(descriptor);
    return result;
  }

  auto open(const string&) -> bool override { return true; }

  auto extract(const File& file) const -> std::vector<u8> override {
    if(file.name == descriptor.name) {
      auto view = dataViewIfUncompressed(file);
      return {view.begin(), view.end()};
    }
    auto result = source->extract(file);
    if(result.empty() && file.size) errorMessage = source->error();
    return result;
  }

  auto isDataUncompressed(const File& file) const -> bool override {
    if(file.name == descriptor.name) return true;
    return source->isDataUncompressed(file);
  }

  auto dataViewIfUncompressed(const File& file) const -> std::span<const u8> override {
    if(file.name == descriptor.name)
      return {(const u8*)cue.data(), cue.size()};
    auto result = source->dataViewIfUncompressed(file);
    if(result.empty() && file.size) errorMessage = source->error();
    return result;
  }

  auto error() const -> string override {
    if(errorMessage) return errorMessage;
    return source->error();
  }

  auto close() -> void override {
    source->close();
  }

private:
  std::unique_ptr<Archive> source;
  File descriptor;
  string cue;
  mutable string errorMessage;
};

// Resolves one optical-disc descriptor stored inside an archive container.
// CUE/BIN sets are preferred. If there is no CUE, one unambiguous 2048-byte
// sector ISO is exposed through GeneratedCueArchive.
struct DiscArchive {
  auto open(const string& filename) -> bool {
    close();
    errorMessage = {};
    location = filename;

    if(filename.iendsWith(".zip")) archive = std::make_unique<ZIPArchive>();
    else if(filename.iendsWith(".7z")) archive = std::make_unique<SevenZipArchive>();
    else return fail("Unsupported archive container.");

    if(!archive->open(filename)) return fail(archive->error());

    maybe<Archive::File> cueCandidate;
    maybe<Archive::File> isoCandidate;
    for(auto& entry : archive->files()) {
      if(entry.name.iendsWith(".cue")) {
        if(cueCandidate)
          return fail("The archive contains multiple CUE files; disc selection is ambiguous.");
        cueCandidate = entry;
      }
      if(entry.name.iendsWith(".iso")) {
        if(isoCandidate)
          return fail("The archive contains multiple ISO files and no deterministic selection is possible.");
        isoCandidate = entry;
      }
    }

    if(cueCandidate) {
      descriptor = *cueCandidate;
      return true;
    }

    if(!isoCandidate)
      return fail("The archive contains no supported disc descriptor (CUE) or single ISO image.");
    if(!isoCandidate->size || isoCandidate->size % 2048)
      return fail("The archive's single ISO member is not a valid sequence of 2048-byte sectors.");

    auto generated = std::make_unique<GeneratedCueArchive>(std::move(archive), *isoCandidate);
    descriptor = generated->generatedDescriptor();
    archive = std::move(generated);
    generatedDescriptor = true;
    return true;
  }

  auto close() -> void {
    if(archive) archive->close();
    archive.reset();
    descriptor = {};
    location = {};
    generatedDescriptor = false;
  }

  auto error() const -> string {
    return errorMessage;
  }

  auto readDataSector(u32 sectorID) -> std::vector<u8> {
    if(!archive || !descriptor.name) {
      errorMessage = "No archive disc descriptor is open.";
      return {};
    }

    Decode::CUE cuesheet;
    if(!cuesheet.load(location, archive.get(), &descriptor)) {
      errorMessage = cuesheet.error();
      if(!errorMessage) errorMessage = archive->error();
      if(!errorMessage) errorMessage = "The archived CUE is invalid or references a missing/unsupported member.";
      return {};
    }

    for(auto& cueFile : cuesheet.files) {
      if(cueFile.type != "binary") continue;

      auto filePathInArchive = Archive::resolveMemberName(cueFile.archiveFolder, cueFile.name);
      if(!filePathInArchive) {
        errorMessage = {"Unsafe CUE member reference: ", cueFile.name};
        return {};
      }
      auto fileEntry = archive->findFile(*filePathInArchive);
      if(!fileEntry) {
        errorMessage = archive->error();
        return {};
      }

      std::vector<u8> rawDataBuffer;
      std::span<const u8> rawDataView;
      if(archive->isDataUncompressed(*fileEntry)) {
        rawDataView = archive->dataViewIfUncompressed(*fileEntry);
      } else {
        rawDataBuffer = archive->extract(*fileEntry);
        rawDataView = {rawDataBuffer.data(), rawDataBuffer.size()};
      }
      if(rawDataView.size() != fileEntry->size) {
        errorMessage = archive->error();
        if(!errorMessage) errorMessage = {"Failed to read archived disc member: ", fileEntry->name};
        return {};
      }

      u64 offset = 0;
      for(auto& track : cueFile.tracks) {
        for(auto& index : track.indices) {
          u32 sectorSize = 0;
          if(track.type == "mode1/2048") sectorSize = 2048;
          if(track.type == "mode1/2352") sectorSize = 2352;
          if(track.type == "mode2/2352") sectorSize = 2352;

          if(sectorSize && index.number == 1) {
            if(sectorID > (std::numeric_limits<u64>::max() - offset) / sectorSize) {
              errorMessage = "Requested archived disc sector offset overflows.";
              return {};
            }
            auto readPos = offset + (u64)sectorSize * sectorID + (sectorSize == 2352 ? 16 : 0);
            if(readPos > rawDataView.size() || rawDataView.size() - readPos < 2048) {
              errorMessage = "Requested archived disc sector is outside the data track.";
              return {};
            }

            std::vector<u8> sector(2048);
            memory::copy(sector.data(), sector.size(), rawDataView.data() + readPos, sector.size());
            errorMessage = {};
            return sector;
          }

          offset += (u64)track.sectorSize() * index.sectorCount();
        }
      }
    }

    errorMessage = "The archived disc has no supported data track.";
    return {};
  }

  string location;
  std::unique_ptr<Archive> archive;
  Archive::File descriptor;
  bool generatedDescriptor = false;

private:
  auto fail(const string& message) -> bool {
    errorMessage = message;
    if(archive) archive->close();
    archive.reset();
    descriptor = {};
    location = {};
    generatedDescriptor = false;
    return false;
  }

  string errorMessage;
};

}
