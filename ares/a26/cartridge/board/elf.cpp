struct ELFCartridge : Interface {
  ELFCartridge(Cartridge& cartridge) : Interface(cartridge) {}

  auto fail(string reason) -> void {
    runtime.unload();
    publishedError = std::move(reason);
    ready = false;
    cartridge.node->setAttribute("error", publishedError);
  }

  auto clearError() -> void {
    publishedError = {};
    cartridge.node->setAttribute("error");
  }

  auto publishRuntimeError() -> bool {
    if(!runtime.faulted()) return false;
    auto error = runtime.error();
    if(error != publishedError) {
      publishedError = std::move(error);
      cartridge.node->setAttribute("error", publishedError);
    }
    return true;
  }

  auto access(n16 address, n8 value) -> n8 {
    if(!ready || publishRuntimeError()) return value;
    return runtime.access(address, value);
  }

  auto load() -> void override {
    image.clear();
    object = {};
    linker = {};
    ready = false;
    clearError();
    if(auto fp = pak->read("program.elf")) {
      if(fp->size() > ELF::MaximumImageSize) return fail("ELF image exceeds 16 MiB");
      image.resize(fp->size());
      fp->read({image.data(), image.size()});
    } else {
      return fail("ELF package is missing program.elf");
    }
    if(!object.parse(image)) return fail(object.error());
    if(!linker.link(object, ELF::externalSymbols(Region::NTSC()))) return fail(linker.error());
    ready = true;
  }

  auto unload() -> void override {
    runtime.unload();
    image.clear();
    object = {};
    linker = {};
    ready = false;
    publishedError = {};
  }

  auto power(bool) -> void override {
    if(!ready) return;
    if(!linker.link(object, ELF::externalSymbols(Region::NTSC()))) return fail(linker.error());
    runtime.power(linker.result(), Region::NTSC());
    clearError();
    publishRuntimeError();
  }

  auto read(n16 address, n8 value) -> n8 override {
    return access(address, value);
  }

  auto write(n16 address, n8 value) -> n8 override {
    return access(address, value);
  }

  auto serialize(serializer& s) -> void override {
    if(!ready) return;
    runtime.serialize(s);
    if(s.reading()) {
      clearError();
      publishRuntimeError();
    }
  }

  std::vector<u8> image;
  ELF::Object object;
  ELF::Linker linker;
  ELF::Runtime runtime;
  bool ready = false;
  string publishedError;
};
