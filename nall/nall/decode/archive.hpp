#pragma once

#include <memory>
#include <nall/decode/zip.hpp>
#include <nall/decode/sevenzip.hpp>

namespace nall::Decode {

// Unified archive reader that transparently handles both .zip and .7z files.
// The file list contains only regular files — directory entries are excluded.
struct Archive {
  struct File {
    string   name;
    u64      size      = 0;
    time_t   timestamp = 0;

  private:
    u32 index = 0;   // index into the underlying decoder's file vector
    friend struct Archive;
  };

  ~Archive() { close(); }

  // Returns true if filename uses a supported archive extension.
  static auto supported(const string& filename) -> bool {
    return filename.iendsWith(".zip") || filename.iendsWith(".7z");
  }

  auto open(const string& filename) -> bool {
    close();

    if(filename.iendsWith(".zip")) {
      auto zip = std::make_unique<ZIP>();
      if(!zip->open(filename)) return false;
      for(u32 index = 0; index < (u32)zip->file.size(); index++) {
        const auto& entry = zip->file[index];
        File f;
        f.name      = entry.name;
        f.size      = entry.size;
        f.timestamp = entry.timestamp;
        f.index     = index;
        file.push_back(f);
      }
      _zip = std::move(zip);
      return true;
    }

    if(filename.iendsWith(".7z")) {
      auto sevenzip = std::make_unique<SevenZip>();
      if(!sevenzip->open(filename)) return false;
      for(u32 index = 0; index < (u32)sevenzip->file.size(); index++) {
        const auto& entry = sevenzip->file[index];
        if(entry.directory) continue;
        File f;
        f.name      = entry.name;
        f.size      = entry.size;
        f.timestamp = entry.timestamp;
        f.index     = index;
        file.push_back(f);
      }
      _sevenzip = std::move(sevenzip);
      return true;
    }

    return false;
  }

  auto extract(const File& f) -> std::vector<u8> {
    if(_zip && f.index < (u32)_zip->file.size())
      return _zip->extract(_zip->file[f.index]);
    if(_sevenzip && f.index < (u32)_sevenzip->file.size())
      return _sevenzip->extract(_sevenzip->file[f.index]);
    return {};
  }

  auto close() -> void {
    file.clear();
    _zip.reset();
    _sevenzip.reset();
  }

  std::vector<File> file;

private:
  std::unique_ptr<ZIP>      _zip;
  std::unique_ptr<SevenZip> _sevenzip;
};

}
