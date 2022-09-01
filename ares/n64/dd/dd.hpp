//Disk Drive

struct DD : Memory::IO<DD> {
  Node::Object node;
  Node::Port drive;
  Node::Peripheral disk;
  VFS::Pak pak;
  VFS::File fd;
  Memory::Readable iplrom;
  Memory::Writable c2s;
  Memory::Writable ds;
  Memory::Writable ms;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  auto title() const -> string { return information.title; }
  auto cic() const -> string { return information.cic; }

  //dd.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string title;
    string cic = "CIC-NUS-8303";
  } information;
};

extern DD dd;
