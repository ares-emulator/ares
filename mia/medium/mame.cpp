// Helper/Utility for importing roms for systems that are using MAME romsets
struct Mame : Medium {
  auto loadRoms(string location, Markup::Node& info, string sectionName) -> std::vector<u8>;
  auto loadRomFile(string location, string filename, Markup::Node& currentInfo) -> std::vector<u8>;
  auto endianSwap(std::vector<u8>& memory, u32 address = 0, int size = -1) -> void;
};

auto Mame::loadRoms(string location, Markup::Node& info, string sectionName) -> std::vector<u8> {
  std::vector<u8> output;

  if(!location.iendsWith(".zip") && !location.iendsWith(".7z")) return output;

  string filename = {};
  std::vector<u8> input = {};
  u32 readOffset = 0;
  string loadType = {};

  for(auto section : info[{"game/", sectionName}]) {
    if(section.name() == "rom") {
      if(section["type"] && section["type"].string() != "continue") loadType = section["type"].string();
      if(section["name"]) {
        filename = section["name"].string().strip();
        if(filename) {
          input = {};
          input = loadRomFile(location, filename, info);
          readOffset = 0;
          if (input.empty()) return output;
        }
      }

      auto writeOffset = section["offset"].natural();
      auto size = section["size"].natural();

      auto startIndex = 0;
      auto increment = 1;
      if(loadType == "load16_byte") {
        increment = 2;
        size *= 2;
        if(writeOffset & 1) {
          startIndex = 1;
          writeOffset--;
        }
      }

      if(output.size() < writeOffset + size) output.resize(writeOffset + size);

      for(auto index = startIndex; index < size; index += increment) {
        if(loadType == "fill") {
          output[index + writeOffset] = section["value"].natural();
          continue;
        }

        output[index + writeOffset] = input[readOffset++];
      }

      if(loadType == "load16_word_swap") endianSwap(output, section["offset"].natural(), size);
    }
  }
  return output;
}

auto Mame::loadRomFile(string location, string filename, Markup::Node& info) -> std::vector<u8> {
  if(Decode::Archive::supported(location)) {
    Decode::Archive archive;
    if(archive.open(location)) {
      for(auto& file : archive.file) {
        if(file.name.iequals(filename)) {
          return archive.extract(file);
        }
      }
    }
  }

  if(auto parent = info["game/parent"].string()) {
    auto manifest = manifestDatabaseArcade(parent);
    auto document = BML::unserialize(manifest);
    if(!document) return {};

    auto parentName = document["game/name"].string();
    string parentZip = {Location::path(location), "/", parentName, ".zip"};
    string parent7z = {Location::path(location), "/", parentName, ".7z"};

    if(location.iendsWith(".7z")) {
      if(auto output = loadRomFile(parent7z, filename, document); !output.empty()) return output;
      return loadRomFile(parentZip, filename, document);
    }

    if(auto output = loadRomFile(parentZip, filename, document); !output.empty()) return output;
    return loadRomFile(parent7z, filename, document);
  }

  return {};
};


auto Mame::endianSwap(std::vector<u8>& memory, u32 address, int size) -> void {
  if(size < 0) size = (int)memory.size();
  for(u32 index = 0; index < (u32)size; index += 2) {
    swap(memory[address + index + 0], memory[address + index + 1]);
  }
}
