struct None : Interface {
  using Interface::Interface;

  auto load() -> void override {
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
  }

  auto read(n16 address, n8 data) -> n8 override {
    return 0xff;
  }

  auto write(n16 address, n8 data) -> void override {
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }
};
