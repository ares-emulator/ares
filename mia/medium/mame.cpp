// Helper/Utility for importing ROMs for systems that use MAME ROM sets.
struct Mame : Medium {
  enum class AssemblyStatus : u32 {
    Successful,
    InvalidArchive,
    ExtractionFailed,
    MissingMember,
    TruncatedMember,
    MalformedMetadata,
    MalformedParent,
    ParentCycle,
  };

  struct AssemblyResult {
    AssemblyStatus status;
    std::vector<u8> data;
    string region;
    string member;
    string archive;
    string detail;
    std::vector<string> archiveChain;

    AssemblyResult() : status(AssemblyStatus::Successful) {}
    AssemblyResult(
      AssemblyStatus status, std::vector<u8> data, string region,
      string member, string archive, string detail,
      std::vector<string> archiveChain
    ) : status(status), data(std::move(data)), region(std::move(region)), member(std::move(member)),
        archive(std::move(archive)), detail(std::move(detail)), archiveChain(std::move(archiveChain)) {}

    AssemblyResult(
      AssemblyStatus status, string region, string member,
      string archive, string detail,
      std::vector<string> archiveChain
    ) : AssemblyResult(status, {}, std::move(region), std::move(member), std::move(archive),
        std::move(detail), std::move(archiveChain)) {}

    explicit operator bool() const { return status == AssemblyStatus::Successful; }
  };

  auto loadRoms(string location, Markup::Node& info, string sectionName, std::function<Markup::Node (string)> resolver = {}) -> AssemblyResult;
  auto loadRomFile(string location, string filename, string region, Markup::Node currentInfo, std::function<Markup::Node (string)> resolver, maybe<u32> checksum = nothing, bool extract = true) -> AssemblyResult;
  auto parseNatural(Markup::Node node, u64& value) const -> bool;
  auto assemblyError(const AssemblyResult& result) const -> string;
  auto endianSwap(std::vector<u8>& memory, u64 address = 0, u64 size = std::numeric_limits<u64>::max()) -> bool;
};

auto Mame::parseNatural(Markup::Node node, u64& value) const -> bool {
  if(!node) return false;
  auto text = node.string().strip();
  if(!text) return false;

  u32 offset = 0;
  bool hexadecimal = text.beginsWith("0x") || text.beginsWith("0X");
  if(hexadecimal) offset = 2;
  if(offset == text.size()) return false;

  value = 0;
  u32 base = hexadecimal ? 16 : 10;
  for(u32 index = offset; index < text.size(); index++) {
    auto character = text[index];
    bool decimalDigit = character >= '0' && character <= '9';
    bool hexadecimalDigit = hexadecimal && (
      (character >= 'a' && character <= 'f')
      || (character >= 'A' && character <= 'F')
    );
    if(!decimalDigit && !hexadecimalDigit) return false;
    u32 digit = decimalDigit ? character - '0' : (character | 0x20) - 'a' + 10;
    if(value > (std::numeric_limits<u64>::max() - digit) / base) return false;
    value = value * base + digit;
  }

  return true;
}

auto Mame::loadRoms(string location, Markup::Node& info, string sectionName, std::function<Markup::Node (string)> resolver) -> AssemblyResult {
  if(!location.iendsWith(".zip")) {
    return {AssemblyStatus::InvalidArchive, sectionName, {}, location, "expected a .zip archive", {}};
  }

  if(!resolver) {
    resolver = [&](string parent) -> Markup::Node {
      return BML::unserialize(manifestDatabaseArcade(parent));
    };
  }

  string filename;
  std::vector<u8> input;
  string inputArchive;
  std::vector<string> inputArchiveChain;
  u64 readOffset = 0;
  string loadType;
  bool hasInput = false;
  AssemblyResult result;

  for(auto section : info[{"game/", sectionName}]) {
    if(section.name() != "rom") continue;

    auto recordName = section["name"].string().strip();
    auto assemblyFailure = [&](AssemblyStatus status, string detail) -> AssemblyResult {
      auto member = recordName ? recordName : filename;
      return {status, sectionName, member, {}, std::move(detail), {}};
    };

    auto type = section["type"].string();
    if(type
    && type != "continue"
    && type != "fill"
    && type != "ignore"
    && type != "load"
    && type != "load16_byte"
    && type != "load16_word_swap"
    && type != "load32_byte"
    && type != "load32_word_swap"
    && type != "copy") {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, {"unsupported ROM load type: ", type});
    }

    //copy duplicates data already assembled in this region (MAME's ROM_COPY)
    if(type == "copy") {
      u64 from = 0, to = 0, size = 0;
      if(section["name"] || !parseNatural(section["from"], from)
      || !parseNatural(section["offset"], to) || !parseNatural(section["size"], size)) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM copy record needs from, offset and size");
      }
      if(from > result.data.size() || size > result.data.size() - from) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM copy source is outside the region");
      }
      if(result.data.size() < to + size) result.data.resize(to + size);
      memmove(result.data.data() + to, result.data.data() + from, size);
      continue;
    }

    u64 fillValue = 0;
    bool fillRecord = type == "fill";
    if(type == "continue" || type == "ignore") {
      if(section["name"] || section["value"]) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM reuse record must only contain offset and size");
      }
    } else if(fillRecord) {
      if(section["name"] || !parseNatural(section["value"], fillValue) || fillValue > 0xff) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM fill record has an invalid name or value");
      }
      loadType = type;
      hasInput = false;
    } else {
      if(!section["name"] || section["value"]) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM file record has an invalid name or value");
      }
      filename = recordName;
      if(!filename) {
        return {AssemblyStatus::MalformedMetadata, sectionName, {}, {}, "ROM file record has an empty member name", {}};
      }
      loadType = type;

      maybe<u32> checksum;
      if(section["crc"]) checksum = (u32)section["crc"].string().hex();
      auto file = loadRomFile(location, filename, sectionName, info, resolver, checksum);
      if(!file) return file;
      input = std::move(file.data);
      inputArchive = std::move(file.archive);
      inputArchiveChain = std::move(file.archiveChain);
      readOffset = 0;
      hasInput = true;
    }

    if(!section["offset"] || !section["size"]) {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM record is missing offset or size");
    }

    u64 writeOffset = 0;
    u64 sourceSize = 0;
    if(!parseNatural(section["offset"], writeOffset) || !parseNatural(section["size"], sourceSize)) {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM record has an invalid offset or size");
    }
    if(!sourceSize) {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM record has an empty extent");
    }
    u64 outputSize = sourceSize;
    u64 startIndex = 0;
    u64 increment = 1;

    if(loadType == "load16_byte") {
      if(sourceSize > std::numeric_limits<u64>::max() / 2) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "interleaved ROM size overflows");
      }
      outputSize = sourceSize * 2;
      increment = 2;
      if(writeOffset & 1) {
        startIndex = 1;
        writeOffset--;
      }
    }

    if(loadType == "load32_byte") {
      if(sourceSize > std::numeric_limits<u64>::max() / 4) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "interleaved ROM size overflows");
      }
      outputSize = sourceSize * 4;
      increment = 4;
      startIndex = writeOffset & 3;
      writeOffset -= startIndex;
    }

    //load32_word_swap: each source word is byte-swapped into the first or second half of a 32-bit group
    bool wordInterleave = loadType == "load32_word_swap";
    if(wordInterleave) {
      if(sourceSize & 1) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "word-interleaved ROM size must be even");
      }
      if(sourceSize > std::numeric_limits<u64>::max() / 2) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "interleaved ROM size overflows");
      }
      outputSize = sourceSize * 2;
      startIndex = writeOffset & 3;
      writeOffset -= startIndex;
    }

    if(writeOffset > std::numeric_limits<u64>::max() - outputSize) {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM destination range overflows");
    }

    u64 outputEnd = writeOffset + outputSize;
    if(outputEnd > result.data.max_size()) {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM destination range is too large");
    }

    if(!fillRecord) {
      if(!hasInput) {
        return assemblyFailure(AssemblyStatus::MalformedMetadata, "ROM continuation has no source member");
      }

      u64 bytesRequired = 0;
      if(wordInterleave) {
        bytesRequired = sourceSize;
      } else if(outputSize > startIndex) {
        bytesRequired = (outputSize - startIndex + increment - 1) / increment;
      }
      if(readOffset > input.size()
      || bytesRequired > input.size() - readOffset) {
        string detail = {
          "requested ", bytesRequired, " bytes at source offset ", readOffset,
          ", but only ", input.size() - std::min<u64>(readOffset, input.size()), " remain"
        };
        return {
          AssemblyStatus::TruncatedMember,
          sectionName, filename, inputArchive,
          std::move(detail), inputArchiveChain
        };
      }
    }

    //ignore skips source data without writing it
    if(type == "ignore") {
      readOffset += sourceSize;
      continue;
    }

    if(result.data.size() < outputEnd) result.data.resize(outputEnd);

    if(wordInterleave) {
      for(u64 index = startIndex; index < outputSize; index += 4) {
        result.data[index + writeOffset + 1] = input[readOffset++];
        result.data[index + writeOffset + 0] = input[readOffset++];
      }
      continue;
    }

    for(u64 index = startIndex; index < outputSize; index += increment) {
      if(fillRecord) {
        result.data[index + writeOffset] = fillValue;
        continue;
      }
      result.data[index + writeOffset] = input[readOffset++];
    }

    if(loadType == "load16_word_swap" && !endianSwap(result.data, writeOffset, sourceSize)) {
      return assemblyFailure(AssemblyStatus::MalformedMetadata, "word-swap range is invalid");
    }
  }

  return result;
}

auto Mame::loadRomFile(string location, string filename, string region, Markup::Node currentInfo, std::function<Markup::Node (string)> resolver, maybe<u32> checksum, bool extract) -> AssemblyResult {
  auto romRoot = Location::path(location);
  auto board = currentInfo["game/board"].string();
  auto system = Location::file(board);
  if(system && Location::prefix(romRoot).iequals(system)) romRoot = Location::dir(romRoot);
  std::vector<string> visited;
  std::vector<string> archiveChain;
  bool openedArchive = false;
  auto assemblyFailure = [&](AssemblyStatus status, string detail) {
    return AssemblyResult{status, region, filename, location, std::move(detail), std::move(archiveChain)};
  };

  while(true) {
    auto currentName = currentInfo["game/name"].string();
    if(!currentName) {
      return assemblyFailure(AssemblyStatus::MalformedParent, "manifest is missing game/name");
    }
    for(auto& name : visited) {
      if(name.iequals(currentName)) {
        return assemblyFailure(AssemblyStatus::ParentCycle, {"parent cycle repeats ", currentName});
      }
    }
    visited.push_back(currentName);

    std::vector<string> candidates{location};
    if(system) candidates.push_back({romRoot, system, "/", currentName, ".zip"});
    candidates.push_back({romRoot, currentName, ".zip"});
    for(auto& candidate : candidates) {
      bool alreadyTried = false;
      for(auto& previous : archiveChain) {
        if(previous.iequals(candidate)) alreadyTried = true;
      }
      if(alreadyTried) continue;
      archiveChain.push_back(candidate);
      if(!inode::exists(candidate)) continue;  //Merged sets need no separate clone ZIP.
      location = candidate;
      Decode::ZIP archive;
      if(!archive.open(candidate)) {
        return assemblyFailure(AssemblyStatus::InvalidArchive, "could not open a ZIP archive in the parent chain");
      }
      openedArchive = true;

      const Decode::ZIP::File* member = nullptr;
      //A merged parent may contain clone-specific files alongside shared ROMs.
      std::vector<string> memberNames;
      for(auto& name : visited) memberNames.push_back({name, "/", filename});
      memberNames.push_back(filename);
      for(auto& memberName : memberNames) {
        for(auto& file : archive.file) {
          if(!file.name.iequals(memberName)) continue;
          if(checksum && file.crc32 != checksum()) continue;
          member = &file;
          break;
        }
        if(member) break;
      }
      //MAME ROM names can change between machine and software-list definitions.
      if(!member && checksum) {
        for(auto& file : archive.file) {
          if(file.crc32 != checksum()) continue;
          member = &file;
          break;
        }
      }
      if(!member) continue;
      if(!extract) return {AssemblyStatus::Successful, {}, {}, filename, candidate, {}, std::move(archiveChain)};
      auto data = archive.extract(*member);
      if(data.size() != member->size || (checksum && Hash::CRC32(data).value() != checksum())) {
        return {
          AssemblyStatus::ExtractionFailed, std::move(data), region, filename, candidate,
          "extracted ROM does not match its size or checksum", std::move(archiveChain)
        };
      }
      return {AssemblyStatus::Successful, std::move(data), {}, filename, candidate, {}, std::move(archiveChain)};
    }

    auto parent = currentInfo["game/parent"].string();
    if(!parent) {
      return assemblyFailure(openedArchive ? AssemblyStatus::MissingMember : AssemblyStatus::InvalidArchive,
        openedArchive ? "member was not found in the archive chain" : "no ZIP archive was found in the parent chain");
    }
    auto parentInfo = resolver(parent);
    if(!parentInfo || !parentInfo["game/name"].string()) {
      return assemblyFailure(AssemblyStatus::MalformedParent, {"database manifest is missing for parent ", parent});
    }
    auto parentName = parentInfo["game/name"].string();
    location = {Location::path(location), parentName, ".zip"};
    currentInfo = parentInfo;
  }
}

auto Mame::assemblyError(const AssemblyResult& result) const -> string {
  string message;

  switch(result.status) {
  case AssemblyStatus::Successful:
    return {};
  case AssemblyStatus::InvalidArchive:
    message = {"Could not open MAME archive: ", result.archive, ". "};
    break;
  case AssemblyStatus::ExtractionFailed:
    message = {"Could not extract MAME member ", result.member, " from ", result.archive, ". "};
    break;
  case AssemblyStatus::MissingMember:
    message = {"Missing MAME member: ", result.member, ". "};
    break;
  case AssemblyStatus::TruncatedMember:
    message = {"MAME member is too short: ", result.member, ". "};
    break;
  case AssemblyStatus::MalformedMetadata:
    message = {"Malformed MAME metadata for region ", result.region, ". "};
    break;
  case AssemblyStatus::MalformedParent:
    message = {"Malformed MAME parent metadata for member ", result.member, ". "};
    break;
  case AssemblyStatus::ParentCycle:
    message = {"MAME parent cycle while resolving member ", result.member, ". "};
    break;
  }

  if(!result.archiveChain.empty()) {
    message.append("Archive chain: ");
    for(u32 index : range((u32)result.archiveChain.size())) {
      if(index) message.append(" -> ");
      message.append(Location::file(result.archiveChain[index]));
    }
    message.append(". ");
  }
  if(result.detail) message.append(result.detail, ". ");
  return message;
}

auto Mame::endianSwap(std::vector<u8>& memory, u64 address, u64 size) -> bool {
  if(address > memory.size()) return false;
  if(size == std::numeric_limits<u64>::max()) size = memory.size() - address;
  if(size > memory.size() - address || (size & 1)) return false;
  for(u64 index = 0; index < size; index += 2) {
    swap(memory[address + index + 0], memory[address + index + 1]);
  }
  return true;
}
