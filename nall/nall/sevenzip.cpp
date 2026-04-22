#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <nall/decode/sevenzip.hpp>

extern "C" {
#include <7z.h>
#include <7zAlloc.h>
#include <7zCrc.h>
#include <7zFile.h>
#include <7zTypes.h>
}

namespace nall::Decode {

namespace {

static constexpr size_t InputBufferSize = (size_t)1 << 18;
static constexpr int64_t NtfsEpochOffset = 11644473600;

auto utf16ToUtf8(const UInt16* source) -> string {
  std::string output;
  for(size_t index = 0; source[index] != 0; index++) {
    u32 codepoint = source[index];
    if(codepoint >= 0xd800 && codepoint <= 0xdbff) {
      auto next = source[index + 1];
      if(next >= 0xdc00 && next <= 0xdfff) {
        codepoint = ((codepoint - 0xd800) << 10) + (next - 0xdc00) + 0x10000;
        index++;
      }
    }

    if(codepoint <= 0x7f) {
      output.push_back((char)codepoint);
    } else if(codepoint <= 0x7ff) {
      output.push_back((char)(0xc0 | (codepoint >> 6)));
      output.push_back((char)(0x80 | (codepoint & 0x3f)));
    } else if(codepoint <= 0xffff) {
      output.push_back((char)(0xe0 | (codepoint >> 12)));
      output.push_back((char)(0x80 | ((codepoint >> 6) & 0x3f)));
      output.push_back((char)(0x80 | (codepoint & 0x3f)));
    } else {
      output.push_back((char)(0xf0 | (codepoint >> 18)));
      output.push_back((char)(0x80 | ((codepoint >> 12) & 0x3f)));
      output.push_back((char)(0x80 | ((codepoint >> 6) & 0x3f)));
      output.push_back((char)(0x80 | (codepoint & 0x3f)));
    }
  }
  return output.c_str();
}

auto ntfsToUnixTime(const CNtfsFileTime& time) -> time_t {
  u64 ticks = (u64)time.Low | ((u64)time.High << 32);
  int64_t seconds = (int64_t)(ticks / 10000000);
  seconds -= NtfsEpochOffset;
  if(seconds < 0) seconds = 0;
  return (time_t)seconds;
}

}

struct SevenZip::State {
  ISzAlloc allocMain;
  ISzAlloc allocTemp;
  CFileInStream archiveStream;
  CLookToRead2 lookStream;
  CSzArEx archive;
  Byte* outBuffer;
  size_t outBufferSize;
  UInt32 blockIndex;
  bool streamOpen;
  bool archiveOpen;

  State() {
    allocMain = {SzAlloc, SzFree};
    allocTemp = {SzAllocTemp, SzFreeTemp};

    File_Construct(&archiveStream.file);
    FileInStream_CreateVTable(&archiveStream);
    archiveStream.wres = 0;

    LookToRead2_CreateVTable(&lookStream, False);
    lookStream.buf = nullptr;
    lookStream.bufSize = 0;
    lookStream.realStream = &archiveStream.vt;
    LookToRead2_INIT(&lookStream);

    SzArEx_Init(&archive);

    outBuffer = nullptr;
    outBufferSize = 0;
    blockIndex = 0xffffffff;
    streamOpen = false;
    archiveOpen = false;
  }
};

NALL_HEADER_INLINE SevenZip::~SevenZip() {
  close();
}

NALL_HEADER_INLINE auto SevenZip::findFile(const string& filename) const -> const maybe<File> {
  for(const auto& currentFile : file) {
    if(currentFile.name.iequals(filename)) {
      return currentFile;
    }
  }
  return nothing;
}

NALL_HEADER_INLINE auto SevenZip::open(const string& filename) -> bool {
  close();
  state = new State;

  auto wres = InFile_Open(&state->archiveStream.file, filename.data());
  if(wres != 0) {
    close();
    return false;
  }
  state->streamOpen = true;

  state->lookStream.buf = (Byte*)ISzAlloc_Alloc(&state->allocMain, InputBufferSize);
  if(!state->lookStream.buf) {
    close();
    return false;
  }
  state->lookStream.bufSize = InputBufferSize;
  LookToRead2_INIT(&state->lookStream);

  CrcGenerateTable();

  auto result = SzArEx_Open(&state->archive, &state->lookStream.vt, &state->allocMain, &state->allocTemp);
  if(result != SZ_OK) {
    close();
    return false;
  }
  state->archiveOpen = true;

  file.clear();
  file.reserve(state->archive.NumFiles);

  std::vector<UInt16> utf16Name;
  for(u32 index = 0; index < (u32)state->archive.NumFiles; index++) {
    auto length = SzArEx_GetFileNameUtf16(&state->archive, index, nullptr);
    if(length == 0) continue;

    utf16Name.resize(length);
    SzArEx_GetFileNameUtf16(&state->archive, index, utf16Name.data());

    File archiveFile;
    archiveFile.name = utf16ToUtf8(utf16Name.data());
    archiveFile.size = SzArEx_GetFileSize(&state->archive, index);
    archiveFile.index = index;
    archiveFile.directory = SzArEx_IsDir(&state->archive, index) != 0;
    archiveFile.timestamp = 0;
    if(SzBitWithVals_Check(&state->archive.MTime, index)) {
      archiveFile.timestamp = ntfsToUnixTime(state->archive.MTime.Vals[index]);
    }

    file.push_back(archiveFile);
  }

  return true;
}

NALL_HEADER_INLINE auto SevenZip::extract(const File& file) -> std::vector<u8> {
  std::vector<u8> output;
  if(!state || !state->archiveOpen) return output;
  if(file.directory) return output;
  if(file.index >= state->archive.NumFiles) return output;

  size_t offset = 0;
  size_t outSizeProcessed = 0;
  auto result = SzArEx_Extract(
    &state->archive,
    &state->lookStream.vt,
    file.index,
    &state->blockIndex,
    &state->outBuffer,
    &state->outBufferSize,
    &offset,
    &outSizeProcessed,
    &state->allocMain,
    &state->allocTemp
  );
  if(result != SZ_OK || !state->outBuffer) return output;

  output.resize(outSizeProcessed);
  if(outSizeProcessed) {
    memcpy(output.data(), state->outBuffer + offset, outSizeProcessed);
  }
  return output;
}

NALL_HEADER_INLINE auto SevenZip::close() -> void {
  file.clear();
  if(!state) return;

  if(state->outBuffer) {
    ISzAlloc_Free(&state->allocMain, state->outBuffer);
    state->outBuffer = nullptr;
    state->outBufferSize = 0;
  }

  if(state->archiveOpen) {
    SzArEx_Free(&state->archive, &state->allocMain);
    state->archiveOpen = false;
  }

  if(state->lookStream.buf) {
    ISzAlloc_Free(&state->allocMain, state->lookStream.buf);
    state->lookStream.buf = nullptr;
    state->lookStream.bufSize = 0;
  }

  if(state->streamOpen) {
    File_Close(&state->archiveStream.file);
    state->streamOpen = false;
  }

  delete state;
  state = nullptr;
}

}