#include "archive.hpp"

#include <nall/decode/zip.hpp>

extern "C" {
#include <7z.h>
#include <7zAlloc.h>
#include <7zCrc.h>
#include <7zFile.h>
}

struct ZipArchive final : Archive {
  auto entries() const -> const std::vector<nall::string>& override { return fileNames; }

  auto extract(u32 index) -> std::vector<u8> override {
    if(index >= archive.file.size()) return {};
    return archive.extract(archive.file[index]);
  }

  Decode::ZIP archive;
  std::vector<nall::string> fileNames;
};

struct SevenZipArchive final : Archive {
  SevenZipArchive() {
    alloc = {SzAlloc, SzFree};
    allocTemp = {SzAllocTemp, SzFreeTemp};
    File_Construct(&stream.file);
    SzArEx_Init(&database);
  }

  ~SevenZipArchive() override {
    if(outputBuffer) {
      ISzAlloc_Free(&alloc, outputBuffer);
      outputBuffer = nullptr;
    }
    SzArEx_Free(&database, &alloc);
    File_Close(&stream.file);
  }

  auto open(const nall::string& location) -> bool {
    if(InFile_Open(&stream.file, location) != 0) return false;

    FileInStream_CreateVTable(&stream);
    LookToRead2_CreateVTable(&look, False);
    look.realStream = &stream.vt;
    look.buf = inputBuffer;
    look.bufSize = sizeof(inputBuffer);
    LookToRead2_INIT(&look);

    CrcGenerateTable();

    if(SzArEx_Open(&database, &look.vt, &alloc, &allocTemp) != SZ_OK) {
      File_Close(&stream.file);
      return false;
    }

    for(u32 index = 0; index < database.NumFiles; index++) {
      if(SzArEx_IsDir(&database, index)) continue;

      auto length = SzArEx_GetFileNameUtf16(&database, index, nullptr);
      std::vector<UInt16> utf16;
      utf16.resize(length);
      SzArEx_GetFileNameUtf16(&database, index, utf16.data());

      nall::string name;
      for(u32 offset = 0; offset + 1 < length; offset++) {
        auto code = utf16[offset];
        if(code == '\\') code = '/';
        if(code < 0x80) name.append((char)code);
        else name.append('?');
      }

      fileNames.push_back(name);
      fileIndices.push_back(index);
    }

    return true;
  }

  auto entries() const -> const std::vector<nall::string>& override { return fileNames; }

  auto extract(u32 index) -> std::vector<u8> override {
    std::vector<u8> memory;
    if(index >= fileIndices.size()) return memory;

    size_t offset = 0;
    size_t outputSizeProcessed = 0;
    auto result = SzArEx_Extract(
      &database,
      &look.vt,
      fileIndices[index],
      &blockIndex,
      &outputBuffer,
      &outputBufferSize,
      &offset,
      &outputSizeProcessed,
      &alloc,
      &allocTemp
    );
    if(result != SZ_OK) return memory;

    memory.resize(outputSizeProcessed);
    if(outputSizeProcessed) memcpy(memory.data(), outputBuffer + offset, outputSizeProcessed);
    return memory;
  }

  CFileInStream stream = {};
  CLookToRead2 look = {};
  CSzArEx database = {};
  ISzAlloc alloc = {};
  ISzAlloc allocTemp = {};
  UInt32 blockIndex = 0xffffffff;
  Byte* outputBuffer = nullptr;
  size_t outputBufferSize = 0;
  Byte inputBuffer[1 << 16] = {};

  std::vector<nall::string> fileNames;
  std::vector<u32> fileIndices;
};

auto isArchive(const nall::string& location) -> bool {
  return location.iendsWith(".zip") || location.iendsWith(".7z");
}

auto openArchive(const nall::string& location) -> std::shared_ptr<Archive> {
  if(location.iendsWith(".zip")) {
    auto archive = std::make_shared<ZipArchive>();
    if(!archive->archive.open(location)) return {};
    for(auto& file : archive->archive.file) {
      archive->fileNames.push_back(file.name);
    }
    return archive;
  }

  if(location.iendsWith(".7z")) {
    auto archive = std::make_shared<SevenZipArchive>();
    if(!archive->open(location)) return {};
    return archive;
  }

  return {};
}

auto archiveExtractFirstMatching(const std::shared_ptr<Archive>& archive, const std::vector<nall::string>& match) -> std::vector<u8> {
  if(!archive) return {};

  auto& entries = archive->entries();
  for(u32 index = 0; index < entries.size(); index++) {
    for(auto& pattern : match) {
      if(entries[index].imatch(pattern)) {
        auto memory = archive->extract(index);
        if(!memory.empty()) return memory;
      }
    }
  }

  return {};
}

auto archiveExtractByName(const std::shared_ptr<Archive>& archive, const nall::string& name) -> std::vector<u8> {
  if(!archive) return {};

  auto& entries = archive->entries();
  for(u32 index = 0; index < entries.size(); index++) {
    if(entries[index].iequals(name)) {
      return archive->extract(index);
    }
  }

  return {};
}
