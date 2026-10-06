#include <nall/nall.hpp>
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
constexpr u64 SevenZipMaximumArchiveSize = 2ull << 30;
constexpr u64 SevenZipMaximumDecodedBlockSize = 1ull << 30;
constexpr u64 SevenZipMaximumExpandedSize = 2ull << 30;
constexpr UInt32 SevenZipMaximumFileCount = 10000;

static auto sevenZipError(SRes result, const string& context) -> string {
  if(result == SZ_ERROR_UNSUPPORTED)
    return {context, ": encrypted/password-protected archives and unsupported compression methods are not supported."};
  if(result == SZ_ERROR_CRC || result == SZ_ERROR_DATA || result == SZ_ERROR_ARCHIVE || result == SZ_ERROR_INPUT_EOF || result == SZ_ERROR_NO_ARCHIVE)
    return {context, ": the SevenZip archive is corrupt or truncated."};
  if(result == SZ_ERROR_MEM)
    return {context, ": the SevenZip archive exceeds the decoder memory limit."};
  if(result == SZ_ERROR_READ)
    return {context, ": the SevenZip archive could not be read."};
  return {context, ": SevenZip decoder error ", result, "."};
}

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

      if(self->file.offset() > self->file.size()) {
        *size = 0;
        return SZ_ERROR_READ;
      }
      auto remaining = self->file.size() - self->file.offset();
      auto length = (size_t)std::min<u64>(remaining, *size);
      if(length) self->file.read(std::span<u8>((u8*)data, length));
      *size = length;
      return SZ_OK;
    }

    static auto seek(ISeekInStreamPtr stream, Int64* position, ESzSeek origin) -> SRes {
      auto self = from(stream);
      if(!self->file) return SZ_ERROR_READ;

      if(origin != SZ_SEEK_SET && origin != SZ_SEEK_CUR && origin != SZ_SEEK_END) return SZ_ERROR_PARAM;
      if(self->file.size() > (u64)std::numeric_limits<s64>::max()) return SZ_ERROR_UNSUPPORTED;

      s64 base = 0;
      if(origin == SZ_SEEK_CUR) base = (s64)self->file.offset();
      if(origin == SZ_SEEK_END) base = (s64)self->file.size();

      if(*position > 0 && base > std::numeric_limits<s64>::max() - *position) return SZ_ERROR_FAIL;
      if(*position < 0 && base < std::numeric_limits<s64>::min() - *position) return SZ_ERROR_FAIL;
      auto target = base + *position;
      if(target < 0 || (u64)target > self->file.size()) return SZ_ERROR_FAIL;

      self->file.seek(target);
      *position = target;
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
  mutable UInt32 viewFileIndex = 0xffffffff;
  mutable size_t viewOffset = 0;
  mutable size_t viewSize = 0;
  mutable std::mutex decoderMutex;
  mutable string errorMessage;
};

SevenZipArchive::SevenZipArchive() : impl(std::make_unique<Impl>()) {
}

SevenZipArchive::~SevenZipArchive() {
  close();
}

auto SevenZipArchive::open(const string& filename) -> bool {
  close();
  impl->errorMessage = {};
  auto fail = [&](const string& message) -> bool {
    close();
    impl->errorMessage = message;
    return false;
  };

  if(!impl->input.open(filename)) return fail("The SevenZip archive could not be opened.");
  if(impl->input.file.size() > SevenZipMaximumArchiveSize)
    return fail("The SevenZip archive exceeds the 2 GiB compressed-size safety limit.");

  LookToRead2_CreateVTable(&impl->look, False);
  impl->look.buf = (Byte*)ISzAlloc_Alloc(&impl->allocator, SevenZipInputBufferSize);
  if(!impl->look.buf) return fail("The SevenZip decoder could not allocate its input buffer.");
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
  if(result != SZ_OK) return fail(sevenZipError(result, "Unable to read the SevenZip directory"));

  if(impl->database.NumFiles > SevenZipMaximumFileCount)
    return fail("The SevenZip archive contains more than 10000 members.");

  for(UInt32 folder = 0; folder < impl->database.db.NumFolders; folder++) {
    auto unpackSize = SzAr_GetFolderUnpackSize(&impl->database.db, folder);
    if(unpackSize > SevenZipMaximumDecodedBlockSize || unpackSize > std::numeric_limits<size_t>::max())
      return fail("A SevenZip solid block exceeds the 1 GiB decoded-size safety limit.");
  }

  impl->entries.reserve(impl->database.NumFiles);
  u64 expandedSize = 0;
  for(UInt32 index = 0; index < impl->database.NumFiles; index++) {
    if(SzArEx_IsDir(&impl->database, index)) continue;

    auto nameLength = SzArEx_GetFileNameUtf16(&impl->database, index, nullptr);
    if(!nameLength || nameLength > SevenZipMaximumNameLength)
      return fail("A SevenZip member name is empty or exceeds the 1 MiB safety limit.");

    std::vector<UInt16> name(nameLength);
    if(SzArEx_GetFileNameUtf16(&impl->database, index, name.data()) != nameLength)
      return fail("A SevenZip member name could not be decoded.");

    Impl::Entry entry;
    entry.index = index;
    auto decodedName = utf16ToUtf8(name.data(), name.size());
    auto normalizedName = Archive::normalizeMemberName(decodedName);
    if(!normalizedName) return fail({"Unsafe SevenZip member name: ", decodedName});
    entry.file.name = *normalizedName;
    entry.file.size = SzArEx_GetFileSize(&impl->database, index);
    if(entry.file.size > SevenZipMaximumDecodedBlockSize)
      return fail({"SevenZip member exceeds the 1 GiB decoded-size safety limit: ", entry.file.name});
    if(expandedSize > SevenZipMaximumExpandedSize - entry.file.size)
      return fail("The SevenZip archive exceeds the 2 GiB total expanded-size safety limit.");
    expandedSize += entry.file.size;

    for(auto& existing : impl->entries) {
      if(existing.file.name == entry.file.name)
        return fail({"Duplicate normalized SevenZip member name: ", entry.file.name});
    }
    impl->entries.push_back(std::move(entry));
  }

  return true;
}

auto SevenZipArchive::findFile(const string& filename) const -> const maybe<File> {
  if(!impl) return nothing;

  auto normalized = Archive::normalizeMemberName(filename);
  if(!normalized) {
    impl->errorMessage = {"Unsafe archive member reference: ", filename};
    return nothing;
  }

  for(auto& entry : impl->entries) {
    if(entry.file.name == *normalized) {
      impl->errorMessage = {};
      return entry.file;
    }
  }

  const Impl::Entry* match = nullptr;
  for(auto& entry : impl->entries) {
    if(!entry.file.name.iequals(*normalized)) continue;
    if(match) {
      impl->errorMessage = {"Ambiguous case-insensitive archive member reference: ", *normalized};
      return nothing;
    }
    match = &entry;
  }
  if(match) {
    impl->errorMessage = {};
    return match->file;
  }
  impl->errorMessage = {"Archive member not found: ", *normalized};
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
  auto view = decodedView(file);
  std::vector<u8> output(view.size());
  if(!view.empty()) memory::copy(output.data(), output.size(), view.data(), view.size());
  return output;
}

auto SevenZipArchive::decodedView(const File& file) const -> std::span<const u8> {
  if(!impl || !impl->databaseInitialized) return {};
  std::lock_guard<std::mutex> lock(impl->decoderMutex);

  const Impl::Entry* selected = nullptr;
  for(auto& entry : impl->entries) {
    if(entry.file.name == file.name) {
      selected = &entry;
      break;
    }
  }
  if(!selected) {
    const Impl::Entry* match = nullptr;
    for(auto& entry : impl->entries) {
      if(!entry.file.name.iequals(file.name)) continue;
      if(match) {
        impl->errorMessage = {"Ambiguous case-insensitive archive member reference: ", file.name};
        return {};
      }
      match = &entry;
    }
    selected = match;
  }
  if(!selected) {
    impl->errorMessage = {"Archive member not found: ", file.name};
    return {};
  }
  if(selected->file.size > std::numeric_limits<size_t>::max()) {
    impl->errorMessage = {"SevenZip member is too large for this platform: ", selected->file.name};
    return {};
  }

  if(impl->viewFileIndex == selected->index) {
    if(impl->viewOffset <= impl->blockSize && impl->viewSize <= impl->blockSize - impl->viewOffset)
      return {impl->block + impl->viewOffset, impl->viewSize};
    impl->errorMessage = {"Invalid cached SevenZip member bounds: ", selected->file.name};
    return {};
  }

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
  if(result != SZ_OK) {
    impl->errorMessage = sevenZipError(result, {"Unable to decompress SevenZip member ", selected->file.name});
    return {};
  }
  if(offset > impl->blockSize || extractedSize > impl->blockSize - offset) {
    impl->errorMessage = {"Invalid decoded bounds for SevenZip member: ", selected->file.name};
    return {};
  }
  if(extractedSize != selected->file.size) {
    impl->errorMessage = {"Decoded size mismatch for SevenZip member: ", selected->file.name};
    return {};
  }

  impl->viewFileIndex = selected->index;
  impl->viewOffset = offset;
  impl->viewSize = extractedSize;
  impl->errorMessage = {};
  return {impl->block + offset, extractedSize};
}

auto SevenZipArchive::isDataUncompressed(const File& file) const -> bool {
  // Decode into the SDK's bounded solid-block cache and let sequential disc
  // consumers read that view directly, avoiding a second full-member copy.
  return file.size == 0 || !decodedView(file).empty();
}

auto SevenZipArchive::dataViewIfUncompressed(const File& file) const -> std::span<const u8> {
  return decodedView(file);
}

auto SevenZipArchive::error() const -> string {
  if(!impl) return "SevenZip archive is not initialized.";
  return impl->errorMessage;
}

auto SevenZipArchive::close() -> void {
  if(!impl) return;

  if(impl->block) {
    ISzAlloc_Free(&impl->allocator, impl->block);
    impl->block = nullptr;
  }
  impl->blockIndex = 0xffffffff;
  impl->blockSize = 0;
  impl->viewFileIndex = 0xffffffff;
  impl->viewOffset = 0;
  impl->viewSize = 0;

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
