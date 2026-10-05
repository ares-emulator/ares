#pragma once

#include <nall/decode/archive.hpp>
#include <nall/decode/zip.hpp>

namespace nall::Decode {

// Adapter that exposes the existing ZIP decoder through the format-neutral
// Archive interface without changing ZIP parsing or extraction semantics.
struct ZIPArchive : Archive {
  ~ZIPArchive() override {
    close();
  }

  auto findFile(const string& filename) const -> const maybe<File> override {
    auto result = archive.findFile(filename);
    if(!result) return nothing;
    return convert(*result);
  }

  auto files() const -> std::vector<File> override {
    std::vector<File> result;
    result.reserve(archive.file.size());
    for(auto& entry : archive.file) result.push_back(convert(entry));
    return result;
  }

  auto open(const string& filename) -> bool override {
    return archive.open(filename);
  }

  auto extract(const File& file) const -> std::vector<u8> override {
    auto entry = archive.findFile(file.name);
    if(!entry) return {};
    return archive.extract(*entry);
  }

  auto isDataUncompressed(const File& file) const -> bool override {
    auto entry = archive.findFile(file.name);
    if(!entry) return false;
    return archive.isDataUncompressed(*entry);
  }

  auto dataViewIfUncompressed(const File& file) const -> std::span<const u8> override {
    auto entry = archive.findFile(file.name);
    if(!entry) return {};
    return archive.dataViewIfUncompressed(*entry);
  }

  auto close() -> void override {
    archive.close();
  }

private:
  static auto convert(const ZIP::File& source) -> File {
    File target;
    target.name = source.name;
    target.data = source.data;
    target.size = source.size;
    target.csize = source.csize;
    target.cmode = source.cmode;
    target.crc32 = source.crc32;
    target.timestamp = source.timestamp;
    return target;
  }

  ZIP archive;
};

}
