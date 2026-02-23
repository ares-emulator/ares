#pragma once

#include <nall/file-map.hpp>
#include <nall/string.hpp>
#include <vector>
#include <span>
#include <nall/decode/inflate.hpp>

namespace nall::Decode {

struct ZIP {
  struct File {
    string name;
    const u8* data;
    u64 size;
    u64 csize;
    u32 cmode;  //0 = uncompressed, 8 = deflate
    u32 crc32;
    time_t timestamp;
  };

  ~ZIP() {
    close();
  }

  auto findFile(const string& filename) const -> const maybe<File> {
    for (const auto& currentFile : file) {
      if (currentFile.name.iequals(filename)) {
        return currentFile;
      }
    }
    return nothing;
  }

  auto open(const string& filename) -> bool {
    close();
    if(fm.open(filename, file::mode::read) == false) return false;
    if(open(fm.data(), fm.size()) == false) {
      fm.close();
      return false;
    }
    return true;
  }

  auto open(const u8* data, u64 size) -> bool {
    if(size < 22) return false;

    filedata = data;
    filesize = size;

    file.clear();

    bool isZip64 = false;

    const u8* footer = data + size - 22;
    while(true) {
      if(footer <= data + 22) return false;
      if(read(footer, 4) == 0x06054b50) {
        if (read(footer + 16, 4) == 0xFFFFFFFF) {
          isZip64 = true;
          break;
        }
        u32 commentlength = read(footer + 20, 2);
        if(footer + 22 + commentlength == data + size) break;
      }
      footer--;
    }

    if (isZip64) {
      footer = data + size - 22;
      while(true) {
        if(footer <= data + 22) return false;
        if(read(footer, 4) == 0x06064b50) {
          u32 commentlength = read(footer + 20, 2);
          if(read(footer + 16, 4) == 0) break;
        }
        footer--;
      }
    }

    u64 fileCount = (isZip64 ? read(footer + 40, 8) : read(footer + 10, 2));
    const u8* directory = (isZip64 ? (data + read(footer + 48, 8)) : (data + read(footer + 16, 4)));

    this->file.reserve(fileCount);

    while(true) {
      u32 signature = read(directory + 0, 4);
      if(signature != 0x02014b50) break;

      bool needsZip64ExtraFieldRecord = false;
      File file;
      file.cmode = read(directory + 10, 2);
      file.crc32 = read(directory + 16, 4);
      file.csize = read(directory + 20, 4);
      if (isZip64 && (file.csize == 0xFFFFFFFF)) {
        needsZip64ExtraFieldRecord = true;
      }
      file.size  = read(directory + 24, 4);
      if (isZip64 && (file.size == 0xFFFFFFFF)) {
        needsZip64ExtraFieldRecord = true;
      }

      u16 dosTime = read(directory + 12, 2);
      u16 dosDate = read(directory + 14, 2);
      tm info = {};
      info.tm_sec  = (dosTime >>  0 &  31) << 1;
      info.tm_min  = (dosTime >>  5 &  63);
      info.tm_hour = (dosTime >> 11 &  31);
      info.tm_mday = (dosDate >>  0 &  31);
      info.tm_mon  = (dosDate >>  5 &  15) - 1;
      info.tm_year = (dosDate >>  9 & 127) + 80;
      info.tm_isdst = -1;
      file.timestamp = mktime(&info);

      u32 namelength = read(directory + 28, 2);
      u32 extralength = read(directory + 30, 2);
      u32 commentlength = read(directory + 32, 2);
      u16 diskNumberStart = read(directory + 34, 2);

      char* filename = new char[namelength + 1];
      memcpy(filename, directory + 46, namelength);
      filename[namelength] = 0;
      file.name = filename;
      delete[] filename;

      u64 offset = read(directory + 42, 4);
      if (isZip64 && (offset == 0xFFFFFFFF)) {
        needsZip64ExtraFieldRecord = true;
      }

      // Locate the ZIP64 local file extra field record if required, and extract the 64-bit values for this file entry.
      if (needsZip64ExtraFieldRecord) {
        u64 extraFieldOffset = 0;
        bool foundZip64ExtraFieldRecord = false;
        while (extraFieldOffset < extralength) {
          size_t extraFieldBaseAddress = 46 + namelength + extraFieldOffset;
          u16 extraFieldRecordTag = read(directory + extraFieldBaseAddress + 0, 2);
          u16 extraFieldRecordSize = read(directory + extraFieldBaseAddress + 2, 2);
          if (extraFieldRecordTag == 0x0001) {
            size_t expectedExtraRecordSize = (file.size == 0xFFFFFFFF ? 8 : 0) + (file.csize == 0xFFFFFFFF ? 8 : 0) + (offset == 0xFFFFFFFF ? 8 : 0) + (diskNumberStart == 0xFFFF ? 4 : 0);
            if (expectedExtraRecordSize == extraFieldRecordSize) {
              size_t extraFieldOffset = 4;
              if (file.size == 0xFFFFFFFF) {
                file.size = read(directory + extraFieldBaseAddress + extraFieldOffset, 8);
                extraFieldOffset += 8;
              }
              if (file.csize == 0xFFFFFFFF) {
                file.csize = read(directory + extraFieldBaseAddress + extraFieldOffset, 8);
                extraFieldOffset += 8;
              }
              if (offset == 0xFFFFFFFF) {
                offset = read(directory + extraFieldBaseAddress + extraFieldOffset, 8);
                extraFieldOffset += 8;
              }
            }
            foundZip64ExtraFieldRecord = true;
            break;
          }
          extraFieldOffset += 4 + extraFieldRecordSize;
        }
        if (!foundZip64ExtraFieldRecord) {
          return false;
        }
      }

      u64 offsetNL = read(data + offset + 26, 2);
      u64 offsetEL = read(data + offset + 28, 2);
      file.data = data + offset + 30 + offsetNL + offsetEL;

      directory += 46 + namelength + extralength + commentlength;

      this->file.push_back(file);
    }

    return true;
  }

  auto extract(const File& file) const -> std::vector<u8> {
    std::vector<u8> buffer;

    if(file.cmode == 0) {
      buffer.resize(file.size);
      memcpy(buffer.data(), file.data, file.size);
    }

    if(file.cmode == 8) {
      buffer.resize(file.size);
      if(inflate(buffer.data(), buffer.size(), file.data, file.csize) == false) {
        buffer.clear();
      }
    }

    return buffer;
  }

  auto isDataUncompressed(const File& file) const {
    return (file.cmode == 0);
  }

  auto dataViewIfUncompressed(const File& file) const -> std::span<const u8> {
    if(file.cmode == 0) {
      return std::span<const u8>(file.data, file.size);
    }
    return std::span<const u8>();
  }

  auto close() -> void {
    if(fm) fm.close();
  }

protected:
  file_map fm;
  const u8* filedata;
  u64 filesize;

  auto read(const u8* data, u32 size) -> u64 {
    u64 result = 0, shift = 0;
    while(size--) { result |= (u64)(*data++) << shift; shift += 8; }
    return result;
  }

public:
  std::vector<File> file;
};

}
