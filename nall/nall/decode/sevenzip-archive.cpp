#include <nall/decode/sevenzip-archive.hpp>
#include <nall/file-buffer.hpp>

#include <algorithm>
#include <limits>
#include <mutex>
#include <span>
#include <utility>

#include "../../../thirdparty/lzma-sdk-26.02/C/7z.h"
#include "../../../thirdparty/lzma-sdk-26.02/C/7zAlloc.h"
#include "../../../thirdparty/lzma-sdk-26.02/C/7zCrc.h"

namespace nall::Decode {

namespace {

constexpr size_t SevenZipInputBufferSize = 1u << 18;
constexpr size_t SevenZipMaximumNameLength = 1u << 20;

static auto appendUtf8(std::vector<u8>& output, u32 codepoint) -> void {
  if(codepoint <= 0x7f) {
    output.push_back((u8)codepoint);
    return;
  }
  if(codepoint <= 0x7ff) {
    output.push_back((u8)(0xc0 | (codepoint >> 6)));
    output.push_back((u8)(0x80 | (codepoint & 0x3f)));
    return;
  }
  if(codepoint <= 0xffff) {
    output.push_back((u8)(0xe0 | (codepoint >> 12)));
    output.push_back((u8)(0x80 | ((codepoint >> 6) & 0x3f)));
    output.push_back((u8)(0x80 | (codepoint & 0x3f)));
    return;
  }

  output.push_back((u8)(0xf0 | (codepoint >> 18)));
  output.push_back((u8)(0x80 | ((codepoint >> 12) & 0x3f)));
  output.push_back((u8)(0x80 | ((codepoint >> 6) & 0x3f)));
  output.push_back((u8)(0x80 | (codepoint & 0x3f)));
}

static auto utf16ToUtf8(const UInt16* input, size_t length) -> string {
  std::vector<u8> output;
  output.reserve(length * 3);

  for(size_t index = 0; index < length; index++) {
    u32 codepoint = input[index];
    if(!codepoint) break;

    if(codepoint >= 0xd800 && codepoint <= 0xdbff) {
      if(index + 1 < length) {
        u32 low = input[index + 1];
        if(low >= 0xdc00 && low <= 0xdfff) {
          codepoint = 0x10000 + ((codepoint - 0xd800) << 10) + (low - 0xdc00);
          index++;
        } else {
          codepoint = 0xfffd;
        }
      } else {
        codepoint = 0xfffd;
      }
    } else if(codepoint >= 0xdc00 && codepoint <= 0xdfff) {
      codepoint = 0xfffd;
    }

    appendUtf8(output, codepoint);
  }

  if(output.empty()) return {};
  return string(std::span<const u8>(output.data(), output.size()));
}

}  // namespace

struct SevenZipArchive::Impl {
  struct Entry {
    File file;
    UInt32 index = 0;
  };

  struct SeekStream {
    ISeekInStream vt{};
    file_buffer file;

    static auto from(ISeekInStreamPtr stream) -> SeekStream* {
      return reinterpret_cast<SeekStream*>(const_cast<ISeekInStream*>(stream));
    }

    static auto read(ISeekInStreamPtr stream, void* data, size_t* size) -> SRes {
      auto self = from(stream);
      if(!self->file) {
        *size = 0;
        return SZ_ERROR_READ;
      }

      auto fileSize = self->file.size();
      auto fileOffset = self->file.offset();
      if(fileOffset > fileSize) {
        *size = 0;
        return SZ_ERROR_READ;
      }

      auto remaining = fileSize - fileOffset;
      auto length = (size_t)std::min<u64>(remaining, *size);
      if(length) self->file.read(std::span<u8>((u8*)data, length));
      *size = length;
      return SZ_OK;
    }

    static auto seek(ISeekInStreamPtr stream, Int64* position, ESzSeek origin) -> SRes {
      auto self = from(stream);
      if(!self->file) return SZ_ERROR_READ;

      auto fileSize = self->file.size();
      if(fileSize > (u64)std::numeric_limits<Int64>::max()) return SZ_ERROR_FAIL;

      u64 base = 0;
      if(origin == SZ_SEEK_SET) base = 0;
      else if(origin == SZ_SEEK_CUR) base = self->file.offset();
      else if(origin == SZ_SEEK_END) base = fileSize;
      else return SZ_ERROR_FAIL;
      if(base > fileSize) return SZ_ERROR_FAIL;

      u64 target = 0;
      if(*position >= 0) {
        auto delta = (u64)*position;
        if(delta > fileSize - base) return SZ_ERROR_FAIL;
        target = base + delta;
      } else {
        // Convert the negative displacement to its magnitude without negating
        // INT64_MIN, which would itself overflow a signed integer.
        auto delta = (u64)(-(*position + 1)) + 1;
        if(delta > base) return SZ_ERROR_FAIL;
        target = base - delta;
      }

      self->file.seek(target);
      *position = (Int64)target;
      return SZ_OK;
    }

    auto open(const string& filename) -> bool {
      vt.Read = &read;
      vt.Seek = &seek;
      return file.open(filename, file_buffer::mode::read);
    }

    auto close() -> void {
      file.close();
    }
  };

  SeekStream input;
  CLookToRead2 look{};
  CSzArEx database{};
  ISzAlloc allocator{SzAlloc, SzFree};
  ISzAlloc temporaryAllocator{SzAllocTemp, SzFreeTemp};
  std::vector<Entry> entries;

  bool databaseInitialized = false;
  mutable UInt32 blockIndex = 0xffffffff;
  mutable Byte* block = nullptr;
  mutable size_t blockSize = 0;
};

SevenZipArchive::SevenZipArchive() : impl(std::make_unique<Impl>()) {
}

SevenZipArchive::~SevenZipArchive() {
  close();
}

auto SevenZipArchive::open(const string& filename) -> bool {
  close();
  if(!impl->input.open(filename)) return false;

  LookToRead2_CreateVTable(&impl->look, False);
  impl->look.buf = (Byte*)ISzAlloc_Alloc(&impl->allocator, SevenZipInputBufferSize);
  if(!impl->look.buf) {
    close();
    return false;
  }
  impl->look.bufSize = SevenZipInputBufferSize;
  impl->look.realStream = &impl->input.vt;
  LookToRead2_INIT(&impl->look)

  static std::once_flag crcTable;
  std::call_once(crcTable, [] { CrcGenerateTable(); });

  SzArEx_Init(&impl->database);
  impl->databaseInitialized = true;
  auto result = SzArEx_Open(
    &impl->database,
    &impl->look.vt,
    &impl->allocator,
    &impl->temporaryAllocator
  );
  if(result != SZ_OK) {
    close();
    return false;
  }

  impl->entries.reserve(impl->database.NumFiles);
  for(UInt32 index = 0; index < impl->database.NumFiles; index++) {
    if(SzArEx_IsDir(&impl->database, index)) continue;

    auto nameLength = SzArEx_GetFileNameUtf16(&impl->database, index, nullptr);
    if(!nameLength || nameLength > SevenZipMaximumNameLength) {
      close();
      return false;
    }

    std::vector<UInt16> name(nameLength);
    if(SzArEx_GetFileNameUtf16(&impl->database, index, name.data()) != nameLength) {
      close();
      return false;
    }

    Impl::Entry entry;
    entry.index = index;
    entry.file.name = utf16ToUtf8(name.data(), name.size());
    entry.file.size = SzArEx_GetFileSize(&impl->database, index);
    if(!entry.file.name) {
      close();
      return false;
    }
    impl->entries.push_back(std::move(entry));
  }

  return true;
}

auto SevenZipArchive::findFile(const string& filename) const -> const maybe<File> {
  if(!impl) return nothing;

  for(auto& entry : impl->entries) {
    if(entry.file.name == filename) return entry.file;
  }
  for(auto& entry : impl->entries) {
    if(entry.file.name.iequals(filename)) return entry.file;
  }
  return nothing;
}

auto SevenZipArchive::files() const -> std::vector<File> {
  std::vector<File> result;
  if(!impl) return result;

  result.reserve(impl->entries.size());
  for(auto& entry : impl->entries) result.push_back(entry.file);
  return result;
}

auto SevenZipArchive::extract(const File& file) const -> std::vector<u8> {
  if(!impl || !impl->databaseInitialized) return {};

  const Impl::Entry* selected = nullptr;
  for(auto& entry : impl->entries) {
    if(entry.file.name == file.name) {
      selected = &entry;
      break;
    }
  }
  if(!selected) {
    for(auto& entry : impl->entries) {
      if(entry.file.name.iequals(file.name)) {
        selected = &entry;
        break;
      }
    }
  }
  if(!selected) return {};
  if(selected->file.size > std::numeric_limits<size_t>::max()) return {};

  size_t offset = 0;
  size_t extractedSize = 0;
  auto result = SzArEx_Extract(
    &impl->database,
    &impl->look.vt,
    selected->index,
    &impl->blockIndex,
    &impl->block,
    &impl->blockSize,
    &offset,
    &extractedSize,
    &impl->allocator,
    &impl->temporaryAllocator
  );
  if(result != SZ_OK) return {};
  if(offset > impl->blockSize || extractedSize > impl->blockSize - offset) return {};
  if(extractedSize != selected->file.size) return {};

  std::vector<u8> output(extractedSize);
  if(extractedSize) memory::copy(output.data(), output.size(), impl->block + offset, extractedSize);
  return output;
}

auto SevenZipArchive::isDataUncompressed(const File&) const -> bool {
  // A 7z member can share a decoded solid block with other members, so expose
  // all members through extract() even when the coder itself is Copy.
  return false;
}

auto SevenZipArchive::dataViewIfUncompressed(const File&) const -> std::span<const u8> {
  return {};
}

auto SevenZipArchive::close() -> void {
  if(!impl) return;

  if(impl->block) {
    ISzAlloc_Free(&impl->allocator, impl->block);
    impl->block = nullptr;
  }
  impl->blockIndex = 0xffffffff;
  impl->blockSize = 0;

  if(impl->databaseInitialized) {
    SzArEx_Free(&impl->database, &impl->allocator);
    impl->databaseInitialized = false;
  }

  if(impl->look.buf) {
    ISzAlloc_Free(&impl->allocator, impl->look.buf);
    impl->look.buf = nullptr;
  }
  impl->look.bufSize = 0;
  impl->look.realStream = nullptr;
  LookToRead2_INIT(&impl->look)

  impl->input.close();
  impl->entries.clear();
}

}
