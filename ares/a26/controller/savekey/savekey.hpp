struct SaveKey : Controller {
  SaveKey(Node::Port, string name = "SaveKey", string filename = "savekey.eeprom");

  auto save() -> void override;
  auto power(bool reset) -> void override;
  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto serialize(serializer&) -> void override;

protected:
  auto synchronizeEeprom() -> void;
  auto synchronizePersistent() -> void;

  VFS::Pak pak;
  M24C eeprom;
  PersistentMemory persistent;
};
