#pragma once

#include <nall/decode/archive.hpp>
#include <memory>

namespace nall::Decode {

// Read-only 7z container backend implemented with the vendored 7-Zip/LZMA SDK.
// The SDK state is hidden behind Impl so its C API does not leak through nall's
// public headers.
struct SevenZipArchive : Archive {
  SevenZipArchive();
  ~SevenZipArchive() override;

  SevenZipArchive(const SevenZipArchive&) = delete;
  auto operator=(const SevenZipArchive&) -> SevenZipArchive& = delete;

  auto findFile(const string& filename) const -> const maybe<File> override;
  auto files() const -> std::vector<File> override;
  auto open(const string& filename) -> bool override;
  auto extract(const File& file) const -> std::vector<u8> override;
  auto isDataUncompressed(const File& file) const -> bool override;
  auto dataViewIfUncompressed(const File& file) const -> std::span<const u8> override;
  auto error() const -> string override;
  auto close() -> void override;

private:
  auto decodedView(const File& file) const -> std::span<const u8>;

  struct Impl;
  std::unique_ptr<Impl> impl;
};

}
